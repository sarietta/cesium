#ifndef __GL_EVENT_HANDLER_H__
#define __GL_EVENT_HANDLER_H__

namespace slib {
  namespace gl {

    enum GLEventType {
      GL_EVENT_TYPE_INIT,
      GL_EVENT_TYPE_RESIZE,
      GL_EVENT_TYPE_DRAW,
      GL_EVENT_TYPE_KILL_WINDOW
    };

    class GLEventHandler {
    public:
      virtual void HandleEvent(const GLEventType& event) = 0;
    };

  }  // namespace gl
}  // namespace slib

#endif
