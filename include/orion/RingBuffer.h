#pragma once

#include <vector>
#include <atomic>
#include <cstddef>
#include <cstring>
#include <algorithm>

namespace Orion {

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4324)
#endif

    template <typename T>
    class RingBuffer {
    public:
        explicit RingBuffer(size_t capacity) : capacity(capacity), buffer(capacity + 1) {
            readIndex.store(0, std::memory_order_relaxed);
            writeIndex.store(0, std::memory_order_relaxed);
        }

        bool push(const T& item) {
            const auto currentWrite = writeIndex.load(std::memory_order_relaxed);
            const auto nextWrite = (currentWrite + 1) % (capacity + 1);

            if (nextWrite == readIndex.load(std::memory_order_acquire)) {
                return false;
            }

            buffer[currentWrite] = item;
            writeIndex.store(nextWrite, std::memory_order_release);
            return true;
        }

        bool pop(T& item) {
            const auto currentRead = readIndex.load(std::memory_order_relaxed);

            if (currentRead == writeIndex.load(std::memory_order_acquire)) {
                return false;
            }

            item = buffer[currentRead];
            readIndex.store((currentRead + 1) % (capacity + 1), std::memory_order_release);
            return true;
        }

        bool isEmpty() const {
             return readIndex.load(std::memory_order_acquire) == writeIndex.load(std::memory_order_acquire);
        }

        size_t availableRead() const {
            const auto w = writeIndex.load(std::memory_order_acquire);
            const auto r = readIndex.load(std::memory_order_relaxed);
            if (w >= r) return w - r;
            return (capacity + 1) - (r - w);
        }

        size_t availableWrite() const {


            return capacity - availableRead();
        }

        bool write(const T* data, size_t count) {
            if (availableWrite() < count) return false;

            const auto w = writeIndex.load(std::memory_order_relaxed);
            const auto size = capacity + 1;

            const size_t chunk1 = std::min(count, size - w);
            const size_t chunk2 = count - chunk1;

            std::memcpy(&buffer[w], data, chunk1 * sizeof(T));
            if (chunk2 > 0) {
                std::memcpy(&buffer[0], data + chunk1, chunk2 * sizeof(T));
            }

            writeIndex.store((w + count) % size, std::memory_order_release);
            return true;
        }

        bool read(T* data, size_t count) {
            if (availableRead() < count) return false;

            const auto r = readIndex.load(std::memory_order_relaxed);
            const auto size = capacity + 1;

            const size_t chunk1 = std::min(count, size - r);
            const size_t chunk2 = count - chunk1;

            std::memcpy(data, &buffer[r], chunk1 * sizeof(T));
            if (chunk2 > 0) {
                std::memcpy(data + chunk1, &buffer[0], chunk2 * sizeof(T));
            }

            readIndex.store((r + count) % size, std::memory_order_release);
            return true;
        }

    private:
        size_t capacity;
        std::vector<T> buffer;
        alignas(64) std::atomic<size_t> readIndex;
        alignas(64) std::atomic<size_t> writeIndex;
    };

#ifdef _MSC_VER
#pragma warning(pop)
#endif

}
