/*/
 * MetaReal version 1.0.0
 *
 * Integer Library version 1.0.0
 *
 * (num1) ** (num2)
/*/

#include <int.h>
#include <stdlib.h>

int_p int_power(const int_p num1, const int_p num2)
{
    int_p res = malloc(sizeof(int_t));

    mpz_init(res->value);
    res->ref = 0;

    mpz_pow_ui(res->value, num1->value, mpz_get_ui(num2->value));

    return res;
}
