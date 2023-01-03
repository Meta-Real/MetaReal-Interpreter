/*/
 * MetaReal version 1.0.0
/*/

#ifndef __M_FUNCTION__
#define __M_FUNCTION__

#include <interpreter/context.h>

struct __arg_value_t__
{
    value_t value;
    unsigned long long index;
};
typedef struct __arg_value_t__ arg_value_t;
typedef struct __arg_value_t__* arg_value_p;

struct __func_t__
{
    unsigned char type;

    unsigned long long min_size;
    unsigned long long max_size;

    arg_value_p args;
    unsigned long long size;

    context_t context;
    body_t body;
};
typedef struct __func_t__ func_t;
typedef struct __func_t__* func_p;

func_p func_set(unsigned char type, unsigned long long min_size, unsigned long long max_size,
    arg_value_p args, unsigned long long size,
    context_p context, body_p body);

func_p func_copy(const func_p func);

void func_free(func_p func);

void func_print(FILE* stream, const func_p func, const char* end);

/* */

void arg_value_free(arg_value_p args, unsigned long long size);

#endif /* __M_FUNCTION__ */
