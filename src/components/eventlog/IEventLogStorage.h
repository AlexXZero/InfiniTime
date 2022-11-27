#pragma once

#include <cstdint>  // for uint32_t
#include <cstddef>  // for size_t

namespace Pinetime {
  namespace Components {
    class IEventLogStorage {
    public:
      IEventLogStorage() = default;

      /*
       * Returns amount of available pages for storing event log.
       * This amount should be a static value, i.e. it shouldn't be changed
       * during running application.
       */
      virtual size_t PagesAmount() const = 0;

      /*
       * Returns amount of words for storing event log (including reserved area).
       * All pages should have the same size and it should be a static value,
       * i.e. it shouldn't be changed during running application.
       */
      virtual size_t PageSize() const = 0;

      /*
       * Erase requested page. Erased page should contains 0xff bytes.
       */
      virtual void Erase(size_t page) = 0;

      /*
       * Write 32-bit word to the page.
       * Note: writing event logs shouldn't generate any errors since it might
       * be called from hard fault handler.
       */
      virtual void Write(size_t page, size_t wordOffset, uint32_t wordValue) = 0;

      /*
       * Read 32-bit word from the page.
       * Returns value of word or 0xffffffff if value is not available.
       */
      virtual uint32_t Read(size_t page, size_t wordOffset) const = 0;
    };
  }
}
