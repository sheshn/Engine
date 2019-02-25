extern "C"
{
    int _fltused = 0;

    void* __cdecl memset(void*, int, size_t);
    #pragma intrinsic(memset)

    void* __cdecl memcpy(void*, void*, size_t);
    #pragma intrinsic(memcpy)

    #pragma function(memset)
    void* __cdecl memset(void* dest, int value, size_t count)
    {
        unsigned char* bytes = (unsigned char*)dest;
        while (count--)
        {
            *bytes++ = (unsigned char)value;
        }
        return dest;
    }

    #pragma function(memcpy)
    void* __cdecl memcpy(void* dest, void* src, size_t size)
    {
        unsigned char* d = (unsigned char*)dest;
        unsigned char* s = (unsigned char*)src;
        while (size--)
        {
            *d++ = *s++;
        }
        return dest;
    }
}