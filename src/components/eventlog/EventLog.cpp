#include "EventLog.h"
#include "systemtask/SystemTask.h"
#include "components/datetime/DateTimeController.h"

using namespace Pinetime::Components;

using word_t = uint32_t;

static constexpr size_t HEADER_WORD = 0;
static constexpr word_t EMPTY_WORD = 0xffffffff;
static constexpr word_t EMERGENCY_EVENTS_AMOUNT = 0x10;

static constexpr size_t VERSION_SHIFT = 24;
static constexpr word_t VERSION_MASK = 0xff;
static constexpr uint8_t CURRENT_VERSION = 0x01;

static constexpr size_t HEADER_SIZE_SHIFT = 16;
static constexpr word_t HEADER_SIZE_MASK = 0xff;
static constexpr uint8_t DEFAULT_HEADER_SIZE = 0x01;

static constexpr size_t PAGE_NUMBER_SHIFT = 0;
static constexpr word_t PAGE_NUMBER_MASK = 0xffff;


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


void EventLog::WriteRaw(uint32_t event) {
  if (lastEvent.pageHeaderSize + lastEvent.eventIndex >= storage.PageSize()) return; // page overflow check
  storage.Write(lastEvent.storagePageIndex, lastEvent.pageHeaderSize + lastEvent.eventIndex, event);
  ++lastEvent;
}

template <> void EventLog::Write<Event::UnixTime>() {
  auto unix_time = std::chrono::time_point_cast<std::chrono::seconds>(dateTimeController.CurrentDateTime());
  last_timediff = unix_time;
  uint32_t seconds = unix_time.time_since_epoch().count();
  WriteRaw((UNIX_TIME_TYPE << UNIX_TIME_TYPE_SHIFT)
         | (seconds & UNIX_TIME_VALUE_MASK));
}

template <> void EventLog::Write<Event::TimeDiff>(uint32_t diff_ms) {
  WriteRaw((TIME_DIFF_TYPE << TIME_DIFF_TYPE_SHIFT)
         | (diff_ms & TIME_DIFF_VALUE_MASK));
}

template <> void EventLog::Write<Event::Simple>(SimpleEvent event_type) {
  WriteEvent((SIMPLE_EVENT_TYPE << SIMPLE_EVENT_TYPE_SHIFT)
           | ((uint32_t)event_type & SIMPLE_EVENT_VALUE_MASK));
}

template <> void EventLog::Write<Event::Event16>(Event16Event event_type, uint16_t data) {
  WriteEvent((EVENT16_TYPE << EVENT16_TYPE_SHIFT)
           | (((uint32_t)event_type & EVENT16_SUBTYPE_MASK) << EVENT16_SUBTYPE_SHIFT)
           | ((uint32_t)data & EVENT16_VALUE_MASK));
}


size_t EventLog::FindLastPage(IEventLogStorage& storage) {
  uint16_t lastPageNumber = 0;

  for (size_t pageIndex = 0; pageIndex < storage.PagesAmount(); pageIndex++) {
    word_t pageHeader = storage.Read(pageIndex, HEADER_WORD);
    uint16_t currentPageNumber = ((pageHeader & PAGE_NUMBER_MASK) << PAGE_NUMBER_SHIFT);

    if (pageIndex == 0) {
      if (pageHeader == EMPTY_WORD) {
        storage.Erase(pageIndex);
        storage.Write(pageIndex, HEADER_WORD, ((word_t)(CURRENT_VERSION & VERSION_MASK) << VERSION_SHIFT)
                                            | ((word_t)(DEFAULT_HEADER_SIZE & HEADER_SIZE_MASK) << HEADER_SIZE_SHIFT)
                                            | ((word_t)(lastPageNumber & PAGE_NUMBER_MASK) << PAGE_NUMBER_SHIFT));
        return pageIndex;
      }
    } else {
      if (currentPageNumber != (lastPageNumber + 1)) {
        return pageIndex - 1;
      }
    }

    lastPageNumber = currentPageNumber;
  }

  return storage.PagesAmount() - 1;
}

EventLog::EventLog(Pinetime::System::SystemTask& systemTask,
                   IEventLogStorage& storage,
                   const Pinetime::Controllers::DateTime& dateTimeController)
  : systemTask {systemTask},
    storage {storage},
    dateTimeController {dateTimeController},
    lastEvent {lastEvent, storage, FindLastPage(storage)} {

  // Find last event
  while ((lastEvent.pageHeaderSize + lastEvent.eventIndex < storage.PageSize()) && (lastEvent.Value() != EMPTY_WORD)) {
    ++lastEvent;
  }

  Write<Event::UnixTime>();
}

