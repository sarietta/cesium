#ifndef __SLIB_UTIL_TIMER_H__
#define __SLIB_UTIL_TIMER_H__

#include "../common/types.h"
#include <ostream>
#include <stack>
#include <time.h>

/**
   This class is NOT thread-safe. Making it thread safe is tricky
   since we don't want to stall parts of the code just to enable the
   timing routines to work correctly.
 */
namespace slib {
  namespace util {

    class Timer {
    public:
      static void Start() {
	Timer::_start_ticks.push(clock());
      }

      static void StartIf(const bool& test) {
	if (test) {
	  Timer::_start_ticks.push(clock());
	}
      }
      // This returns a handle to a Timer object that can be passed to
      // an ostream.
      static const Timer& Stop() {
	Timer& timer = Timer::GetInstance();
	clock_t end_ticks = clock();
	if (Timer::_start_ticks.size() > 0) {
	  timer._elapsed = (((float) end_ticks) - ((float) Timer::_start_ticks.top())) / CLOCKS_PER_SEC;
	  Timer::_start_ticks.pop();
	}
	return timer;
      }

      friend std::ostream& operator<<(std::ostream& out, const Timer& timer) {
	out << timer.GetElapsedSeconds() << "s";
	return out;
      }

    private:
      static std::stack<clock_t> _start_ticks;
      static Timer _obj;

      double _elapsed;
      
      static Timer& GetInstance() {
	_obj._elapsed = 0.0;
	return Timer::_obj;
      }

      double GetElapsedSeconds() const {
	return _elapsed;
      }
    };

  }  // namespace util
}  // namespace slib

#endif
