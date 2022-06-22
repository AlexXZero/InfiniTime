#include "EventLog.h"
#include "systemtask/SystemTask.h"

using namespace Pinetime::Components;

/*
 * 0x00000000 - 0x7fffffff (31 bits) - Unix timestamp: saved on page swapping and on time syncing
 * 0x80000000 - 0x80ffffff (24 bits) - Time diff milliseconds: time was elapsed since last event was saved
 * 0x81000000 - 0x81ffffff (24 bits) - Simple events: 16,777,216 unique events without data
 * 0x82000000 - 0x82ffffff (24 bits) - 16 bits events: 256 types of unique events with 16 bits data
 * 0x83000000 - 0xfffffffe - unused
 */

static const uint32_t UNIX_TIME_TYPE = 0x00;
static const uint32_t UNIX_TIME_TYPE_SHIFT = 31;
static const uint32_t UNIX_TIME_VALUE_MASK = 0x7fffffff;

static const uint32_t TIME_DIFF_TYPE = 0x80;
static const uint32_t TIME_DIFF_TYPE_SHIFT = 24;
static const uint32_t TIME_DIFF_VALUE_MASK = 0xffffff;

static const uint32_t SIMPLE_EVENT_TYPE = 0x81;
static const uint32_t SIMPLE_EVENT_TYPE_SHIFT = 24;
static const uint32_t SIMPLE_EVENT_VALUE_MASK = 0xffffff;

static const uint32_t EVENT16_TYPE = 0x82;
static const uint32_t EVENT16_TYPE_SHIFT = 24;
static const uint32_t EVENT16_SUBTYPE_MASK = 0xff;
static const uint32_t EVENT16_SUBTYPE_SHIFT = 16;
static const uint32_t EVENT16_VALUE_MASK = 0xffff;


template <> void EventLog::Write<Event::UnixTime>() {
  auto unix_time = std::chrono::time_point_cast<std::chrono::seconds>(dateTimeController.CurrentDateTime());
  last_timediff = unix_time;
  uint32_t seconds = unix_time.time_since_epoch().count();
  lastPage.Write(lastEvent.EventIndex(), (UNIX_TIME_TYPE << UNIX_TIME_TYPE_SHIFT)
                                  | (seconds & UNIX_TIME_VALUE_MASK));
  ++lastEvent;
}

template <> void EventLog::Write<Event::TimeDiff>(uint32_t diff_ms) {
  lastPage.Write(lastEvent.EventIndex(), (TIME_DIFF_TYPE << TIME_DIFF_TYPE_SHIFT)
                                  | (diff_ms & TIME_DIFF_VALUE_MASK));
  ++lastEvent;
}

template <> void EventLog::Write<Event::Simple>(SimpleEvent event_type) {
  WriteRaw((SIMPLE_EVENT_TYPE << SIMPLE_EVENT_TYPE_SHIFT)
         | ((uint32_t)event_type & SIMPLE_EVENT_VALUE_MASK));
}

template <> void EventLog::Write<Event::Event16>(Event16Event event_type, uint16_t data) {
  WriteRaw((EVENT16_TYPE << EVENT16_TYPE_SHIFT)
         | (((uint32_t)event_type & EVENT16_SUBTYPE_MASK) << EVENT16_SUBTYPE_SHIFT)
         | ((uint32_t)data & EVENT16_VALUE_MASK));
}


static EventLogPage FindLastPage(IEventLogStorage& storage) {
  // If eventlog is not initialised, then lets do it
  if (EventLogPage{storage, 0}.IsEmpty()) {
    EventLogPage page{storage, 1, (uint16_t)(1U - storage.PagesAmount())};
    for (size_t pageIndex = 0; pageIndex < storage.PagesAmount(); pageIndex++) {
      page.Initialise();
      ++page;
    }
    return EventLogPage{storage, 0};
  }

#if 0 // more efficient
  EventLogPage testPage{storage, 0};
  for (size_t pageIndex = 1; pageIndex < storage.PagesAmount(); pageIndex++) {
    if (testPage.Number() + 1U != EventLogPage(storage, pageIndex).Number()) {
      break;
    }
    testPage++;
  }
  return testPage;
#else // more readable
  for (size_t pageIndex = 0; pageIndex < storage.PagesAmount() - 1; pageIndex++) {
    EventLogPage testPage{storage, pageIndex};
    EventLogPage nextPage{storage, pageIndex + 1};
    if (testPage.Number() + 1U != nextPage.Number()) {
      return testPage;
    }
  }
  return EventLogPage{storage, storage.PagesAmount() - 1};
#endif
}

EventLog::EventLog(Pinetime::System::SystemTask& systemTask,
    IEventLogStorage& storage, const Pinetime::Controllers::DateTime& dateTimeController)
  : systemTask {systemTask},
    storage{storage},
    dateTimeController{dateTimeController},
    lastPage{FindLastPage(storage)},
    lastEvent{lastPage, lastPage} {
  // TODO: check if eventlog corrupted, then erase it.

  if (lastPage.IsEmpty()) {
    lastPage.Initialise();
  } else {
    while (lastEvent.Value() != EMPTY_WORD) ++lastEvent;
  }

  Write<Event::UnixTime>();
}

void EventLog::SwapPages() {
  ++lastPage;
  lastPage.Erase();
  lastPage.Initialise();
  ++lastEvent;
  swapRequested = false;
  Write<Event::UnixTime>();
}

void EventLog::WriteRaw(uint32_t event) {
  auto now = dateTimeController.CurrentDateTime();
  uint32_t duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_timediff).count();
  if (duration_ms >= 1) {
    last_timediff = now;
    Write<Event::TimeDiff>(duration_ms);
  }
  lastPage.Write(lastEvent.EventIndex(), event);
  ++lastEvent;
  if (!swapRequested && lastPage.IsFullEnough(lastEvent.EventIndex())) {
    systemTask.PushMessage(Pinetime::System::Messages::SwapEventlogPages);
    swapRequested = true;
  }
}
