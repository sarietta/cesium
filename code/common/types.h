#ifndef __SLIB_COMMON_TYPES_H__
#define __SLIB_COMMON_TYPES_H__

#include <iostream>
#include <stdint.h>

// No namespace.

typedef int8_t int8;
typedef uint8_t uint8;
typedef int16_t int16;
typedef uint16_t uint16;
typedef int32_t int32;
typedef uint32_t uint32;
#ifndef SLIB_NO_DEFINE_64BIT
typedef int64_t int64;
typedef uint64_t uint64;
#endif

#ifdef cimg_version
typedef cimg_library::CImg<float> FloatImage;
typedef cimg_library::CImg<double> DoubleImage;
typedef cimg_library::CImg<uint8> UInt8Image;
#endif

#define FloatMatrix Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>

template <typename T>
struct Pair {
  T x;
  T y;
  
  Pair() {}  
  Pair(T x_, T y_) { x = x_; y = y_;  }
  
  T& first() { return x; }
  
  T& second() { return y; }

  const T& first() const { return x; }
  
  const T& second() const { return y; }
  
  Pair<T>& operator=(const Pair<T>& pair) {
    x = pair.x;
    y = pair.y;
    return *this;
  }
};

template <typename T>
struct Triplet {
  T x;
  T y;
  T z;
  
  Triplet() {}
  
  Triplet(T x_, T y_, T z_) { x = x_; y = y_; z = z_; }
  
  T& first() { return x; }
  
  T& second() { return y; }

  T& third() { return z; }

  const T& first() const { return x; }
  
  const T& second() const { return y; }
  
  const T& third() const { return z; }
  
  Triplet<T>& operator=(const Triplet<T>& triplet) {
    x = triplet.x;
    y = triplet.y;
    z = triplet.z;
    return *this;
  }
};

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
  
  // Should be a specialization of Pair.
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
