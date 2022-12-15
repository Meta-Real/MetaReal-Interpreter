/*/
 * MetaReal version 1.0.0
 *
 * Integer Library version 1.0.0
 *
 * (num1) >> (num2)
/*/

#include <int.h>

void int_ul_rshift(unsigned long num1, int_p num2)
{
    mpz_t num1i;
    mpz_init_set_ui(num1i, num1);

    mpz_fdiv_q_2exp(num2->value, num1i, mpz_get_ui(num2->value));

    mpz_clear(num1i);
}
