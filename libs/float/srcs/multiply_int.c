/*/
 * MetaReal version 1.0.0
 *
 * Float Library version 1.0.0
 *
 * (num1) * (num2)
/*/

#include <float.h>
#include <stdlib.h>

float_p float_multiply_int(const float_p num1, const int_p num2)
{
    float_p res = malloc(sizeof(float_t));

    mpfr_init2(res->value, float_prec_bit);
    res->ref = 0;

    mpfr_mul_z(res->value, num1->value, num2->value, MPFR_RNDN);

    return res;
}
