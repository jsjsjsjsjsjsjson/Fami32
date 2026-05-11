#include <stdio.h>

template <typename T, size_t Size>
class ringbuf {
public:
    ringbuf() : head(0), tail(0), full(false) {
        memset(buffer, 0, sizeof(buffer));
    }

    void push(const T& value) {
        buffer[tail] = value;
        if (full) {
            head = (head + 1) % Size;
        }
        tail = (tail + 1) % Size;
        full = tail == head;
    }

    size_t size() const {
        if (full) {
            return Size;
        }
        if (tail >= head) {
            return tail - head;
        }
        return Size + tail - head;
    }

    bool empty() const {
        return (!full && head == tail);
    }

    bool isFull() const {
        return full;
    }

    T& operator[](size_t index) {
        return buffer[(head + index) % Size];
    }

    const T& operator[](size_t index) const {
        return buffer[(head + index) % Size];
    }

private:
    T buffer[Size];
    size_t head, tail;
    bool full;
};