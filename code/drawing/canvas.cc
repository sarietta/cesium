#include "canvas.h"

#include <CImg.h>
#include <common/types.h>
#include <event/keyboard_handlers.h>
#include <event/mouse_handlers.h>
#include <glog/logging.h>

using slib::event::KeyboardEventHandler;
using slib::event::MouseMotionEventHandler;
using slib::event::MouseClickEventHandler;

using namespace cimg_library;

namespace slib {
  namespace drawing {
    
    Canvas::Canvas() {
      init(0, 0);
    }
    
    Canvas::Canvas(const int32 width, const int32 height) {
      init(width, height);
    }
    
    Canvas::Canvas(const FloatImage background_image) {
      init(background_image.width(), background_image.height());
      SetBackgroundImage(background_image);
    }
    
    Canvas::~Canvas() {}

    void Canvas::init(const int32& width, const int32& height) {      
      canvas_ = NULL;
      ResizeCanvas(width, height);
      SetBrushColor(1.0f, 0.0f, 0.0f, 0.5f);
    }

    const FloatImage& Canvas::GetCanvas() const {
      return *(static_cast<const FloatImage*>(canvas_));
    }

    FloatImage& Canvas::GetCanvasMutable() const {
      return *(canvas_);
    }

    void Canvas::ResizeCanvas(const int32& width, const int32& height) {
      if (canvas_ != NULL) {
	const FloatImage copy(*canvas_);
	delete canvas_;
	canvas_ = new FloatImage(width, height, 1, 3);
	canvas_->draw_image(copy);
      } else {
	canvas_ = new FloatImage(width, height, 1, 3);
      }
    }
    
    void Canvas::SetBrushColor(const float& r, const float& g, 
			       const float& b, const float& opacity) {
      current_brush_color_[0] = r;
      current_brush_color_[1] = g;
      current_brush_color_[2] = b;
      current_brush_color_[3] = opacity;
    }

    void Canvas::SetBackgroundImage(const FloatImage image) {
      canvas_->draw_image(0, 0, image);
    }
    
    void Canvas::DisplayCanvas(const MouseMotionEventHandler* motion_handler, 
			       const MouseClickEventHandler* click_handler,
			       const KeyboardEventHandler* key_handler) const {
      if (canvas_ == NULL) {
	LOG(WARNING) << "Tried to start drawing on an empty canvas.";
	return;
      }
      
      int xo = -1, yo = -1, x = -1, y = -1;
      bool redraw = true;

      CImgDisplay disp((*canvas_));
      while (!disp.is_closed() && !disp.is_keyQ() && !disp.is_keyESC()) {
	const int32 button = disp.button();
	redraw = false;
	xo = x; yo = y; x = disp.mouse_x(); y = disp.mouse_y();
	if (xo >= 0 && yo >= 0 && x >= 0 && y >= 0) {
	  if (button & 1 || button & 4) {
	    if (y < canvas_->height() && x < canvas_->width()) {
	      const float tmax = static_cast<float>(cimg::max(cimg::abs(xo - x), cimg::abs(yo - y)) + 0.1f);
	      const int radius = (button & 1 ? 3 : 0) + (button & 4 ? 6 : 0);
	      for (float t = 0; t <= tmax; ++t) {
		canvas_->draw_circle(static_cast<int>(x + t * (xo - x) / tmax), 
				     static_cast<int>(y + t * (yo - y) / tmax), 
				     radius, 
				     current_brush_color_,
				     current_brush_color_[3]);
	      }
	    }
	    redraw = true;
	  }
	  if (disp.button() & 2) { 
	    canvas_->draw_fill(x, y, current_brush_color_, current_brush_color_[3]); 
	    redraw = true; 
	  }
	}
	if (redraw) {
	  disp.display(*canvas_);
	}
	disp.resize(disp).wait();
      }
    }
    
  }  // namespace drawing
}  // namespace slib
