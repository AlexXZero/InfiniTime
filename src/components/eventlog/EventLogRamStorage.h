#pragma once

#include <cstring>  // for memset()
#include "IEventLogStorage.h"


namespace Pinetime {
  namespace Components {
    class EventLogRamStorage: public IEventLogStorage {
    public:
      EventLogRamStorage() {
        memset(pages, 0xff, sizeof(pages));
      }

      virtual constexpr size_t PagesAmount() const {
        return PAGES_AMOUNT;
      }

      virtual constexpr size_t PageSize() const {
        return PAGE_SIZE;
      }

      virtual void Erase(size_t page) {
        memset(&pages[page], 0xff, sizeof(pages[page]));
      }

      virtual void Write(size_t page, size_t wordOffset, uint32_t wordValue) {
        pages[page].words[wordOffset] &= wordValue;
      }

      virtual uint32_t Read(size_t page, size_t wordOffset) const {
        return pages[page].words[wordOffset];
      }

    private:
      static constexpr size_t PAGE_SIZE = 0x100;
      static constexpr size_t PAGES_AMOUNT = 3;
      struct {
        uint32_t words[PAGE_SIZE];
      } pages[PAGES_AMOUNT];
    };
  }
}
