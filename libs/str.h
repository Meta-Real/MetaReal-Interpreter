/*/
 * MetaReal version 1.0.0
 *
 * String Library version 1.0.0
/*/

#ifndef __M_STR__
#define __M_STR__

#include <stdio.h>

struct __str__
{
    char* str;
    unsigned long long size;
};
typedef struct __str__ str_t;
typedef struct __str__* str_p;

str_p str_set(const str_p src);
str_p str_set_str(char* src, unsigned long long size);

void str_free(str_p str);

void str_print(FILE* stream, const str_p str, const char* end);
void str_label(FILE* stream, const str_p str, const char* end);

/* */

void str_concat(str_p str1, const str_p str2);
void str_concat_str(str_p str1, const char* str2);
void str_str_concat(const char* str1, str_p str2);
void str_concat_char(str_p str, char chr);
void str_char_concat(char chr, str_p str);

void str_remove(str_p str, unsigned long long pos);

void str_repeat(str_p str, unsigned long long count);

char str_equal(const str_p str1, const str_p str2);
char str_equal_char(const str_p str, char chr);

char str_nequal(const str_p str1, const str_p str2);
char str_nequal_char(const str_p str, char chr);

char str_contains(const str_p str1, const str_p str2);
char str_contains_str(const str_p str1, const char* str2);
char str_contains_char(const str_p str, char chr);

/* */

unsigned long long str_size(const str_p str);

#endif /* __M_STR__ */
