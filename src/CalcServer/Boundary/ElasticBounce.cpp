/*
 *  This file is part of AQUAgpusph, a free CFD program based on SPH.
 *  Copyright (C) 2012  Jose Luis Cercos Pita <jl.cercos@upm.es>
 *
 *  AQUAgpusph is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  AQUAgpusph is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with AQUAgpusph.  If not, see <http://www.gnu.org/licenses/>.
 */

// ----------------------------------------------------------------------------
// Include the Problem setup manager header
// ----------------------------------------------------------------------------
#include <ProblemSetup.h>

// ----------------------------------------------------------------------------
// Include the Problem setup manager header
// ----------------------------------------------------------------------------
#include <ScreenManager.h>

// ----------------------------------------------------------------------------
// Include the main header
// ----------------------------------------------------------------------------
#include <CalcServer/Boundary/ElasticBounce.h>

// ----------------------------------------------------------------------------
// Include the calculation server
// ----------------------------------------------------------------------------
#include <CalcServer.h>

namespace Aqua{ namespace CalcServer{ namespace Boundary{

ElasticBounce::ElasticBounce()
	: Kernel("ElasticBounce")
	, mPath(0)
	, program(0)
	, kernel(0)
{
	InputOutput::ScreenManager *S = InputOutput::ScreenManager::singleton();
	InputOutput::ProblemSetup *P  = InputOutput::ProblemSetup::singleton();
	if(   P->SPH_opts.boundary_type!=0  // ElasticBounce boundary
	   && P->SPH_opts.boundary_type!=1  // Fixed particles boundary
	   && P->SPH_opts.boundary_type!=2  // DeLeffe boundary
	   )
	    return;
	//! 1st.- Get data
	int nChar = strlen(P->OpenCL_kernels.elastic_bounce);
	if(nChar <= 0) {
	    S->addMessage(3, "(ElasticBounce::ElasticBounce): mPath of ElasticBounce kernel is empty.\n");
	    exit(EXIT_FAILURE);
	}
	mPath = new char[nChar+4];
	if(!mPath) {
	    S->addMessage(3, "(ElasticBounce::ElasticBounce): Can't allocate memory for path.\n");
	    exit(EXIT_FAILURE);
	}
	strcpy(mPath, P->OpenCL_kernels.elastic_bounce);
	strcat(mPath, ".cl");
	//! 2nd.- Setup the kernel
	local_work_size  = localWorkSize();
	if(!local_work_size){
	    S->addMessage(3, "(ElasticBounce::ElasticBounce): No valid local work size for required computation.\n");
	    exit(EXIT_FAILURE);
	}
	global_work_size = globalWorkSize(local_work_size);
	if(setupOpenCL()) {
	    exit(EXIT_FAILURE);
	}
	S->addMessage(1, "(ElasticBounce::ElasticBounce): ElasticBounce boundary condition ready to work!\n");
}

ElasticBounce::~ElasticBounce()
{
	if(kernel)clReleaseKernel(kernel); kernel=0;
	if(program)clReleaseProgram(program); program=0;
	if(mPath) delete[] mPath; mPath=0;
}

bool ElasticBounce::execute()
{
	//! Test if boundary condition must be executed
	InputOutput::ProblemSetup *P = InputOutput::ProblemSetup::singleton();
	if(   P->SPH_opts.boundary_type!=0  // ElasticBounce boundary
	   && P->SPH_opts.boundary_type!=1  // Fixed particles boundary
	   && P->SPH_opts.boundary_type!=2  // DeLeffe boundary
	   )
	    return false;
	InputOutput::ScreenManager *S = InputOutput::ScreenManager::singleton();
	CalcServer *C = CalcServer::singleton();
	cl_int err_code=0;
	//! Send variables to server
	err_code  = sendArgument(kernel,  0, sizeof(cl_mem  ), (void*)&(C->imovein));
	err_code |= sendArgument(kernel,  1, sizeof(cl_mem  ), (void*)&(C->posin));
	err_code |= sendArgument(kernel,  2, sizeof(cl_mem  ), (void*)&(C->v));
	err_code |= sendArgument(kernel,  3, sizeof(cl_mem  ), (void*)&(C->f));
	err_code |= sendArgument(kernel,  4, sizeof(cl_mem  ), (void*)&(C->fin));
	err_code |= sendArgument(kernel,  5, sizeof(cl_mem  ), (void*)&(C->normal));
	err_code |= sendArgument(kernel,  6, sizeof(cl_mem  ), (void*)&(C->hpin));
	err_code |= sendArgument(kernel,  7, sizeof(cl_mem  ), (void*)&(C->pos));
	err_code |= sendArgument(kernel,  8, sizeof(cl_mem  ), (void*)&(C->icell));
	err_code |= sendArgument(kernel,  9, sizeof(cl_mem  ), (void*)&(C->ihoc));
	err_code |= sendArgument(kernel, 10, sizeof(cl_mem  ), (void*)&(C->permutation));
	err_code |= sendArgument(kernel, 11, sizeof(cl_mem  ), (void*)&(C->permutation_inverse));
	err_code |= sendArgument(kernel, 12, sizeof(cl_uint ), (void*)&(C->N));
	err_code |= sendArgument(kernel, 13, sizeof(cl_float), (void*)&(C->dt));
	err_code |= sendArgument(kernel, 14, sizeof(uivec   ), (void*)&(C->num_cells_vec));
	err_code |= sendArgument(kernel, 15, sizeof(vec     ), (void*)&(C->g));
	if(err_code != CL_SUCCESS) {
		S->addMessage(3, "(ElasticBounce::Boundary): Can't send arguments to boundary computation kernel.\n");
	    return true;
	}
	//! Execute the kernel
	#ifdef HAVE_GPUPROFILE
	    cl_event event;
	    cl_ulong end, start;
	    profileTime(0.f);
	    err_code = clEnqueueNDRangeKernel(C->command_queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, &event);
	#else
	    err_code = clEnqueueNDRangeKernel(C->command_queue, kernel, 1, NULL, &global_work_size, NULL, 0, NULL, NULL);
	#endif
	if(err_code != CL_SUCCESS) {
		S->addMessage(3, "(ElasticBounce::Boundary): Can't execute the kernel.\n");
	    if(err_code == CL_INVALID_WORK_GROUP_SIZE)
	        S->addMessage(0, "\tInvalid local work group size.\n");
	    else if(err_code == CL_OUT_OF_RESOURCES)
	        S->addMessage(0, "\tDevice out of resources.\n");
	    else if(err_code == CL_MEM_OBJECT_ALLOCATION_FAILURE)
	        S->addMessage(0, "\tAllocation error at device.\n");
	    else if(err_code == CL_OUT_OF_HOST_MEMORY)
	        S->addMessage(0, "\tfailure to allocate resources required by the OpenCL implementation on the host.\n");
	    return true;
	}
	//! Profile the kernel execution
	#ifdef HAVE_GPUPROFILE
	    err_code = clWaitForEvents(1, &event);
	    if(err_code != CL_SUCCESS) {
	        S->addMessage(3, "(ElasticBounce::Boundary): Can't wait to kernels end.\n");
	        return true;
	    }
	    err_code |= clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &end, 0);
	    err_code |= clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &start, 0);
	    if(err_code != CL_SUCCESS) {
	        S->addMessage(3, "(ElasticBounce::Boundary): Can't profile kernel execution.\n");
	        return true;
	    }
	    profileTime(profileTime() + (end - start)/1000.f);  // 10^-3 ms
	#endif
	return false;
}

bool ElasticBounce::setupOpenCL()
{
	InputOutput::ScreenManager *S = InputOutput::ScreenManager::singleton();
	InputOutput::ProblemSetup *P  = InputOutput::ProblemSetup::singleton();
	CalcServer *C = CalcServer::singleton();
	char msg[1024];
	cl_int err_code;
	cl_device_id device;
	cl_ulong localMem, reqLocalMem;
	err_code |= clGetCommandQueueInfo(C->command_queue,CL_QUEUE_DEVICE,
	                                sizeof(cl_device_id),&device, NULL);
	if(err_code != CL_SUCCESS) {
		S->addMessage(3, "(ElasticBounce::setupOpenCL): Can't get device from command queue.\n");
	    return true;
	}
	err_code |= clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(localMem), &localMem, NULL);
	if(err_code != CL_SUCCESS) {
		S->addMessage(3, "(ElasticBounce::setupOpenCL): Can't get local memory available on device.\n");
	    return true;
	}
	char flags[512];
	sprintf(flags, "-D__ELASTIC_FACTOR__=%ff", P->SPH_opts.elastic_factor);
	sprintf(flags, "%s -D__MIN_BOUND_DIST__=%ff", flags, fabs(P->SPH_opts.elastic_dist));
    if(P->SPH_opts.elastic_dist < 0.f){
        sprintf(flags, "%s -D__FORCE_MIN_BOUND_DIST__", flags);
    }
	if(!loadKernelFromFile(&kernel, &program, C->context, C->device, mPath, "Boundary", flags))
	    return true;
	if(program)clReleaseProgram(program); program=0;
	//! Test if there are enough local memory
	err_code |= clGetKernelWorkGroupInfo(kernel,device,CL_KERNEL_LOCAL_MEM_SIZE,
	                                   sizeof(cl_ulong), &reqLocalMem, NULL);
	if(err_code != CL_SUCCESS) {
		S->addMessage(3, "(ElasticBounce::setupOpenCL): Can't get kernel memory usage.\n");
	    return true;
	}
	if(localMem < reqLocalMem){
		S->addMessage(3, "(ElasticBounce::setupOpenCL): Not enough local memory for execution.\n");
	    sprintf(msg, "\tNeeds %lu bytes, but only %lu bytes are available.\n",
	           reqLocalMem, localMem);
	    S->addMessage(0, msg);
	    return true;
	}
	//! Test if local work gorup size must be modified
	size_t localWorkGroupSize=0;
	err_code |= clGetKernelWorkGroupInfo(kernel,device,CL_KERNEL_WORK_GROUP_SIZE,
	                                   sizeof(size_t), &localWorkGroupSize, NULL);
	if(err_code != CL_SUCCESS) {
		S->addMessage(3, "(ElasticBounce::setupOpenCL): Can't get maximum local work group size.\n");
	    return true;
	}
	if(localWorkGroupSize < local_work_size)
	    local_work_size  = localWorkGroupSize;
	//! Look for better local work group size
	err_code |= clGetKernelWorkGroupInfo(kernel,device,CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE,
	                                   sizeof(size_t), &localWorkGroupSize, NULL);
	if(err_code != CL_SUCCESS) {
		S->addMessage(3, "(ElasticBounce::setupOpenCL): Can't get preferred local work group size.\n");
	    return true;
	}
	local_work_size  = (local_work_size/localWorkGroupSize) * localWorkGroupSize;
	global_work_size = globalWorkSize(local_work_size);
	return false;
}

}}}  // namespaces
