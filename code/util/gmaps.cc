#include "gmaps.h"

#include <CImg.h>
#include <common/types.h>
#include <glog/logging.h>
#include <math.h>
#include "mathutils.h"
#include <string>
#include <string/stringutils.h>
#include <util/colormap.h>
#include <vector>

using slib::StringUtils;
using std::string;
using std::vector;

namespace slib {
  namespace util {

    /** GoogleMaps Implementation **/    
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
    
    FloatImage GoogleMaps::DrawPoints(const LatLon& southwest, const LatLon& northeast,
				      const vector<LatLon>& locations, 
				      const FloatImage& map_image, const int& zoom,
				      const std::string& point_color, const int& point_size) {
      FloatImage map(map_image);
      
      const LatLonBounds map_bounds 
	= GoogleMaps::GetMapBounds(southwest, northeast, zoom);
      
      VLOG(2) << "Map Southwest: " 
		<< map_bounds.southwest.lat << DEGREE_SYMBOL << " " 
		<< map_bounds.southwest.lon << DEGREE_SYMBOL;
      VLOG(2) << "Map Northeast: " 
		<< map_bounds.northeast.lat << DEGREE_SYMBOL << " " 
		<< map_bounds.northeast.lon << DEGREE_SYMBOL;
      
      int numTiles = 1 << zoom;
      const Point2D min_point 
	= GoogleMaps::ConvertFromLatLonToPoint(LatLon(map_bounds.northeast.lat,
						      map_bounds.southwest.lon));
      double map_min_x = min_point.x * numTiles;
      double map_min_y = min_point.y * numTiles;     
      
      int sprite_size = point_size;
      if (sprite_size < 0) {
	sprite_size = 15 / (1 << (16 - zoom));
      }
      float* color = new float(3);
      slib::util::ColorMap::HexToRGB(point_color, color);
      
      int numPointsRendered = 0;
      VLOG(1) << "Found " << locations.size() << " locations";
      
      for (uint32 i = 0; i < locations.size(); i++) {
	Point2D point = GoogleMaps::ConvertFromLatLonToPoint(locations[i]);
	point.x = point.x * numTiles - map_min_x;
	point.y = point.y * numTiles - map_min_y;
	
	if (point.x < 0 || point.x > map.width()) {
	  continue;
	}
	if (point.y < 0 || point.y > map.height()) {
	  continue;
	}
	numPointsRendered++;
	
	const double opacity = 0.3;
	map.draw_circle(point.x, point.y, sprite_size, color, opacity);
      }
      
      VLOG(1) << "Rendered a total of " << numPointsRendered << " points";
      
      return map;
    }
    /** GoogleMaps Implementation **/

    /** GoogleMapsConverter Implementation **/
    
    GoogleMapsConverter::GoogleMapsConverter(const int& zoom, const LatLon& southwest, const LatLon& northeast) {
      const LatLonBounds map_bounds = GoogleMaps::GetMapBounds(southwest, northeast, zoom);
      _num_tiles = 1 << zoom;
      const Point2D min_point = GoogleMaps::ConvertFromLatLonToPoint(LatLon(map_bounds.northeast.lat,
									    map_bounds.southwest.lon));
      _map_min_x = min_point.x * _num_tiles;
      _map_min_y = min_point.y * _num_tiles;
    }

    // Converts a latitude/longitude position to an (x,y) position
    // in the map. This is obviously dependent on the map size,
    // which is implicitly set via the constructor's zoom parameter.
    Point2D GoogleMapsConverter::ConvertLocationToMapPoint(const LatLon& location) {
      Point2D point = GoogleMaps::ConvertFromLatLonToPoint(location);
      point.x = point.x * _num_tiles - _map_min_x;
      point.y = point.y * _num_tiles - _map_min_y;

      return point;
    }

    /** GoogleMapsConverter Implementation **/
    
  }  // namespace util
}  // namespace slib
