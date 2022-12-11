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

Timer* Timer::p_active_timers = nullptr;

void Timer::Start() {
  IRQ_Guard guard;
  start = nrf_rtc_counter_get(portNRF_RTC_REG);
  add_to_list();
}

void Timer::Stop() {
  IRQ_Guard guard;
  remove_from_list();
}

void Timer::ChangePeriod(tick_t newPeriod) {
  // TODO: if (IsActive())
  period = newPeriod;
}

void Timer::Process() {
  tick_t now = nrf_rtc_counter_get(portNRF_RTC_REG);
  IRQ_Guard guard;

  while (p_active_timers != nullptr) {
    if (GetTimeDiff(now, p_active_timers->GetExpiryTime()) > 0) {
      break;
    }

    Timer* p_timer = p_active_timers;
    p_active_timers = p_timer->p_next;
    p_timer->is_active = false;
    if (p_timer->mode == Mode::Repeated) {
      p_timer->start += p_timer->period;
      p_timer->add_to_list();
    }

    guard.Disable();
    p_timer->callback();
    guard.Enable();
  }
}

/*
 * Add the timer to the active timers. The next timer due is in front.
 */
void Timer::add_to_list() {
  Timer** p_timer = &p_active_timers;
  while (*p_timer != nullptr) {
    if (GetTimeDiff(GetExpiryTime(), (*p_timer)->GetExpiryTime()) > 0) {
      break;
    }

    p_timer = &(*p_timer)->p_next;
  }

  p_next = *p_timer;
  *p_timer = this;
  is_active = true;
}

/*
 * Deactivate a timer and remove it from the timers queue.
 */
void Timer::remove_from_list() {
  for (Timer** p = &p_active_timers; *p != nullptr; p = &(*p)->p_next) {
    if (*p == this) {
      *p = p_next;
      is_active = false;
      break;
    }
  }
}
