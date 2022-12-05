#include "components/timer/TimerController.h"
#include "systemtask/SystemTask.h"

using namespace Pinetime::Controllers;

TimerController::TimerController()
  : timer {1, Components::Timer::Mode::SingleShot, [this] {
             OnTimerEnd();
           }} {
}

void TimerController::Init(Pinetime::System::SystemTask* systemTask) {
  this->systemTask = systemTask;
}

void TimerController::StartTimer(uint32_t duration) {
  timer.Start(pdMS_TO_TICKS(duration));
}

uint32_t TimerController::GetTimeRemaining() {
  if (timer.IsActive()) {
    auto remainingTime = timer.GetExpiryTime() - xTaskGetTickCount();
    return (remainingTime * 1000 / configTICK_RATE_HZ);
  }
  return 0;
}

void TimerController::StopTimer() {
  timer.Stop();
}

bool TimerController::IsRunning() {
  return timer.IsActive();
}

void TimerController::OnTimerEnd() {
  systemTask->PushMessage(System::Messages::OnTimerDone);
}
