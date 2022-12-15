#pragma once

#include "IEventLogStorage.h"
#include "drivers/InternalFlash.h"

namespace Pinetime {
  namespace Components {
    class EventLogInternalFlashStorage: public IEventLogStorage {
    public:
      virtual constexpr size_t PagesAmount() const {
        return PAGES_AMOUNT;
      }

      virtual constexpr size_t PageSize() const {
        return WORDS_PER_PAGE;
      }

      virtual void Erase(size_t page) {
        Drivers::InternalFlash::ErasePage((uintptr_t)&pages[page]);
      }

      virtual void Write(size_t page, size_t wordOffset, uint32_t wordValue) {
        Drivers::InternalFlash::WriteWord((uintptr_t)&pages[page].words[wordOffset], wordValue);
      }

      virtual uint32_t Read(size_t page, size_t wordOffset) const {
        return pages[page].words[wordOffset];
      }

    private:
      static constexpr size_t WORDS_PER_PAGE = 0x400;
      static constexpr size_t PAGES_AMOUNT = 3;
      static constexpr uint32_t FLASH_OFFSET = 0x7d000;
      struct {
        uint32_t words[WORDS_PER_PAGE];
      } *pages = (decltype(pages))FLASH_OFFSET;
    };
  }
}
