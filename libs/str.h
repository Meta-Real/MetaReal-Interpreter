/*/
 * MetaReal version 1.0.0
 *
 * String Library version 1.0.0
/*/

#ifndef __M_STR__
#define __M_STR__

#include <memory.h>
#include <stdio.h>

struct __str__
{
    char* str;
    unsigned long long size;
};
typedef struct __str__ str_t;
typedef struct __str__* str_p;

str_p str_set(str_p src, cellular_p cellular, heap_p heap);
str_p str_set_str(const char* src, unsigned long long size, cellular_p cellular, heap_p heap);

void str_free(str_p str, cellular_p cellular, heap_p heap);

void str_print(FILE* stream, str_p str, const char* end);

/* */

void str_concat(str_p str1, str_p str2, heap_p heap);
void str_remove(str_p str, unsigned long long pos, heap_p heap);
void str_repeat(str_p str, unsigned long long count, heap_p heap);

char str_equal(str_p str1, str_p str2);
char str_nequal(str_p str1, str_p str2);

char str_contains(str_p str, char chr);

#endif /* __M_STR__ */
