#ifndef __GL_RENDERER_H__
#define __GL_RENDERER_H__

#include <common/types.h>
#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <X11/extensions/xf86vmode.h>

namespace slib {
  namespace gl {
    class GLEventHandler;
  }
}

namespace slib {
  namespace gl {
    /* attributes for a single buffered visual in RGBA format with at least
     * 4 bits per color and a 16 bit depth buffer */
    static int attrListSgl[] = {GLX_RGBA, GLX_RED_SIZE, 4, 
				GLX_GREEN_SIZE, 4, 
				GLX_BLUE_SIZE, 4, 
				GLX_DEPTH_SIZE, 16,
				None};

    /* attributes for a double buffered visual in RGBA format with at least 
     * 4 bits per color and a 16 bit depth buffer */
    static int attrListDbl[] = { GLX_RGBA, GLX_DOUBLEBUFFER, 
				 GLX_RED_SIZE, 4, 
				 GLX_GREEN_SIZE, 4, 
				 GLX_BLUE_SIZE, 4, 
				 GLX_DEPTH_SIZE, 16,
				 None };
    
    class GLRenderer {
    public:
      // Constructor for when you use static / C-style methods
      // TODO(sarietta): Implement me!
      // Constructor for when you use non-static class methods
      GLRenderer(GLEventHandler* handler);

      bool CreateWindow(const char* title, const int& width, const int& height, 
			const int& bits, const bool& fullscreen);
      bool ResizeWindow(const int32& width, const int32& height);
      void RedrawWindow();
      void KillWindow();

      // Quick getter/setter
      inline uint16 GetWidth() const {
	return width_;
      }
      inline uint16 GetHeight() const {
	return height_;
      }
      inline Display* GetMutableDisplay() const {
	return display_;
      }
      inline bool IsFullscreen() const {
	return fullscreenMode_;
      }
    private:
      GLEventHandler* handler_;

      Display* display_;
      XVisualInfo* visualInfo_;
      Colormap colorMap_;
      int32 screen_;
      Window window_;
      GLXContext context_;
      XSetWindowAttributes attributes_;
      bool doubleBuffered_;
      bool fullscreenMode_;
      XF86VidModeModeInfo videoMode_;
      uint16 width_, height_, depth_;
    };

  }  // namespace gl
}  // namespace slib

#endif
