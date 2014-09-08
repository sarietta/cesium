#ifndef __SLIB_COMMON_TYPES_H__
#define __SLIB_COMMON_TYPES_H__

#include <assert.h>
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <string>
#include <vector>

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

#define FloatImage cimg_library::CImg<float>
#define DoubleImage cimg_library::CImg<double> 
#define UInt8Image cimg_library::CImg<uint8>

#define FloatMatrix Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
#define SparseFloatMatrix Eigen::SparseMatrix<float, Eigen::RowMajor>
#define DoubleMatrix Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>
#define SparseDoubleMatrix Eigen::SparseMatrix<double, Eigen::RowMajor>

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

  bool operator==(const Pair<T>& other) const {
    return (x == other.x && y == other.y);
  }

  bool operator!=(const Pair<T>& other) const {
    return !(*this == other);
  }

  friend std::ostream& operator<<(std::ostream& out, const Pair<T>& point) {
    out << "(" << point.x << ", " << point.y << ")";
    return out;
  }

  /** 
      Casting operations.
   */
  // To templated Pair.
  template <typename U>
  operator Pair<U>() const {
    return Pair<U>(static_cast<U>(x), static_cast<U>(y));
  }

  // To templated vector.
  template <typename U>
  operator std::vector<U>() const {
    std::vector<U> values(2);
    values[0] = static_cast<U>(x);
    values[1] = static_cast<U>(y);
    return values;
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

    LatLon(const std::string& latlon) {      
      lat = atof(latlon.c_str());
      lon = atof(strstr(latlon.c_str(), ",") + 1);
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
