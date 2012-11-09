#include "gmaps.h"

#include <common/types.h>
#include <math.h>
#include "mathutils.h"

namespace slib {
  namespace util {

    const double GoogleMaps::_tileSize = 256;
    const Point2D GoogleMaps::_pixelOrigin(_tileSize / 2.0, _tileSize / 2.0);
    const double GoogleMaps::_pixelsPerLonDegree = _tileSize / 360.0;
    const double GoogleMaps::_pixelsPerLonRadian = _tileSize / (2.0 * M_PI);

    
    LatLonBounds GoogleMaps::GetMapBounds(const LatLon& southwest, const LatLon& northeast, const int32& zoom) {
      Point2D southwest_tile = GoogleMaps::ConvertFromLatLonToTile(southwest, zoom);
      Point2D northeast_tile = GoogleMaps::ConvertFromLatLonToTile(northeast, zoom);
      
      LatLon southwest_tile_latlon = GoogleMaps::ConvertFromTileToLatLon(southwest_tile, zoom);
      LatLon northeast_tile_latlon = GoogleMaps::ConvertFromTileToLatLon(northeast_tile, zoom);
      
      return LatLonBounds(southwest_tile_latlon, northeast_tile_latlon);
    }


    Point2D GoogleMaps::ConvertFromLatLonToPoint(const LatLon& latlon) {
      Point2D point(0, 0);     
      point.x = _pixelOrigin.x + latlon.lon * _pixelsPerLonDegree;
      
      // NOTE(sarietta): Truncating to 0.9999 effectively limits
      // latitude to 89.189.  This is about a third of a tile past the
      // edge of the world tile.
      const double siny = BoundValue(sin(DegreesToRadians(latlon.lat)), -0.9999, 0.9999);
      point.y = _pixelOrigin.y - 0.5 * log((1.0 + siny) / (1.0 - siny)) * _pixelsPerLonRadian;
      return point;
    }
    
    LatLon GoogleMaps::ConvertFromPointToLatLon(const Point2D& point) {
      const double lng = (point.x - _pixelOrigin.x) / _pixelsPerLonDegree;
      const double latRadians = (point.y - _pixelOrigin.y) / -_pixelsPerLonRadian;
      const double lat = RadiansToDegrees(2.0 * atan(exp(latRadians)) - M_PI / 2.0);
      return LatLon(lat, lng);
    }
    
    Point2D GoogleMaps::ConvertFromLatLonToTile(const LatLon& latlon, const int32& zoom) {
      const double numTiles = 1 << zoom;
      const Point2D worldCoordinate = GoogleMaps::ConvertFromLatLonToPoint(latlon);
      const Point2D pixelCoordinate(worldCoordinate.x * numTiles,
				    worldCoordinate.y * numTiles);
      const Point2D tileCoordinate(floor(pixelCoordinate.x / _tileSize),
				   floor(pixelCoordinate.y / _tileSize));
      
      return tileCoordinate;
    }

    LatLon GoogleMaps::ConvertFromTileToLatLon(const Point2D& tileCoordinate, const int32& zoom) {
      const double numTiles = 1 << zoom;
      const Point2D pixelCoordinate(tileCoordinate.x * _tileSize,
				    tileCoordinate.y * _tileSize);
      const Point2D worldCoordinate(pixelCoordinate.x / numTiles,
				    pixelCoordinate.y / numTiles);
      const LatLon latlon = GoogleMaps::ConvertFromPointToLatLon(worldCoordinate);
      
      return latlon;
    }

  }  // namespace util
}  // namespace slib
