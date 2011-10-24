#ifndef __SLIB_EVENT_KEYBOARD_HANDLERS_H__
#define __SLIB_EVENT_KEYBOARD_HANDLERS_H__

namespace slib {
  namespace event {

    // Keyboard button event handler.
    class KeyboardEventHandler {
    private:
      const void (*handler_)(const int32 key);
    public:
      KeyboardEventHandler(const void (*handler)(const int32 key)) {
	handler_ = handler;
      }

      void HandleEvent(const int32 key) const {
	handler_(key);
      }
    };
  }  // namespace event
}  // namespace slib

#endif
