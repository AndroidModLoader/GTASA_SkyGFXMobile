#include <cstring>

#define CAT(_to, _what)    strncatf(_to, _what, sizeof(_what))
#define CONCAT(_to, _what) concatf(_to, _what, sizeof(_what))

inline char* strcatf(char* dest, const char* src)
{
    char* dest2 = dest;
    while(*dest2 != '\0') ++dest2;
    return strcat(dest2, src);
}

inline char* strncatf(char* dest, const char* src, size_t count)
{
    char* dest2 = dest;
    while(*dest2 != '\0') ++dest2;
    return (char*)memcpy(dest2, src, count);
}

inline char* concatf(char* dest, const char* src, size_t count)
{
    return (char*)memcpy(dest, src, count) + count - 1;
}