#pragma once

#include <cstring>  // for memset()
#include "IEventLogStorage.h"


namespace Pinetime {
  namespace Components {
    template <size_t PAGE_SIZE, size_t PAGES_AMOUNT> class EventLogRamStorage: public IEventLogStorage {
    public:
      EventLogRamStorage() {
        memset(pages, 0xff, sizeof(pages));
      }

      virtual size_t PagesAmount() const {
        return PAGES_AMOUNT;
      }

      virtual size_t PageSize() const {
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
      struct {
        uint32_t words[PAGE_SIZE];
      } pages[PAGES_AMOUNT];
    };
  }
}
