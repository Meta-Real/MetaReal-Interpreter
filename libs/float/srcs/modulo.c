/*/
 * MetaReal version 1.0.0
 *
 * Float Library version 1.0.0
 *
 * (num1) % (num2)
/*/

#include <float.h>

void float_modulo(float_p num1, float_p num2)
{
    mpfr_fmod(num1->value, num1->value, num2->value, MPFR_RNDN);
}
