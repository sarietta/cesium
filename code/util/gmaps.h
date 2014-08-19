#ifndef __SLIB_UTIL_GOOGLE_MAPS_H__
#define __SLIB_UTIL_GOOGLE_MAPS_H__

#define DEGREE_SYMBOL "\u00B0"

#include <CImg.h>
#include <common/types.h>
#include <string>
#include <vector>

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
      
      static FloatImage DrawPoints(const LatLon& southwest, const LatLon& northeast,
				   const std::vector<LatLon>& locations, 
				   const FloatImage& map_image, const int& zoom,
				   const std::string& point_color = "336699", const int& point_size = -1);
    private:
      static const double _tileSize;
      static const Point2D _pixelOrigin;
      static const double _pixelsPerLonDegree;
      static const double _pixelsPerLonRadian;
    };

    class GoogleMapsConverter {
    public:
      GoogleMapsConverter(const int& zoom, const LatLon& southwest, const LatLon& northeast);

      // Converts a latitude/longitude position to an (x,y) position
      // in the map. This is obviously dependent on the map size,
      // which is implicitly set via the constructor's zoom parameter.
      Point2D ConvertLocationToMapPoint(const LatLon& location);

    private:
      int _num_tiles;
      double _map_min_x;
      double _map_min_y;
    };

  }  // namespace util
}  // namespace slib

#endif
