#include "Timer.h"
#include <hal/nrf_rtc.h> // for nrf_rtc_counter_get
#include <FreeRTOS.h>    // for portNRF_RTC_REG

class IRQ_Guard {
public:
  inline IRQ_Guard() {
    Enable();
  }
  inline ~IRQ_Guard() {
    Disable();
  }
  inline void Enable() {
    ctx = __get_PRIMASK();
    __disable_irq();
  }
  inline void Disable() {
    if ((ctx & 0x01) == 0) {
      __enable_irq();
    }
  }
private:
  uint32_t ctx;
};

using namespace Pinetime::Components;

Pinetime::Utility::HeaplessQueue<Timer> Timer::active_timers;

/*
 * Add the timer to the active timers queue. The next timer due is in front.
 */
void Timer::Start() {
  IRQ_Guard guard;
  start = nrf_rtc_counter_get(portNRF_RTC_REG);
  active_timers.InsertSorted(*this);
  is_active = true;
}

/*
 * Deactivate a timer and remove it from the timers queue.
 */
void Timer::Stop() {
  IRQ_Guard guard;
  active_timers.Extract(*this);
  is_active = false;
}

void Timer::ChangePeriod(tick_t newPeriod) {
  // TODO: if (IsActive())
  period = newPeriod;
}

void Timer::Process() {
  IRQ_Guard guard;
  tick_t now = nrf_rtc_counter_get(portNRF_RTC_REG);

  while (!active_timers.Empty() && (active_timers.Front() <= now)) {
    Timer& timer = active_timers.Front();
    active_timers.Extract(timer);
    if (timer.mode == Mode::Repeated) {
      timer.start += timer.period;
      active_timers.InsertSorted(timer);
    } else {
      timer.is_active = false;
    }

    guard.Disable();
    timer.Invoke();
    guard.Enable();
  }
}
