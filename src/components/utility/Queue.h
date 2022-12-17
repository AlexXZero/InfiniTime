#pragma once

namespace Pinetime {
  namespace Utility {
    /*
     * Usage: declare element type which should contains public `T* next;` field
     *        (or private, then `HeaplessQueue<T>` should be a friend class)
     */
    template <typename T>
    class HeaplessSortedQueue {
    public:
      T* begin() {
        return head;
      }
      const T* begin() const {
        return head;
      }
      constexpr T* end() const {
        return nullptr;
      }
      void Emplace(T* element) {
        T** it = &head;
        while ((*it != end()) && (*element >= **it)) {
          it = &(*it)->p_next;
        }

        element->p_next = *it;
        *it = element;
      }
      void Remove(const T* element) {
        for (T** it = &head; *it != end(); it = &(*it)->p_next) {
          if (*it == element) {
            *it = element->p_next;
            break;
          }
        }
      }
      void Pop() {
        head = head->p_next;
      }
      bool Empty() const {
        return begin() == end();
      }

    private:
      T* head = nullptr;
    };
  }
}
