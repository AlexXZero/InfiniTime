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

HeaplessSortedQueue<Timer> Timer::p_active_timers;

/*
 * Add the timer to the active timers queue. The next timer due is in front.
 */
void Timer::Start() {
  IRQ_Guard guard;
  start = nrf_rtc_counter_get(portNRF_RTC_REG);
  p_active_timers.Emplace(this);
  is_active = true;
}

/*
 * Deactivate a timer and remove it from the timers queue.
 */
void Timer::Stop() {
  IRQ_Guard guard;
  p_active_timers.Remove(this);
  is_active = false;
}

void Timer::ChangePeriod(tick_t newPeriod) {
  // TODO: if (IsActive())
  period = newPeriod;
}

void Timer::Process() {
  tick_t now = nrf_rtc_counter_get(portNRF_RTC_REG);
  IRQ_Guard guard;

  while (!p_active_timers.Empty() && (*p_active_timers.begin() <= now)) {
    Timer* p_timer = p_active_timers.begin();
    p_active_timers.Pop();
    p_timer->is_active = false;
    if (p_timer->mode == Mode::Repeated) {
      p_timer->start += p_timer->period;
      p_active_timers.Emplace(p_timer);
      p_timer->is_active = true;
    }

    guard.Disable();
    p_timer->callback();
    guard.Enable();
  }
}
