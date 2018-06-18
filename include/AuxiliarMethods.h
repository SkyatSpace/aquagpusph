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
 * @brief Set of auxiliar functions.
 */

#ifndef AUXILIARMETHODS_H_INCLUDED
#define AUXILIARMETHODS_H_INCLUDED

#include <sphPrerequisites.h>

#include <string>

namespace Aqua{

/// Returns if a key press event has been registered.
/**
 * @return 0 if no keys have been pressed, 1 otherwise.
 */
int isKeyPressed();

/** @brief Check if a string ends with an specific suffix
 * 
 * @param str String to be checked
 * @param suffix Suffix to be looked for
 * @return true if #str ends with #suffix, false otherwise.
 */
bool hasSuffix(const std::string &str, const std::string &suffix);

/** @brief Replace all substring occurrences by another substring
 * 
 * @param str String to be modified
 * @param search Substring to be replaced
 * @param replace Replacement substring
 */
void replaceAll(std::string &str,
                       const std::string &search,
                       const std::string &replace);

/** @brief Replace all substring occurrences by another substring
 * 
 * @param str String to be modified
 * @param search Substring to be replaced
 * @param replace Replacement substring
 * @return Modified string
 */
std::string replaceAllCopy(std::string str,
                                  std::string search,
                                  std::string replace);

/** @brief Remove all the blank spaces (including line breaks, tabulators...)
 * string prefix.
 * 
 * @param s String to become trimmed
 */
void ltrim(std::string &s);

/** @brief Remove all the blank spaces (including line breaks, tabulators...)
 * string suffix.
 * 
 * @param s String to become trimmed
 */
void rtrim(std::string &s);

/** @brief Remove all the blank spaces (including line breaks, tabulators...)
 * string prefix and suffix.
 * 
 * @param s String to become trimmed
 */
void trim(std::string &s);

/** @brief Remove all the blank spaces (including line breaks, tabulators...)
 * string suffix.
 * 
 * @param s String to become trimmed
 * @return Trimmed string
 */
std::string ltrimCopy(std::string s);

/** @brief Remove all the blank spaces (including line breaks, tabulators...)
 * string suffix.
 * 
 * @param s String to become trimmed
 * @return Trimmed string
 */
std::string rtrimCopy(std::string s);

/** @brief Remove all the blank spaces (including line breaks, tabulators...)
 * string prefix and suffix.
 * 
 * @param s String to become trimmed
 * @return Trimmed string
 */
std::string trimCopy(std::string s);
    
/// Next number which is power of 2.
/** Compute a value which, being power of two, is greater or equal than
 * @paramname{x}.
 *
 * @param x Number to round up to a power of two.
 * @return Next value which is power of two (it can be the same input value
 * @paramname{x}).
 */
unsigned int nextPowerOf2(unsigned int x);

/// Check if a number is power of 2.
/** Compute if a value is power of 2.
 *
 * @param x Value to test.
 * @return 1 if it is a power of two, 0 otherwise.
 */
unsigned int isPowerOf2(unsigned int x);

/// Rounded up value which is divisible by @paramname{divisor}.
/** Compute a value, which being divisible by @paramname{divisor}, is greater
 * or equal than @paramname{x}.
 *
 * @param x Number to round up.
 * @param divisor Divisor.
 * @return Rounded up number.
 */
unsigned int roundUp(unsigned int x, unsigned int divisor);

/** @brief Round an float value to an integer one.
 * @param n Number to round.
 * @return The closest integer to @paramname{n}.
 */
int round(float n);

/// Load an OpenCL kernel from a file.
/**
 * @param kernel The output kernel identifier.
 * @param program The output program identifier.
 * @param context Context from where the program should be loaded.
 * @param device Device where the kernel should be computed.
 * @param path Path of the kernel source code file.
 * @param entry_point Method into the kernel to be called.
 * @param flags Compilation flags.
 * @param header Source code to be inserted at the start of the readed source
 * code.
 * @return A valid work group size to compute the kernel, 0 if errors happened.
 * @note Several compilation flags will be automatically added:
 *  -# -IKERNEL_SRC_PATH (KERNEL_SRC_PATH will be replaced by the path patern @paramname{path})
 *  -# -cl-mad-enable
 *  -# -cl-fast-relaxed-math
 *  -# -DDEBUG (if AQUA_DEBUG has been defined)
 *  -# -DNDEBUG (if AQUA_DEBUG has not been defined)
 *  -# -DHAVE_3D (if HAVE_3D has been defined)
 *  -# -DHAVE_2D (if HAVE_3D has not been defined)
 *  -# -Dh=KERNEL_LENGTH (KERNEL_LENGTH will be replaced by Aqua::InputOutput::ProblemSetup::sphSPHParameters::h)
 *  -# -D__BOUNDARY__=BOUNDARY_TYPE (BOUNDARY_TYPE will be replaced by Aqua::InputOutput::ProblemSetup::sphSPHParameters::boundary_type)
 *  -# -D__FREE_SLIP__ (if Aqua::InputOutput::ProblemSetup::sphSPHParameters::slip_condition is 1)
 *  -# -D__NO_SLIP__ (if Aqua::InputOutput::ProblemSetup::sphSPHParameters::slip_condition is 0)
 */
size_t loadKernelFromFile(cl_kernel* kernel, cl_program* program,
                          cl_context context, cl_device_id device,
                          const char* path, const char* entry_point,
                          const char* flags, const char* header=NULL);

/// Gets the folder path which contains the file @paramname{file_path}.
/**
 * @param file_path The file path.
 * @return The folder.
 */
const char* getFolderFromFilePath(const char* file_path);

/// Gets the file name of the path @paramname{file_path}.
/**
 * @param file_path The file path.
 * @return The file name.
 */
const char* getFileNameFromFilePath(const char* file_path);

/// Gets the file extension.
/** Get the file extension from the full file path @paramname{file_path}.
 *
 * @param file_path The file path.
 * @return Extension of the file.
 */
const char* getExtensionFromFilePath(const char* file_path);

/// Check if the file @paramname{file_path} exist on the system.
/**
 * @param file_name The file path.
 * @return 0 if the file can not be found in the system, 1 otherwise.
 */
int isFile(const char* file_name);

/// Load a file returning it as a characters array.
/**
 * @param source_code Readed file content.
 * @param file_name The file path.
 * @return Length of the source code array.
 * @note If @paramname{source_code} is a NULL pointer, just the length of the source code
 * will be returned without reading the file contents.
 * @warning Be sure that @paramname{source_code} has allocated memory enough,
 * otherwise a segmentation fault will be received.
 */
size_t readFile(char* source_code, const char* file_name);

/// Send an argument to an OpenCL kernel.
/**
 * @param kernel Kernel that must receive the argument.
 * @param index Index of the argument into the kernel @paramname{kernel}.
 * @param size Memory size of the argument.
 * @param ptr Pointer to the argument.
 * @return 0 if the argument has been successfully sent, 1 otherwise.
 */
int sendArgument(cl_kernel kernel, int index, size_t size, void* ptr);

/// Compute the maximum local work size allowed by a device.
/**
 * @param n Amount of data to operate in kernel (aiming threads to launch).
 * @param queue Command queue.
 * @return The local work size, 0 if it is not possible to find a valid value.
 */
size_t getLocalWorkSize(cl_uint n, cl_command_queue queue);

/// Compute the global work size needed to compute @paramname{n} threads.
/**
 * @param n Amount of data to operate in kernel (aiming threads to launch).
 * @param local_work_size The local work size which will be applied.
 * @return The required global work size.
 *
 * @see roundUp()
 */
size_t getGlobalWorkSize(cl_uint n, size_t local_work_size);

/// Gets the minimum of two values.
/**
 * @param a First value.
 * @param b Second value.
 * @return Minimum value.
 */
template <typename T> inline T min(T a, T b){return (a>b)?b:a;}

/// Gets the maximum of two values.
/**
 * @param a First value.
 * @param b Second value.
 * @return Maximum value.
 */
template <typename T> inline T max(T a, T b){return (a<b)?b:a;}

/// Clamps a value between the bounds.
/**
 * @param x Value to adjust into the bounds.
 * @param a Minimum value.
 * @param b Maximum value.
 * @return Clamped value.
 */
inline float clamp(float x, float a, float b){return x < a ? a : (x > b ? b : x);}

/// Return a null vector.
/**
 * @return zeroes vector.
 */
vec Vzero();

/// Return the x direction unit vector.
/**
 * @return x direction unit vector.
 */
vec Vx();

/// Return the y direction unit vector.
/**
 * @return y direction unit vector.
 */
vec Vy();

#ifdef HAVE_3D
/// Return the z direction unit vector.
/**
 * @remarks Only available in the 3D version.
 * @return z direction unit vector.
 */
vec Vz();
#endif

/// Multiply a vector by a scalar.
/**
 * @param n Scalar to operate.
 * @param v Vector to operate.
 * @return @paramname{n} \f$ \cdot \f$ @paramname{v} Resulting vector.
 */
vec mult(float n, vec v);

/// Adding operation.
/**
 * @param a Vector to operate.
 * @param b Vector to operate.
 * @return @paramname{a} + @paramname{b}.
 */
vec add(vec a, vec b);

/// Subtracting operation.
/**
 * @param a Vector to operate.
 * @param b Vector to operate.
 * @return @paramname{a} - @paramname{b}.
 */
vec sub(vec a, vec b);

/// Inner product.
/**
 * @param a Vector to operate.
 * @param b Vector to operate.
 * @return @paramname{a} \f$ \cdot \f$ @paramname{b} scalar product value.
 */
float dot(vec a, vec b);

/// Compute the vector length.
/**
 * @param v Input vector.
 * @return \f$ \vert \f$ @paramname{v} \f$ \vert \f$ vector length.
 */
float length(vec v);

/// Compute a normalized vector copy (such that length() will return 1.0.
/**
 * @param v Vector to normalize.
 * @return Normalized copy of the vector.
 *
 * @see length()
 */
vec normalize(vec v);

#ifdef HAVE_3D
/// Cross product.
/**
 * @remarks Only available in the 3D version.
 * @param a Vector to operate.
 * @param b Vector to operate.
 * @return @paramname{a} \f$ \times \f$ @paramname{b} crossed product vector.
 */
vec cross(vec a, vec b);
#endif

/// Get the number of digits of an integer decimal text representation.
/**
 * @param number Number from which the number of digits should be computed.
 * @return Number of digits.
 */
unsigned int numberOfDigits(unsigned int number);

}   // namespace

#endif // AUXILIARMETHODS_H_INCLUDED
