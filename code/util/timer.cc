#include "timer.h"

#include <stack>

using std::stack;

namespace slib {
  namespace util {

    stack<clock_t> Timer::_start_ticks;
    Timer Timer::_obj = Timer();

  }  // namespace util
}  // namespace slib

