#pragma once

#include <cstdint>
#include <functional>
#include "Queue.h" // for Pinetime::Utility::HeaplessQueue

namespace Pinetime {
  namespace Components {
    class Timer: public Utility::HeaplessQueueObject<Timer> {
    public:
      using tick_t = uint32_t;
      using tick_diff_t = int32_t;
      enum class Mode : uint8_t { SingleShot, Repeated };

      Timer(tick_t period, Mode mode, const std::function<void()>&& callback)
        : callback {callback}, mode {mode}, is_active(false), period {period}, start {0} {
      }
      Timer(tick_t period, const std::function<void()>&& callback) : Timer(period, Mode::SingleShot, std::move(callback)) {
      }
      void Invoke() const {
        callback();
      }
      void Start();
      void Start(tick_t newPeriod) {
        ChangePeriod(newPeriod);
        Start();
      }
      void Stop();
      void Reset() {
        Stop();
        Start();
      }
      void Reset(tick_t newPeriod) {
        Stop();
        Start(newPeriod);
      }
      void ChangePeriod(tick_t newPeriod);
      bool IsActive() const {
        return is_active;
      }
      tick_t GetExpiryTime() const {
        return start + period;
      }
      bool operator<(const Timer& timer) const {
        return static_cast<tick_diff_t>(GetExpiryTime() - timer.GetExpiryTime()) < 0;
      }
      bool operator<=(tick_t time) const {
        return static_cast<tick_diff_t>(GetExpiryTime() - time) <= 0;
      }
      static void Process();

    private:
      std::function<void()> callback;
      Mode mode;
      bool is_active;
      tick_t period;
      tick_t start;
      static Utility::HeaplessQueue<Timer> active_timers;
    };
  }
}
