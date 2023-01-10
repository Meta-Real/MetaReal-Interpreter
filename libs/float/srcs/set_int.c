/*/
 * MetaReal version 1.0.0
 *
 * Float Library version 1.0.0
 *
 * Converts (str) into float based on (size)
/*/

#include <float.h>
#include <stdlib.h>

float_p float_set_int(const int_p src)
{
    float_p dst = malloc(sizeof(float_t));

    mpfr_init2(dst->value, prec_bit);
    mpfr_set_z(dst->value, src->value, MPFR_RNDN);

    dst->ref = 0;

    return dst;
}
