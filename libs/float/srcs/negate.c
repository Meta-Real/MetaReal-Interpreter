/*/
 * MetaReal version 1.0.0
 *
 * Float Library version 1.0.0
 *
 * -(num)
/*/

#include <float.h>
#include <stdlib.h>

float_p float_negate(const float_p num)
{
    float_p res = malloc(sizeof(float_t));

    mpfr_init2(res->value, prec_bit);
    res->ref = 0;

    mpfr_neg(res->value, num->value, MPFR_RNDN);

    return res;
}
