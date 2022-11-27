#pragma once

#include "Event.h"
#include "IEventLogStorage.h"
#include <chrono>

namespace Pinetime {
  namespace System {
    class SystemTask;
  }

  namespace Controllers {
    class DateTime;
  }

  namespace Components {
    class EventLogIterator {
    public:
      EventLogIterator() = default;
      EventLogIterator(const EventLogIterator& lastEvent, IEventLogStorage& storage,
                       size_t storagePageIndex = 0, uint16_t eventIndex = 0);
      EventLogIterator& operator++();
      bool operator==(const EventLogIterator& other) const;
      bool operator!=(const EventLogIterator& other) const;
      const EventLogIterator& operator*() const;
      uint32_t Index() const;
      uint32_t Value() const;

    private:
      // Allow EventLog access to private fields for creating an iterator to first element.
      friend class EventLog;

      void ReadStoragePage();

      // The only reason to use pointers here (instead of references) is to support default constructor
      const EventLogIterator* lastEvent;
      IEventLogStorage* storage;
      size_t storagePageIndex;
      uint8_t pageHeaderSize;
      uint16_t pageNumber;
      uint16_t eventIndex;
    };

    class EventLog {
    public:
      EventLog(Pinetime::System::SystemTask& systemTask, IEventLogStorage& storage, const Pinetime::Controllers::DateTime& dateTimeController);
      template <Event E, typename ...Params> void Write(Params...);
      void SwapPages();
      EventLogIterator begin() const;
      const EventLogIterator& end() const;
      void EraseAll();

    private:
      void WriteRaw(uint32_t event);
      void WriteEvent(uint32_t event);
      static size_t FindLastPage(IEventLogStorage& storage);

      Pinetime::System::SystemTask& systemTask;
      IEventLogStorage& storage;
      const Pinetime::Controllers::DateTime& dateTimeController;
      std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> last_timediff;
      EventLogIterator lastEvent;
      bool swapRequested = false;
    };
  }
}
