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

/** @file
 * @brief OpenCL kernel kernel based tool.
 * (see Aqua::CalcServer::Kernel for details)
 */

#include <CalcServer/Kernel.h>
#include <CalcServer.h>
#include <ScreenManager.h>

namespace Aqua{ namespace CalcServer{

Kernel::Kernel(const char* tool_name,
               const char* kernel_path,
               const char* entry_point,
               const char* n)
    : Tool(tool_name)
    , _path(NULL)
    , _kernel(NULL)
    , _work_group_size(0)
    , _global_work_size(0)
    , _n(NULL)
{
    path(kernel_path);
    _entry_point = new char[strlen(entry_point) + 1];
    strcpy(_entry_point, entry_point);
    _n = new char[strlen(n) + 1];
    strcpy(_n, n);
}

Kernel::~Kernel()
{
    unsigned int i;
    if(_path) delete[] _path; _path=NULL;
    if(_entry_point) delete[] _entry_point; _entry_point=NULL;
    if(_n) delete[] _n; _n=NULL;
    if(_kernel) clReleaseKernel(_kernel); _kernel=NULL;
    for(i = 0; i < _var_names.size(); i++){
        delete[] _var_names.at(i);
    }
    _var_names.clear();
	for(i = 0; i < _var_values.size(); i++){
        free(_var_values.at(i));
	}
	_var_values.clear();
}

bool Kernel::setup()
{
    char msg[1024];
    InputOutput::ScreenManager *S = InputOutput::ScreenManager::singleton();

    sprintf(msg,
            "Loading the tool \"%s\" from the file \"%s\"...\n",
            name(),
            path());
    S->addMessageF(L_INFO, msg);

    if(compile(_entry_point)){
        return true;
    }

    if(variables(_entry_point)){
        return true;
    }

    if(setVariables()){
        return true;
    }

    if(computeGlobalWorkSize()){
        return true;
    }

    return false;
}

bool Kernel::_execute()
{
    cl_int err_code;
    char msg[1024];
    InputOutput::ScreenManager *S = InputOutput::ScreenManager::singleton();
    CalcServer *C = CalcServer::singleton();

    if(setVariables()){
        return true;
    }

    err_code = clEnqueueNDRangeKernel(C->command_queue(),
                                      _kernel,
                                      1,
                                      NULL,
                                      &_global_work_size,
                                      &_work_group_size,
                                      0,
                                      NULL,
                                      NULL);
    if(err_code != CL_SUCCESS){
        sprintf(msg, "Failure launching the tool \"%s\".\n", name());
        S->addMessageF(L_ERROR, msg);
        S->printOpenCLError(err_code);
        return true;
    }
    return false;
}

void Kernel::path(const char* kernel_path)
{
    if(_path) delete[] _path; _path=NULL;
    _path = new char[strlen(kernel_path) + 1];
    strcpy(_path, kernel_path);
}

bool Kernel::compile(const char* entry_point,
                     const char* add_flags,
                     const char* header)
{
    unsigned int i;
    cl_program program;
    cl_kernel kernel;
    char* source = NULL;
    char* flags = NULL;
    size_t source_length = 0;
    cl_int err_code = CL_SUCCESS;
    size_t work_group_size = 0;
    char msg[1024];
    InputOutput::ScreenManager *S = InputOutput::ScreenManager::singleton();
    CalcServer *C = CalcServer::singleton();

    // Allocate the required memory for the source code
    source_length = readFile(NULL, path());
    if(!source_length){
        return true;
    }
    source = new char[source_length + 1];
    if(!source){
        S->addMessageF(L_ERROR, "Failure allocating memory for the source code.\n");
        return true;
    }
    // Read the file
    source_length = readFile(source, path());
    if(!source_length){
        delete[] source; source=NULL;
        return true;
    }
    // Append the header on top of the source code
    if(header){
        char *backup = source;
        source_length += strlen(header) * sizeof(char);
        source = new char[source_length + 1];
        if(!source) {
            S->addMessageF(L_ERROR, "Failure allocate memory to append the header.\n");
            return true;
        }
        strcpy(source, header);
        strcat(source, backup);
        delete[] backup; backup=NULL;
    }

    // Setup the default flags
    flags = new char[1024];
    #ifdef AQUA_DEBUG
        strcpy(flags, "-DDEBUG ");
    #else
        strcpy(flags, "-DNDEBUG ");
    #endif
    strcat(flags, "-I");
    const char *folder = getFolderFromFilePath(path());
    strcat(flags, folder);
    if(strcmp(C->base_path(), "")){
        strcat(flags, " -I");
        strcat(flags, C->base_path());
    }

    strcat(flags, " -cl-mad-enable -cl-fast-relaxed-math ");
    #ifdef HAVE_3D
        strcat(flags, " -DHAVE_3D ");
    #else
        strcat(flags, " -DHAVE_2D ");
    #endif
    // Setup the user registered flags
    for(i = 0; i < C->definitions().size(); i++){
        strcat(flags, C->definitions().at(i));
        strcat(flags, " ");
    }
    // Add the additionally specified flags
    if(add_flags)
        strcat(flags, add_flags);

    // Try to compile without using local memory
    S->addMessageF(L_INFO, "Compiling without local memory... ");
    program = clCreateProgramWithSource(C->context(),
                                        1,
                                        (const char **)&source,
                                        &source_length,
                                        &err_code);
    if(err_code != CL_SUCCESS) {
        S->addMessage(L_DEBUG, "FAIL\n");
        S->addMessageF(L_ERROR, "Failure creating the OpenCL program.\n");
        S->printOpenCLError(err_code);
        delete[] flags; flags=NULL;
        delete[] source; source=NULL;
        return true;
    }
    err_code = clBuildProgram(program, 0, NULL, flags, NULL, NULL);
    if(err_code != CL_SUCCESS) {
        S->addMessage(L_DEBUG, "FAIL\n");
        S->printOpenCLError(err_code);
        S->addMessage(L_ERROR, "--- Build log ---------------------------------\n");
        size_t log_size = 0;
        clGetProgramBuildInfo(program,
                              C->device(),
                              CL_PROGRAM_BUILD_LOG,
                              0,
                              NULL,
                              &log_size);
        char *log = (char*)malloc(log_size + sizeof(char));
        if(!log){
            sprintf(msg,
                    "Failure allocating %lu bytes for the building log\n",
                    log_size);
            S->addMessage(L_ERROR, msg);
            S->addMessage(L_ERROR, "--------------------------------- Build log ---\n");
            return NULL;
        }
        strcpy(log, "");
        clGetProgramBuildInfo(program,
                              C->device(),
                              CL_PROGRAM_BUILD_LOG,
                              log_size,
                              log,
                              NULL);
        strcat(log, "\n");
        S->addMessage(L_DEBUG, log);
        S->addMessage(L_ERROR, "--------------------------------- Build log ---\n");
        free(log); log=NULL;
        delete[] flags; flags=NULL;
        delete[] source; source=NULL;
        clReleaseProgram(program);
        return true;
    }
    kernel = clCreateKernel(program, entry_point, &err_code);
    clReleaseProgram(program);
    if(err_code != CL_SUCCESS) {
        S->addMessage(L_DEBUG, "FAIL\n");
        sprintf(msg, "Failure creating the kernel \"%s\"\n", entry_point);
        S->addMessageF(L_ERROR, msg);
        S->printOpenCLError(err_code);
        delete[] flags; flags=NULL;
        return true;
    }

    // Get the work group size
    err_code = clGetKernelWorkGroupInfo(kernel,
                                        C->device(),
                                        CL_KERNEL_WORK_GROUP_SIZE,
                                        sizeof(size_t),
                                        &work_group_size,
                                        NULL);
    if(err_code != CL_SUCCESS) {
        S->addMessage(L_DEBUG, "FAIL\n");
        S->addMessageF(L_ERROR, "Failure querying the work group size.\n");
        S->printOpenCLError(err_code);
        clReleaseKernel(kernel);
        delete[] flags; flags=NULL;
        return true;
    }
    S->addMessage(L_DEBUG, "OK\n");

    _kernel = kernel;
    _work_group_size = work_group_size;

    // Try to compile with local memory
    S->addMessageF(L_INFO, "Compiling with local memory... ");
    program = clCreateProgramWithSource(C->context(),
                                        1,
                                        (const char **)&source,
                                        &source_length,
                                        &err_code);
    delete[] source; source=NULL;
    if(err_code != CL_SUCCESS) {
        S->addMessage(L_DEBUG, "FAIL\n");
        S->addMessageF(L_ERROR, "Failure creating the OpenCL program.\n");
        S->printOpenCLError(err_code);
        delete[] flags; flags=NULL;
        S->addMessageF(L_INFO, "Falling back to no local memory usage.\n");
        return false;
    }
    sprintf(flags + strlen(flags), " -DLOCAL_MEM_SIZE=%lu", work_group_size);
    err_code = clBuildProgram(program, 0, NULL, flags, NULL, NULL);
    delete[] flags; flags=NULL;
    if(err_code != CL_SUCCESS) {
        S->addMessage(L_DEBUG, "FAIL\n");
        S->printOpenCLError(err_code);
        S->addMessage(L_ERROR, "--- Build log ---------------------------------\n");
        size_t log_size;
        clGetProgramBuildInfo(program,
                              C->device(),
                              CL_PROGRAM_BUILD_LOG,
                              0,
                              NULL,
                              &log_size);
        char *log = (char*)malloc(log_size + sizeof(char));
        clGetProgramBuildInfo(program,
                              C->device(),
                              CL_PROGRAM_BUILD_LOG,
                              log_size,
                              log,
                              NULL);
        strcat(log, "\n");
        S->addMessage(L_DEBUG, log);
        S->addMessage(L_ERROR, "--------------------------------- Build log ---\n");
        free(log); log=NULL;
        clReleaseProgram(program);
        S->addMessageF(L_INFO, "Falling back to no local memory usage.\n");
        return false;
    }
    kernel = clCreateKernel(program, entry_point, &err_code);
    clReleaseProgram(program);
    if(err_code != CL_SUCCESS) {
        S->addMessage(L_DEBUG, "FAIL\n");
        sprintf(msg, "Failure creating the kernel \"%s\"\n", entry_point);
        S->addMessageF(L_ERROR, msg);
        S->printOpenCLError(err_code);
        S->addMessageF(L_INFO, "Falling back to no local memory usage.\n");
        return false;
    }
    cl_ulong used_local_mem;
    err_code = clGetKernelWorkGroupInfo(kernel,
                                        C->device(),
                                        CL_KERNEL_LOCAL_MEM_SIZE,
                                        sizeof(cl_ulong),
                                        &used_local_mem,
                                        NULL);
    if(err_code != CL_SUCCESS) {
        S->addMessage(L_DEBUG, "FAIL\n");
        S->addMessageF(L_ERROR, "Failure querying the used local memory.\n");
        S->printOpenCLError(err_code);
        S->addMessageF(L_INFO, "Falling back to no local memory usage.\n");
        return false;
    }
    cl_ulong available_local_mem;
    err_code = clGetDeviceInfo(C->device(),
                               CL_DEVICE_LOCAL_MEM_SIZE,
                               sizeof(cl_ulong),
                               &available_local_mem,
                               NULL);
    if(err_code != CL_SUCCESS) {
        S->addMessage(L_DEBUG, "FAIL\n");
        S->addMessageF(L_ERROR, "Failure querying the available local memory.\n");
        S->printOpenCLError(err_code);
        S->addMessageF(L_INFO, "Falling back to no local memory usage.\n");
        return false;
    }

    if(available_local_mem < used_local_mem){
        S->addMessage(L_DEBUG, "FAIL\n");
        S->addMessageF(L_ERROR, "Not enough available local memory.\n");
        S->printOpenCLError(err_code);
        S->addMessageF(L_INFO, "Falling back to no local memory usage.\n");
        return false;
    }
    S->addMessage(L_DEBUG, "OK\n");
    clReleaseKernel(_kernel);
    _kernel = kernel;

    return false;
}

/** @brief Main traverse method, which will parse all tokens except functions
 * declarations.
 * @param cursor the cursor whose child may be visited. All kinds of cursors can
 * be visited, including invalid cursors (which, by definition, have no
 * children).
 * @param parent the visitor function that will be invoked for each child of
 * parent.
 * @param client_data pointer data supplied by the client, which will be passed
 * to the visitor each time it is invoked.
 * @return CXChildVisit_Continue if a function declaration is found,
 * CXChildVisit_Recurse otherwise.
 */
CXChildVisitResult cursorVisitor(CXCursor cursor,
                                 CXCursor parent,
                                 CXClientData client_data);

/** Method traverse method, which will parse the input arguments.
 * @param cursor the cursor whose child may be visited. All kinds of cursors can
 * be visited, including invalid cursors (which, by definition, have no
 * children).
 * @param parent the visitor function that will be invoked for each child of
 * parent.
 * @param client_data pointer data supplied by the client, which will be passed
 * to the visitor each time it is invoked.
 * @return CXChildVisit_Continue.
 */
CXChildVisitResult functionDeclVisitor(CXCursor cursor,
                                       CXCursor parent,
                                       CXClientData client_data);

/** @struct clientData
 * @brief Data structure to store the variables requested and a flag
 * to know if the entry point has been found.
 */
struct clientData{
    /// Entry point
    const char* entry_point;
    /// Number of instances of the entry point found.
    unsigned int entry_points;
    /// List of required variables
    std::deque<char*> *var_names;
};

bool Kernel::variables(const char* entry_point)
{
    char msg[1024];
    InputOutput::ScreenManager *S = InputOutput::ScreenManager::singleton();

    CXIndex index = clang_createIndex(0, 0);
    if(index == 0){
        S->addMessageF(L_ERROR, "Failure creating parser index.\n");
        return true;
    }

    int argc = 2;
    const char* argv[2] = {"Kernel", path()};
    CXTranslationUnit translation_unit = clang_parseTranslationUnit(
        index,
        0,
        argv,
        argc,
        0,
        0,
        CXTranslationUnit_None);
    if(translation_unit == 0){
        S->addMessageF(L_ERROR, "Failure parsing the source code.\n");
        return true;
    }

    CXCursor root_cursor = clang_getTranslationUnitCursor(translation_unit);
    struct clientData client_data;
    client_data.entry_point = entry_point;
    client_data.entry_points = 0;
    client_data.var_names = &_var_names;
    clang_visitChildren(root_cursor, *cursorVisitor, &client_data);
    if(client_data.entry_points == 0){
        sprintf(msg, "The entry point \"%s\" cannot be found.\n", entry_point);
        S->addMessageF(L_ERROR, msg);
        return true;
    }
    if(client_data.entry_points != 1){
        sprintf(msg,
                "Entry point \"%s\" found %u times.\n",
                entry_point,
                client_data.entry_points);
        S->addMessageF(L_ERROR, msg);
        return true;
    }

    unsigned int i;
    for(i = 0; i < _var_names.size(); i++){
        _var_values.push_back(NULL);
    }

    clang_disposeTranslationUnit(translation_unit);
    clang_disposeIndex(index);
    return false;
}

CXChildVisitResult cursorVisitor(CXCursor cursor,
                                 CXCursor parent,
                                 CXClientData client_data)
{
    struct clientData *data = (struct clientData *)client_data;
    CXCursorKind kind = clang_getCursorKind(cursor);
    CXString name = clang_getCursorSpelling(cursor);
    if (kind == CXCursor_FunctionDecl ||
        kind == CXCursor_ObjCInstanceMethodDecl)
    {
        if(!strcmp(clang_getCString(name), data->entry_point)){
            data->entry_points++;
            clang_visitChildren(cursor, *functionDeclVisitor, client_data);
        }
        return CXChildVisit_Continue;
    }
    return CXChildVisit_Recurse;
}

CXChildVisitResult functionDeclVisitor(CXCursor cursor,
                                       CXCursor parent,
                                       CXClientData client_data)
{
    struct clientData *data = (struct clientData *)client_data;
    CXCursorKind kind = clang_getCursorKind(cursor);
    if (kind == CXCursor_ParmDecl){
        CXString name = clang_getCursorSpelling(cursor);
        std::deque<char*> *var_names = data->var_names;
        char *var_name = new char[strlen(clang_getCString(name)) + 1];
        strcpy(var_name, clang_getCString(name));
        var_names->push_back(var_name);
    }
    return CXChildVisit_Continue;
}

bool Kernel::setVariables()
{
    unsigned int i;
    char msg[1024];
    cl_int err_code;
    InputOutput::ScreenManager *S = InputOutput::ScreenManager::singleton();
    CalcServer *C = CalcServer::singleton();
    InputOutput::Variables *vars = C->variables();

    for(i = 0; i < _var_names.size(); i++){
        if(!vars->get(_var_names.at(i))){
            sprintf(msg,
                    "The tool \"%s\" requires the undeclared variable \"%s\".\n",
                    name(),
                    _var_names.at(i));
            S->addMessageF(L_ERROR, msg);
            return true;
        }
        InputOutput::Variable *var = vars->get(_var_names.at(i));
        if(_var_values.at(i) == NULL){
            _var_values.at(i) = malloc(var->typesize());
        }
        else if(!memcmp(_var_values.at(i), var->get(), var->typesize())){
            // The variable still being valid
            continue;
        }

        // Update the variable
        InputOutput::ScreenManager *S = InputOutput::ScreenManager::singleton();
        err_code = clSetKernelArg(_kernel, i, var->typesize(), var->get());
        if(err_code != CL_SUCCESS) {
            sprintf(msg,
                    "Failure setting the variable \"%s\" (id=%u) to the tool \"%s\".\n",
                    _var_names.at(i),
                    i,
                    name());
            S->addMessageF(L_ERROR, msg);
            S->printOpenCLError(err_code);
            return true;
        }
        memcpy(_var_values.at(i), var->get(), var->typesize());
    }
    return false;
}

bool Kernel::computeGlobalWorkSize()
{
    unsigned int N;
    char msg[1024];
    InputOutput::ScreenManager *S = InputOutput::ScreenManager::singleton();
    CalcServer *C = CalcServer::singleton();
    if(!_work_group_size){
        S->addMessageF(L_ERROR, "Work group size must be greater than 0.\n");
        return true;
    }
    InputOutput::Variables *vars = C->variables();
    try {
        vars->solve("unsigned int", _n, &N);
    } catch(...) {
        S->addMessageF(L_ERROR, "Failure evaluating the number of threads.\n");
        return true;
    }

    _global_work_size = (size_t)roundUp(N, (unsigned int)_work_group_size);

    return false;
}

}}  // namespace
