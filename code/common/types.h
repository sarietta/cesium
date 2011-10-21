#ifndef __SLIB_COMMON_TYPES_H__
#define __SLIB_COMMON_TYPES_H__

// No namespace.

typedef signed char int8;
typedef unsigned char uint8;
typedef signed int int16;
typedef unsigned int uint16;
typedef signed long int int32;
typedef unsigned long int uint32;
typedef signed long long int int64;
typedef unsigned long long int uint64;

#ifdef cimg_version
typedef cimg_library::CImg<float> FloatImage;
#endif

#endif
