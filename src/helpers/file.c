/*
	Copyright (c) 2017 Miouyouyou <Myy>
  
  Permission is hereby granted, free of charge, to any person obtaining
  a copy of this software and associated documentation files 
  (the "Software"), to deal in the Software without restriction, 
  including	without limitation the rights to use, copy, modify, merge, 
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

#include <helpers/file.h>
#include <helpers/log.h>

/* read - close */
#include <unistd.h>

/* open */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/* mmap */
#include <sys/mman.h>

unsigned int fh_WholeFileToBuffer
(const char * __restrict const pathname,
 void * __restrict const buffer) {

	ssize_t bytes_read;
	off_t file_size;
	struct stat fd_stats;

	int fd = open(pathname, O_RDONLY);

	if (fd != -1) {

		fstat(fd, &fd_stats);

		file_size = fd_stats.st_size;

		bytes_read = read(fd, buffer, file_size);
		close(fd);

		return bytes_read == file_size;

	}
	else {
		LOG_ERRNO("Could not open %s\n", pathname);
		return 0;
	}
}

int fh_ReadFileToBuffer
(char const * __restrict const pathname,
 void * __restrict const buffer, 
 unsigned int const size) {

	int bytes_read = -1;
	int fd = open(pathname, O_RDONLY);
	if (fd != -1) {
		bytes_read = read(fd, buffer, size);
		close(fd);
	}
	else { LOG_ERRNO("Could not open %s\n", pathname); }
	return bytes_read;

}

int fh_ReadFileToStringBuffer
(char const * __restrict const pathname,
 void * __restrict const buffer, 
 unsigned int const size) {

	int bytes_read = fh_ReadFileToBuffer(pathname, buffer, size);
	if (bytes_read != -1)
		((uint8_t *) buffer)[bytes_read] = 0;

	return bytes_read;

}

int fh_ReadBytesFromFile
(char const * __restrict const pathname,
 void * __restrict const buffer,
 unsigned int const size,
 unsigned int const offset) 
{

	int bytes_read = -1;
	int fd = open(pathname, O_RDONLY);
	if (fd != -1) {
		lseek(fd, offset, SEEK_SET);
		bytes_read = read(fd, buffer, size);
		close(fd);
	}
	else { LOG_ERRNO("Could not open %s\n", pathname); }
	return bytes_read;

}

struct myy_fh_map_handle fh_MapFileToMemory
(char const * __restrict const pathname)
{
	LOG("[fh_MapFileToMemory]\n");
	LOG("  pathname : %s\n", pathname);
	int fd = open(pathname, O_RDONLY);
	void * mapped_address = MAP_FAILED;
	off_t file_size = 0;
	unsigned int ok = 0;

	if (fd != 1) {
		struct stat file_stats;
		fstat(fd, &file_stats);
		file_size = file_stats.st_size;
		LOG("  mmap(0, %lu, PROT_READ, MAP_PRIVATE, %d)\n",
		     file_size, fd);
		mapped_address =
		  mmap(0, file_stats.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
		ok = (mapped_address != MAP_FAILED);
		LOG("  mapped_address : %p - Ok ? %d\n", mapped_address, ok);
		close(fd);
	}

	if (ok == 0) {
		LOG_ERRNO("  Could not map file %s to memory\n", pathname);
	}

	struct myy_fh_map_handle handle = {
		.ok      = ok,
		.address = mapped_address,
		.length  = (int) file_size
	};
	return handle;
}

void fh_UnmapFileFromMemory
(struct myy_fh_map_handle const handle) {
	if (handle.ok) munmap(handle.address, handle.length);
}