EventLogIterator EventLog::begin() const {
  size_t storagePageIndex = lastEvent.storagePageIndex;
  do {
    storagePageIndex = (storagePageIndex + 1) % storage.PagesAmount();
  } while (storage.Read(storagePageIndex, HEADER_WORD) == EMPTY_WORD);
  return EventLogIterator(lastEvent, storage, storagePageIndex);
}

const EventLogIterator& EventLog::end() const {
  return lastEvent;
}

void EventLog::SwapPages() {
  lastEvent.pageNumber++;
  lastEvent.storagePageIndex = (lastEvent.storagePageIndex + 1) % storage.PagesAmount();
  lastEvent.pageHeaderSize = DEFAULT_HEADER_SIZE;
  lastEvent.eventIndex = 0;
  storage.Erase(lastEvent.storagePageIndex);
  storage.Write(lastEvent.storagePageIndex, HEADER_WORD,
                ((word_t)(CURRENT_VERSION & VERSION_MASK) << VERSION_SHIFT)
              | ((word_t)(lastEvent.pageHeaderSize & HEADER_SIZE_MASK) << HEADER_SIZE_SHIFT)
              | ((word_t)(lastEvent.pageNumber & PAGE_NUMBER_MASK) << PAGE_NUMBER_SHIFT));
  swapRequested = false;
  Write<Event::UnixTime>();
}

void EventLog::WriteEvent(uint32_t event) {
  auto now = dateTimeController.CurrentDateTime();
  uint32_t duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_timediff).count();
  if (duration_ms >= 1) {
    last_timediff = now;
    Write<Event::TimeDiff>(duration_ms);
  }

  WriteRaw(event);

  // Note: actual condition should looks like:
  // (!swapRequested && (lastEvent.eventIndex >= storage.PageSize() - (lastEvent.pageHeaderSize + EMERGENCY_EVENTS_AMOUNT)))
  // But we should use modified version to be 100% sure we don't have negative value on right side.
  if (!swapRequested && (lastEvent.pageHeaderSize + lastEvent.eventIndex + EMERGENCY_EVENTS_AMOUNT >= storage.PageSize())) {
    systemTask.PushMessage(Pinetime::System::Messages::SwapEventlogPages);
    swapRequested = true;
  }
}

void EventLog::EraseAll()
{
  for (size_t page = 0; page < storage.PagesAmount(); page++) {
    storage.Erase(page);
  }

  lastEvent.pageNumber = 0;
  lastEvent.storagePageIndex = 0;
  lastEvent.pageHeaderSize = DEFAULT_HEADER_SIZE;
  lastEvent.eventIndex = 0;
  storage.Write(lastEvent.storagePageIndex, HEADER_WORD,
                ((word_t)(CURRENT_VERSION & VERSION_MASK) << VERSION_SHIFT)
              | ((word_t)(lastEvent.pageHeaderSize & HEADER_SIZE_MASK) << HEADER_SIZE_SHIFT)
              | ((word_t)(lastEvent.pageNumber & PAGE_NUMBER_MASK) << PAGE_NUMBER_SHIFT));
  swapRequested = false;
  Write<Event::UnixTime>();
}



EventLogIterator::EventLogIterator(const EventLogIterator& lastEvent,
                                   IEventLogStorage& storage,
                                   size_t storagePageIndex,
                                   uint16_t eventIndex)
    : lastEvent {&lastEvent},
      storage {&storage},
      storagePageIndex {storagePageIndex},
      eventIndex {eventIndex} {
  ReadStoragePage();
}

void EventLogIterator::ReadStoragePage() {
  word_t pageHeader = storage->Read(storagePageIndex, HEADER_WORD);
  //pageVersion = (pageHeader >> VERSION_SHIFT) & VERSION_MASK;
  pageNumber = (pageHeader >> PAGE_NUMBER_SHIFT) & PAGE_NUMBER_MASK;
  pageHeaderSize = (pageHeader >> HEADER_SIZE_SHIFT) & HEADER_SIZE_MASK;
}

EventLogIterator& EventLogIterator::operator++() {
  eventIndex++;
  while ((Value() == EMPTY_WORD) && (storagePageIndex != lastEvent->storagePageIndex)) {
    storagePageIndex = (storagePageIndex + 1) % storage->PagesAmount();
    ReadStoragePage();
    eventIndex = 0;
  }
  return *this;
}

bool EventLogIterator::operator==(const EventLogIterator& other) const {
  return Index() == other.Index();
}

bool EventLogIterator::operator!=(const EventLogIterator& other) const {
  return !(*this == other);
}

const EventLogIterator& EventLogIterator::operator*() const {
  return *this;
}

uint32_t EventLogIterator::Index() const {
  return pageNumber * storage->PageSize() + eventIndex;
}

uint32_t EventLogIterator::Value() const {
  if (pageHeaderSize + eventIndex >= storage->PageSize()) return EMPTY_WORD; // page overflow check
  return storage->Read(storagePageIndex, pageHeaderSize + eventIndex);
}
