#pragma once

#include <cstdint>
#include <cstdlib>
#include <cassert>
#include <cstring>

struct Arena {
    unsigned char* buffer;
    size_t size;
    size_t offset;

    void init(size_t size_in_bytes) {
        size = size_in_bytes;
        buffer = (unsigned char*)malloc(size);
        offset = 0;
    }

    void destroy() {
        free(buffer);
        buffer = nullptr;
        offset = 0;
    }

    void* alloc(size_t size_to_alloc) {
        size_t padding = (8 - (offset % 8)) % 8;

        if (offset + padding + size_to_alloc > size) {
            assert(false && "[arena] arena out of memory!");
            return nullptr;
        }

        offset += padding;
        void* ptr = &buffer[offset];
        offset += size_to_alloc;

        return ptr;
    }

    void reset() {
        offset = 0;
    }

    template<typename T>
    T* alloc() {
        return (T*)alloc(sizeof(T));
    }

    template<typename T>
    T* alloc_array(size_t count) {
        return (T*)alloc(sizeof(T) * count);
    }
};