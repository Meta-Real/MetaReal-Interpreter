/*/
 * MetaReal version 1.0.0
 *
 * Memory Library version 1.0.0
 * Memory Stack form
 * 
 * Testing free fuction
/*/

#include <memory.h>
#include <stdio.h>

#define STACK_SIZE 1024

int main()
{
    puts("Memory Library version 1.0.0");
    puts("Memory Stack form");
    puts("Testing free function\n");

    stack_t stack;
    stack_init(&stack, STACK_SIZE);

    printf("stack: {data=%p, size=%llu, sp=%p, temp=%p}\n\n", stack.data, stack.size, stack.sp, stack.temp);

    void* ptr1 = stack_alloc(&stack, 12);
    void* ptr2 = stack_alloc(&stack, 29);
    stack_alloc(&stack, 71);

    printf("stack: {data=%p, size=%llu, sp=%p, temp=%p}\n\n", stack.data, stack.size, stack.sp, stack.temp);

    stack_free(&stack, ptr2);

    printf("stack: {data=%p, size=%llu, sp=%p, temp=%p}\n\n", stack.data, stack.size, stack.sp, stack.temp);

    stack_free(&stack, ptr1);

    printf("stack: {data=%p, size=%llu, sp=%p, temp=%p}\n", stack.data, stack.size, stack.sp, stack.temp);

    stack_delete(&stack);
    return 0;
}
