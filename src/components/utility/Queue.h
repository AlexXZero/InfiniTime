#pragma once

#include <functional> // for std::less

namespace Pinetime {
  namespace Utility {
    template <typename T>
    class HeaplessQueue {
    public:
      T& Front() const {
        return *head;
      }

      template<typename Compare = std::less<T>>
      void InsertSorted(T& element, Compare compare = Compare{}) {
        T** it = &head;
        while ((*it != nullptr) && !compare(element, **it)) {
          it = &(*it)->p_next;
        }

        element.p_next = *it;
        *it = &element;
      }
      void Insert(T& element) {
        element.p_next = head;
        head = &element;
      }

      /*
       * No elements are copied or moved, only the internal pointers of the
       * container nodes are repointed (rebalancing may occur, as with erase()).
       *
       * Extracting a node invalidates only the iterators to the extracted element.
       * Pointers and references to the extracted element remain valid, but
       * cannot be used while element is owned by a node handle: they become
       * usable if the element is inserted into a container.
       */
      void Extract(const T& element) {
        for (T** it = &head; *it != nullptr; it = &(*it)->p_next) {
          if (*it == &element) {
            *it = element.p_next;
            break;
          }
        }
      }
      void Extract() {
        head = head->p_next;
      }

      bool Empty() const {
        return head == nullptr;
      }

    private:
      T* head = nullptr;
    };

    template <typename T>
    class HeaplessQueueObject {
    protected:
      // C.35: A base class destructor should be either public and virtual, or protected and non-virtual
      // See https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#c35-a-base-class-destructor-should-be-either-public-and-virtual-or-protected-and-non-virtual
      ~HeaplessQueueObject() = default;

    private:
      T* p_next = nullptr;
      friend class HeaplessQueue<T>;
    };
  }
}
