/*/
 * MetaReal version 1.0.0
 *
 * Memory Library version 1.0.0
 * Memory Stack form
 * 
 * Deletes (stack) and all temporary stacks with it
/*/

#include <memory.h>
#include <stdlib.h>

void stack_temp_delete(stack_p temp);

void stack_delete(stack_p stack)
{
    free(stack->data);

    if (stack->temp)
        stack_temp_delete(stack->temp);
}

void stack_temp_delete(stack_p temp)
{
    if (temp->temp)
        stack_temp_delete(temp->temp);

    free(temp->data);
    free(temp);
}
