#pragma once

#include <cstdint>
#include <functional>

namespace Pinetime {
  namespace Components {
    class Timer {
      using tick_t = uint32_t;
      using tick_diff_t = int32_t;

    public:
      enum class Mode : uint8_t { SingleShot, Repeated };

      Timer(tick_t period, Mode mode, const std::function<void()>&& callback)
        : callback {callback}, mode {mode}, is_active(false), period {period}, start {0}, p_next {nullptr} {
      }
      Timer(tick_t period, const std::function<void()>&& callback) : Timer(period, Mode::SingleShot, std::move(callback)) {
      }
      ~Timer() {
        if (IsActive()) {
          Stop();
        }
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
      void Advance();
      void ChangePeriod(tick_t newPeriod);
      bool IsActive() const {
        return is_active;
      }
      tick_t GetExpiryTime() const {
        return start + period;
      }
      static void Process();

      /**
       * This function is used for comparing two time points.
       *
       * @note You have to use this function to avoid issues with counter overflow.
       * @note Maximum expected difference should be less than (2^31 - 1).
       *
       * @return positive number in case if @end is higher than @begin.
       * @return 0 if @end is equal @begin.
       * @return negative number if @end is less than @begin.
       */
      static inline tick_diff_t GetTimeDiff(tick_t begin, tick_t end) {
        return static_cast<tick_diff_t>(end - begin);
      }

    private:
      void add_to_list();
      void remove_from_list();
      std::function<void()> callback;
      Mode mode;
      bool is_active;
      tick_t period;
      tick_t start;
      Timer* p_next;
      static Timer* p_active_timers;
    };
  }
}
