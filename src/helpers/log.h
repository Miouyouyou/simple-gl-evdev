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

#ifndef MYY_LOG_H
#define MYY_LOG_H 1

#if defined(DEBUG)
#if defined(__ANDROID__)

#include <android/log.h>
#define LOG(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-insanity", __VA_ARGS__))
#define LOG_ERRNO(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "native-insanity", "Error : %s\n", strerror(errno))); ((void)__android_log_print(ANDROID_LOG_ERROR, "native-insanity", __VA_ARGS__)

#else

#include <stdio.h>
#include <string.h>
#include <errno.h>
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#define LOG_ERRNO(...)  fprintf(stderr, "Error : %s\n", strerror(errno)); fprintf(stderr, __VA_ARGS__)

#endif
#else // DEBUG
#define LOG(...)
#define LOG_ERRNO(...)
#endif // DEBUG
#endif
