/*/
 * MetaReal version 1.0.0
 *
 * Complex Library version 1.0.0
 *
 * Frees (num) from (cellular) and its data from (heap)
/*/

#include <complex.h>
#include <memory.h>

void complex_free(complex_p num)
{
    mpc_clear(num->value);
    cellular_free(&memory.complex_cellular, num);
}
