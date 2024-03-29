#pragma once

#if defined(__clang__)
    #define READ_WRITE_BARRIER __asm__ __volatile__ ("" ::: "memory");
    #define atomic_compare_exchange(dest, old_value, new_value) __sync_bool_compare_and_swap((dest), (old_value), (new_value))
    #define atomic_increment(dest) __sync_add_and_fetch((dest), 1)
    #define atomic_decrement(dest) __sync_sub_and_fetch((dest), 1)
    #define atomic_add(dest, value) __sync_add_and_fetch((dest), (value))
#elif defined(_MSC_VER)
    #include <intrin.h>

    #define READ_WRITE_BARRIER _ReadWriteBarrier();
    #define atomic_compare_exchange(dest, old_value, new_value) InterlockedCompareExchange((dest), (new_value), (old_value)) == (old_value)
    #define atomic_increment(dest) InterlockedIncrement((dest))
    #define atomic_decrement(dest) InterlockedDecrement((dest))
    #define atomic_add(dest, value) InterlockedAdd64((dest), (value))
#endif

internal u32 most_significant_bit_index(u32 value)
{
    u32 index = 0;
#if defined(__clang__)
    index = 31 - __builtin_clz(value);
#elif defined(_MSC_VER)
    _BitScanReverse((DWORD*)&index, value);
#endif
    return index;
}

internal u32 least_significant_bit_index(u32 value)
{
    u32 index = 0;
#if defined(__clang__)
    index = __builtin_ctz(value);
#elif defined(_MSC_VER)
    _BitScanForward((DWORD*)&index, value);
#endif
    return index;
}