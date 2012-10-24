#ifndef __SLIB_DRAWING_CANVAS_H__
#define __SLIB_DRAWING_CANVAS_H__

#define SLIB_NO_DEFINE_64BIT

#include <CImg.h>
#include <common/types.h>

namespace slib {
  namespace event {
    class KeyboardEventHandler;
    class MouseMotionEventHandler;
    class MouseClickEventHandler;
  }
}

namespace slib {
  namespace drawing {
    
    class Canvas {
    private:
      FloatImage* canvas_;
      float current_brush_color_[4];

      void init(const int32& width, const int32& height);
    public:
      Canvas();
      Canvas(const int32 width, const int32 height);
      Canvas(const FloatImage background_image);
      virtual ~Canvas();
      
      const FloatImage& GetCanvas() const;
      FloatImage& GetCanvasMutable() const;

      void ResizeCanvas(const int32& width, const int32& height);
      void SetBrushColor(const float& r, const float& g, 
			 const float& b, const float& opacity);
      void SetBackgroundImage(const FloatImage image);
      void DisplayCanvas(const slib::event::MouseMotionEventHandler* motion_handler, 
			 const slib::event::MouseClickEventHandler* click_handler,
			 const slib::event::KeyboardEventHandler* key_handler) const;
    };
    
  }  // namespace drawing
}  // namespace slib

#endif
