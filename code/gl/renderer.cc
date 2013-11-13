#include "renderer.h"

#include <GL/glx.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <glog/logging.h>
#include <X11/extensions/xf86vmode.h>

#include "eventhandler.h"

namespace slib {
  namespace gl {

    GLRenderer::GLRenderer(GLEventHandler* handler) {
      handler_ = handler;
    }
    
    void GLRenderer::RedrawWindow() {
      if (doubleBuffered_) {
	glXSwapBuffers(display_, window_);
      }
    }

    bool GLRenderer::ResizeWindow(const int32& width, const int32& height) {
      width_ = width;
      height_ = height;
      handler_->HandleEvent(GL_EVENT_TYPE_RESIZE);

      return true;
    }

    void GLRenderer::KillWindow() {
      if (context_) {
        if (!glXMakeCurrent(display_, None, NULL)) {
	  LOG(WARNING) << "Could not release drawing context";
        }
        glXDestroyContext(display_, context_);
        context_ = NULL;
      }
      /* switch back to original desktop resolution if we were in fs */
      if (fullscreenMode_) {
        XF86VidModeSwitchToMode(display_, screen_, &videoMode_);
        XF86VidModeSetViewPort(display_, screen_, 0, 0);
      }
      XCloseDisplay(display_);
    }
    
    bool GLRenderer::CreateWindow(const char* title, const int& width, const int& height, 
				  const int& bits, const bool& fullscreen) {
      Window winDummy;
      unsigned int borderDummy;
      
      /* get a connection */
      display_ = XOpenDisplay(NULL);
      if (!display_) {
	LOG(ERROR) << "Could not get display: 0";
	return false;
      }
      screen_ = DefaultScreen(display_);

      int vidModeMajorVersion, vidModeMinorVersion;
      XF86VidModeQueryVersion(display_, &vidModeMajorVersion,
			      &vidModeMinorVersion);
      LOG(INFO) << "XF86VidModeExtension-Version " 
		<< vidModeMajorVersion << "." << vidModeMinorVersion;
      int glxMajorVersion, glxMinorVersion;
      glXQueryVersion(display_, &glxMajorVersion, &glxMinorVersion);
      LOG(INFO) << "glX-Version " << glxMajorVersion << "." << glxMinorVersion;

      /* look for mode with requested resolution */
      int32 bestMode = 0;
      XF86VidModeModeInfo** modes;
      int modeNum;
      // Save the current mode.
      XF86VidModeGetAllModeLines(display_, screen_, &modeNum, &modes);
      videoMode_ = *modes[0];
      for (int32 i = 0; i < modeNum; i++) {
	if ((modes[i]->hdisplay == width) && (modes[i]->vdisplay == height)) {
	  bestMode = i;
	}
      }

      /* get an appropriate visual */
      visualInfo_ = glXChooseVisual(display_, screen_, attrListDbl);
      if (visualInfo_ == NULL) {
	visualInfo_ = glXChooseVisual(display_, screen_, attrListSgl);
	doubleBuffered_ = false;
	LOG(INFO) << "Only Singlebuffered Visual!";
      } else {
	doubleBuffered_ = true;
	LOG(INFO) << "Got Doublebuffered Visual!";
      }
           
      /* create a GLX context */
      context_ = glXCreateContext(display_, visualInfo_, 0, GL_TRUE);
      /* create a color map */
      colorMap_ = XCreateColormap(display_, RootWindow(display_, visualInfo_->screen),
				  visualInfo_->visual, AllocNone);
      attributes_.colormap = colorMap_;
      attributes_.border_pixel = 0;
      attributes_.event_mask = 
	KeyPressMask		|
	KeyReleaseMask		|
	ButtonPressMask		|
	ButtonReleaseMask 	|
	EnterWindowMask		|
	LeaveWindowMask		|
	PointerMotionMask 	|
	KeymapStateMask		|
	ExposureMask		|
	StructureNotifyMask
	;
      
      fullscreenMode_ = fullscreen;
      if (fullscreenMode_) {
	XF86VidModeSwitchToMode(display_, screen_, modes[bestMode]);
	XF86VidModeSetViewPort(display_, screen_, 0, 0);
	int32 displayWidth = modes[bestMode]->hdisplay;
	int32 displayHeight = modes[bestMode]->vdisplay;
	LOG(INFO) << "Resolution " << displayWidth << "x" << displayHeight;
	
	/* create a fullscreen window */
	attributes_.override_redirect = true;
	window_ = XCreateWindow(display_, 
				RootWindow(display_, visualInfo_->screen),
				0, 0, displayWidth, displayHeight, 0, visualInfo_->depth, 
				InputOutput, visualInfo_->visual,
				CWBorderPixel | CWColormap | CWEventMask | CWOverrideRedirect,
				&attributes_);
	XWarpPointer(display_, None, window_, 0, 0, 0, 0, 0, 0);
	XMapRaised(display_, window_);
	XGrabKeyboard(display_, window_, true, GrabModeAsync,
		      GrabModeAsync, CurrentTime);
	XGrabPointer(display_, window_, true, ButtonPressMask,
		     GrabModeAsync, GrabModeAsync, window_, None, CurrentTime);
	glXMakeCurrent(display_, window_, context_);
      } else {
	/* create a window in window mode*/
	window_ = XCreateWindow(display_, 
				RootWindow(display_, visualInfo_->screen),
				0, 0, width, height, 0, visualInfo_->depth, 
				InputOutput, visualInfo_->visual,
				CWBorderPixel | CWColormap | CWEventMask, &attributes_);
	XSetStandardProperties(display_, window_, title, title, None, NULL, 0, NULL);
	glXMakeCurrent(display_, window_, context_);
	XMapWindow(display_, window_);
      }       

      XSelectInput(display_, window_, attributes_.event_mask);
      XFree(modes);

      int x, y;
      XGetGeometry(display_, window_, &winDummy, &x, &y,
		   &width_, &height_, &borderDummy, &depth_);
      
      if (glXIsDirect(display_, context_)) 
	LOG(INFO) << "Direct Rendering available.";
      else
	LOG(WARNING) << "Direct Rendering not available.";
      
      handler_->HandleEvent(GL_EVENT_TYPE_INIT);
      return true;
    }
    
  }  // namespace gl
}  // namespace slib
