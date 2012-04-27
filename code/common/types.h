#ifndef __SLIB_COMMON_TYPES_H__
#define __SLIB_COMMON_TYPES_H__

#include <iostream>

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

namespace slib {
  struct LatLon {
    double lat;
    double lon;
    
    LatLon() {
      lat = 0.0;
      lon = 0.0;
    }

    LatLon(double lat_, double lon_) {
      lat = lat_;
      lon = lon_;
    }

    LatLon(const LatLon& latlon) {
      lat = latlon.lat;
      lon = latlon.lon;
    }
  };
  
  struct Point2D {
    double x;
    double y;

    Point2D() {
      x = 0;
      y = 0;
    }

    Point2D(double x_, double y_) {
      x = x_;
      y = y_;
    }

    friend std::ostream& operator<<(std::ostream& out, const Point2D& point) {
      out << "(" << point.x << ", " << point.y << ")";
      return out;
    }

    friend std::ostream& operator<<=(std::ostream& out, const Point2D& point) {
      out.write((char*) &point.x, sizeof(double));
      out.write((char*) &point.y, sizeof(double));
      return out;
    }

    friend std::istream& operator>>=(std::istream& in, Point2D& point) {
      in.read((char*) &point.x, sizeof(double));
      in.read((char*) &point.y, sizeof(double));
      return in;
    }
  };
  
  struct LatLonBounds {
    LatLon southwest;
    LatLon northeast;

    LatLonBounds(const LatLon& southwest_, const LatLon& northeast_) {
      southwest.lat = southwest_.lat;
      southwest.lon = southwest_.lon;

      northeast.lat = northeast_.lat;
      northeast.lon = northeast_.lon;
    }
  };  

}  // namespace slib

#endif
