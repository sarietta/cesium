#ifndef __SLIB_UTIL_GOOGLE_MAPS_H__
#define __SLIB_UTIL_GOOGLE_MAPS_H__

#include <common/types.h>

namespace slib {
  namespace util {

    class GoogleMaps {
    public:
      static Point2D ConvertFromLatLonToPoint(const LatLon& latlon);
      static LatLon ConvertFromPointToLatLon(const Point2D& point);

      static Point2D ConvertFromLatLonToTile(const LatLon& latlon, const int32& zoom);
      static LatLon ConvertFromTileToLatLon(const Point2D& tileCoordinate, const int32& zoom);

      static LatLonBounds GetMapBounds(const LatLon& southwest, const LatLon& northeast, const int32& zoom);

      static inline double GetTileSize() {
	return GoogleMaps::_tileSize;
      }
    private:
      static const double _tileSize;
      static const Point2D _pixelOrigin;
      static const double _pixelsPerLonDegree;
      static const double _pixelsPerLonRadian;
    };

  }  // namespace util
}  // namespace slib

#endif
