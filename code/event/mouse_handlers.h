#ifndef __SLIB_EVENT_MOUSE_HANDLERS_H__
#define __SLIB_EVENT_MOUSE_HANDLERS_H__

namespace slib {
  namespace event {

    // Mouse motion event handler.
    class MouseMotionEventHandler {
    private:
      const void (*handler_)(const int32 x, const int32 y, const int32 button);
    public:
      MouseMotionEventHandler(const void (*handler)(const int32 x, const int32 y, const int32 button)) {
	handler_ = handler;
      }

      void HandleEvent(const int32 x, const int32 y, const int32 button) const {
	handler_(x, y, button);
      }
    };

    // Mouse click event handler.
    class MouseClickEventHandler {
    private:
      const void (*handler_)(const int32 button);
    public:
      MouseClickEventHandler(const void (*handler)(const int32 button)) {
	handler_ = handler;
      }

      void HandleEvent(const int32 button) const {
	handler_(button);
      }
    };

  }  // namespace event
}  // namespace slib

#endif
