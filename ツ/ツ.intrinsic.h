#pragma once

#if defined(_MSC_VER)
    #include <intrin.h>

    #define READ_WRITE_BARRIER _ReadWriteBarrier();
    #define atomic_compare_exchange(dest, old_value, new_value) InterlockedCompareExchange((dest), (new_value), (old_value)) == (old_value)
    #define atomic_increment(dest) InterlockedIncrement((dest))
    #define atomic_decrement(dest) InterlockedDecrement((dest))
    #define atomic_add(dest, value) InterlockedAdd64((dest), (value))
#elif defined(__clang__)
    #define READ_WRITE_BARRIER __asm__ __volatile__ ("" ::: "memory");
    #define atomic_compare_exchange(dest, old_value, new_value) __sync_bool_compare_and_swap((dest), (old_value), (new_value))
    #define atomic_increment(dest) __sync_add_and_fetch((dest), 1)
    #define atomic_decrement(dest) __sync_sub_and_fetch((dest), 1)
    #define atomic_add(dest, value) __sync_add_and_fetch((dest), (value))
#endif