#ifndef __SLIB_UTIL_DISTANCE_H__
#define __SLIB_UTIL_DISTANCE_H__

#include "../common/types.h"
#include <math.h>

namespace slib {
  namespace util {

    template <class T> T EuclideanDistance(const T* v, const T* u, const int32& N) {
      T sum = 0;
      for (int32 i = 0; i < N; i++) {
	T d = (v[i] - u[i]);
	sum += (d*d);
      }

      return sqrt(sum);
    }

    double HaversineDistance(const LatLon& u, const LatLon& v, const double& radius = 6373.0) {
      const double lat1 = u.lat;
      const double lon1 = u.lon;
      const double lat2 = v.lat;
      const double lon2 = v.lon;
      
      const double dlon = lon2 - lon1;
      const double dlat = lat2 - lat1;
      const double sdlat = sin(dlat / 2.0);
      const double sdlon = sin(dlon / 2.0);

      const double a = sdlat * sdlat + cos(lat1) * cos(lat2) * sdlon * sdlon;
      const double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
      return (radius * c);
    }

  }  // namespace util
}  // namespace slib

#endif
