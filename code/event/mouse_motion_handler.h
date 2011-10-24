#ifndef __SLIB_EVENT_MOUSE_HANDLERS_H__
#define __SLIB_EVENT_MOUSE_HANDLERS_H__

namespace slib {
  namespace event {

    class MouseMotionEventHandler {
    private:
      void (*handler_)(const int32 x, const int32 y, const int32 button) const;
    public:
      MouseMotionEventHandler(const void (*handler)(const int32 x, const int32 y, const int32 button) const) {
	handler_ = handler;
      }

      void HandleEvent(const int32 x, const int32 y, const int32 button) const {
	handler_(x, y, button);
      }
    };

  }  // namespace event
}  // namespace slib

#endif
