/*
  Copyright (c) 2017 Miouyouyou <Myy>

  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files
  (the "Software"), to deal in the Software without restriction,
  including without limitation the rights to use, copy, modify, merge,
  publish, distribute, sublicense, and/or sell copies of the Software,
  and to permit persons to whom the Software is furnished to do so,
  subject to the following conditions:

  The above copyright notice and this permission notice shall be
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
  CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
  TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
  SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef MYY_INCLUDE_FILE_HELPERS
#define MYY_INCLUDE_FILE_HELPERS 1

#include <stdint.h>

/**
 * Read `size` bytes from the file at `pathname` into `buffer`
 * 
 * ASSUMPTIONS :
 * - `buffer` can contain `size` bytes
 * 
 * @params
 * @param pathname The file's name and path (e.g. /tmp/file.txt)
 * @param buffer   The buffer to store the file bytes in
 * @param size     How many bytes to copy from the file into the buffer
 * 
 * @return
 * - The number of bytes read on success. (Can be 0 if buffer == 0)
 * - -1 on failure
 */
int fh_ReadFileToBuffer
(char const * __restrict const pathname,
 void * __restrict const buffer,
 const unsigned int size);

/**
 * Read `size` bytes from the file at `pathname` into `buffer` and
 * add a '\0' suffix
 * 
 * ASSUMPTIONS :
 * - `buffer` can contain `size` bytes + 1
 * 
 * @params
 * @param pathname The file's name and path (e.g. /tmp/file.txt)
 * @param buffer   The buffer to store the file bytes in
 * @param size     How many bytes to copy from the file into the buffer
 * 
 * @returns
 * - The number of bytes read on success. (Can be 0 if buffer == 0)
 * - -1 on failure
 */
int fh_ReadFileToStringBuffer
(char const * __restrict const pathname,
 void * __restrict const buffer,
 const unsigned int size);

/**
 * Read `size` bytes from the file at `pathname` into `buffer`,
 * starting from `offset`
 * 
 * ASSUMPTIONS :
 * - `buffer` can contain `size` bytes + 1
 * 
 * @params
 * @param pathname The file's name and path (e.g. /tmp/file.txt)
 * @param buffer   The buffer to store the file bytes in
 * @param size     How many bytes to copy from the file into the buffer
 * @param offset   The absolute offset to start reading from.
 * 
 * @returns
 * - The number of bytes read on success. (Can be 0 if buffer == 0)
 * - -1 on failure
 */
int fh_ReadBytesFromFile
(char const * __restrict const pathname,
 void * __restrict const buffer,
 unsigned int const size,
 unsigned int const offset);

/** Copy the whole file contents into buffer
*
* ASSUMPTIONS :
* - 'buffer' can contain the whole file
*
* CAUTION :
* - If there's not enough space in buffer, this procedure will most
*   likely generate a SIGSEGV or, worse, corrupt the memory.
* - This function returns either 0 or 1. Not -1 !
*
* PARAMS :
* @param pathname The file's path in the Assets archive.
* @param buffer   The buffer to store the file's content in.
*
* @returns
* - 1 if the whole copy was done successfully
* - 0 otherwise
*/
unsigned int fh_WholeFileToBuffer
(char const * __restrict const pathname,
 void * __restrict const buffer);

struct myy_fh_map_handle {
	unsigned int ok;
	void const * address;
	int length;
};

/**
 * Map an entire file in memory (mmap).
 *
 * PARAMS :
 * @param pathname The file's path in the Assets archive.
 *
 * @returns
 * Three values packed in a struct myy_fh_map_handle data structure.
 * This structure contains the following fields :
 * - ok : The mapping was done successfully
 * - address : The memory address where the file has been mapped, if
 *             the mapping was performed successfully
 * - handle  : An opaque handle used by fh_UnmapFileFromMemory
 */
struct myy_fh_map_handle fh_MapFileToMemory
(char const * __restrict const pathname);

/**
 * Unmap a previously mapped file from the system's memory
 *
 * @brief fh_UnmapFileFromMemory
 *
 * PARAMS:
 * @param handle The data structure returned by fh_MapFileToMemory
 */
void fh_UnmapFileFromMemory
(struct myy_fh_map_handle const handle);

#endif
