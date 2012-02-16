#ifndef __SLIB_COMMON_TYPES_H__
#define __SLIB_COMMON_TYPES_H__

// No namespace.

typedef signed char int8;
typedef unsigned char uint8;
typedef signed int int16;
typedef unsigned int uint16;
typedef signed long int int32;
typedef unsigned long int uint32;
#ifndef SLIB_NO_DEFINE_64BIT
typedef signed long long int int64;
typedef unsigned long long int uint64;
#endif

#ifdef cimg_version
typedef cimg_library::CImg<float> FloatImage;
typedef cimg_library::CImg<uint8> UInt8Image;
#endif

#endif
