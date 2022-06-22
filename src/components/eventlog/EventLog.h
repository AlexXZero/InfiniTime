#pragma once

#include "Event.h"
#include "IEventLogStorage.h"
#include "components/datetime/DateTimeController.h"


namespace Pinetime {
  namespace System {
    class SystemTask;
  }

  namespace Components {
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

    class EventLogPage {
    public:
      EventLogPage() = default;
      EventLogPage(IEventLogStorage& storage, size_t storagePageIndex) :
          storage{&storage}, storagePageIndex{storagePageIndex} {
        word_t header = storage.Read(storagePageIndex, HEADER_WORD);
        //version = (header >> VERSION_SHIFT) & VERSION_MASK;
        pageNumber = (header >> PAGE_NUMBER_SHIFT) & PAGE_NUMBER_MASK;
        headerSize = (header >> HEADER_SIZE_SHIFT) & HEADER_SIZE_MASK;
      }
      EventLogPage(IEventLogStorage& storage, size_t storagePageIndex, uint16_t initPageNumber) :
          storage{&storage}, storagePageIndex{storagePageIndex}, pageNumber{initPageNumber} {}

      void Initialise() {
        headerSize = DEFAULT_HEADER_SIZE;
        storage->Write(storagePageIndex, HEADER_WORD,
              ((word_t)(CURRENT_VERSION & VERSION_MASK) << VERSION_SHIFT)
            | ((word_t)(headerSize & HEADER_SIZE_MASK) << HEADER_SIZE_SHIFT)
            | ((word_t)(pageNumber & PAGE_NUMBER_MASK) << PAGE_NUMBER_SHIFT));
      }

      bool IsEmpty() const {
        for (size_t word_index = 0; word_index < storage->PageSize(); word_index++) {
          if (storage->Read(storagePageIndex, word_index) != EMPTY_WORD) return false;
        }
        return true;
      }

      word_t Read(size_t eventIndex) const {
        if (headerSize + eventIndex >= storage->PageSize()) return EMPTY_WORD; // page overflow check
        return storage->Read(storagePageIndex, headerSize + eventIndex);
      }

      void Write(size_t eventIndex, word_t word) {
        if (headerSize + eventIndex >= storage->PageSize()) return; // page overflow check
        storage->Write(storagePageIndex, headerSize + eventIndex, word);
      }

      void Erase() {
        storage->Erase(storagePageIndex);
      }

      uint16_t Number() const { return pageNumber; }
      size_t Size() const { return storage->PageSize(); }
      uint32_t Index() const {return storagePageIndex;} // for debug
      bool IsFullEnough(size_t eventIndex) const {
        // Note: actual condition should looks like:
        // return eventIndex >= storage->PageSize() - (headerSize + EMERGENCY_EVENTS_AMOUNT);
        // But we should use modified version to be 100% sure we don't have negative value on right side.
        return headerSize + eventIndex + EMERGENCY_EVENTS_AMOUNT >= storage->PageSize();
      }


      EventLogPage& operator++() {
        pageNumber++; // Note: We shouldn't read it from flash to be able to compare element with "end" page
        storagePageIndex = (storagePageIndex + 1) % storage->PagesAmount();
        word_t header = storage->Read(storagePageIndex, HEADER_WORD);
        //version = (header >> VERSION_SHIFT) & VERSION_MASK;
        headerSize = (header >> HEADER_SIZE_SHIFT) & HEADER_SIZE_MASK;
        //pageNumber = (header >> PAGE_NUMBER_SHIFT) & PAGE_NUMBER_MASK;
        return *this;
      }
      bool operator==(const EventLogPage& other) const {return Number() == other.Number();}
      bool operator!=(const EventLogPage& other) const {return !(*this == other);}

    private:
      IEventLogStorage* storage;
      size_t storagePageIndex;
      uint16_t pageNumber;
      uint8_t headerSize;
    };

    class EventLogIterator {
    public:
      EventLogIterator() = default;
      EventLogIterator(const EventLogPage &currentPage, const EventLogPage &lastPage, uint16_t eventIndex = 0) :
          currentPage{currentPage}, lastPage{&lastPage}, eventIndex{eventIndex} {
        SkipEmptyWords();
      }

      EventLogIterator& operator++() {
        eventIndex++;
        SkipEmptyWords();
        return *this;
      }
      bool operator==(const EventLogIterator& other) const {return Index() == other.Index();}
      bool operator!=(const EventLogIterator& other) const {return !(*this == other);}
      const EventLogIterator& operator*() const { return *this; }
      uint32_t Index() const {return currentPage.Number() * currentPage.Size() + eventIndex;}
      uint32_t Value() const {return currentPage.Read(eventIndex);}
      uint32_t PageIndex() const {return currentPage.Index();} // for debug
      uint32_t PageNumber() const {return currentPage.Number();} // for debug
      uint32_t EventIndex() const {return eventIndex;} // for debug

    private:
      void SkipEmptyWords() {
        while ((Value() == EMPTY_WORD) && (currentPage != *lastPage)) {
          ++currentPage;
          eventIndex = 0;
        }
      }

      EventLogPage currentPage;
      const EventLogPage* lastPage;
      uint16_t eventIndex;
    };

    class EventLog {
    public:
      EventLog(Pinetime::System::SystemTask& systemTask, IEventLogStorage& storage, const Pinetime::Controllers::DateTime& dateTimeController);
      template <Event E, typename ...Params> void Write(Params...);
      void SwapPages();

      EventLogIterator begin() {return {EventLogPage(storage, (lastPage.Index() + 1) % storage.PagesAmount()), lastPage};}
      const EventLogIterator& end() {return lastEvent;}

    public: // FIXME: private
      void WriteRaw(uint32_t event);

      Pinetime::System::SystemTask& systemTask;
      IEventLogStorage& storage;
      const Pinetime::Controllers::DateTime& dateTimeController;
      std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> last_timediff;
      EventLogPage lastPage;
      EventLogIterator lastEvent;
      bool swapRequested = false;
    };
  }
}
