#pragma once

#include <cstdint>
#include <functional>

template <typename T>
class HeaplessSortedQueue {
public:
  T* begin() {
    return head;
  }
  const T* begin() const {
    return head;
  }
  constexpr T* end() const {
    return nullptr;
  }
  void Emplace(T* element) {
    T** it = &head;
    while ((*it != end()) && (*element >= **it)) {
      it = &(*it)->p_next;
    }

    element->p_next = *it;
    *it = element;
  }
  void Remove(const T* element) {
    for (T** it = &head; *it != end(); it = &(*it)->p_next) {
      if (*it == element) {
        *it = element->p_next;
        break;
      }
    }
  }
  void Pop() {
    head = head->p_next;
  }
  bool Empty() const {
    return begin() == end();
  }

private:
  T* head = nullptr;
};

namespace Pinetime {
  namespace Components {
    class Timer {
    public:
      using tick_t = uint32_t;
      using tick_diff_t = int32_t;
      enum class Mode : uint8_t { SingleShot, Repeated };

      Timer() = default;
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
      bool operator<(tick_t time) const {
        return static_cast<tick_diff_t>(GetExpiryTime() - time) < 0;
      }
      bool operator<=(tick_t time) const {
        return static_cast<tick_diff_t>(GetExpiryTime() - time) <= 0;
      }
      bool operator>(tick_t time) const {
        return static_cast<tick_diff_t>(GetExpiryTime() - time) > 0;
      }
      bool operator>=(tick_t time) const {
        return static_cast<tick_diff_t>(GetExpiryTime() - time) >= 0;
      }
      bool operator<(const Timer& timer) const {
        return static_cast<tick_diff_t>(GetExpiryTime() - timer.GetExpiryTime()) < 0;
      }
      bool operator<=(const Timer& timer) const {
        return static_cast<tick_diff_t>(GetExpiryTime() - timer.GetExpiryTime()) <= 0;
      }
      bool operator>(const Timer& timer) const {
        return static_cast<tick_diff_t>(GetExpiryTime() - timer.GetExpiryTime()) > 0;
      }
      bool operator>=(const Timer& timer) const {
        return static_cast<tick_diff_t>(GetExpiryTime() - timer.GetExpiryTime()) >= 0;
      }

    private:
      std::function<void()> callback;
      Mode mode;
      bool is_active;
      tick_t period;
      tick_t start;
      Timer* p_next;
      friend class HeaplessSortedQueue<Timer>;
      static HeaplessSortedQueue<Timer> p_active_timers;
    };
  }
}
