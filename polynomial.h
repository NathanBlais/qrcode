#ifndef _POLYNOMIAL_H
#define _POLYNOMIAL_H

#include <stdint.h>

/**
 * This structure represents a polynomial
 * where the coefficients are elements of the 2^8
 * Galois field used by QR codes. By convention,
 * the coefficients are in reverse order, i.e.
 * coefficients[0] is the coefficient for X^(n_coefficients - 1)
 * while coefficients[n_coefficients - 1] is the constant term
 * of the polynomial.
 */
struct gf_polynomial {
    u_int8_t* coefficients;
    unsigned int n_coefficients;
};


/**
 * Given an array of coefficients representing a polynomial
 * in a 2^8 Galois field and an x value, returns the value
 * obtained when evaluating the polynomial.
 *
 * @param p The polynomial to evaluate
 * @param x The value for which to evaluate the polynomial
 * @return The value of the polynomial calculated for the given x
 */
u_int8_t evaluate_polynomial(struct gf_polynomial* p, u_int8_t x);


/**
 * Allocates a polynomial for the given number of coefficients.
 *
 * @param n_coefficients The number of coefficients
 * @param coefficients If not null, the given array will be copied into the
 *                     newly created polynomial; otherwise, the coefficients are
 *                     initalized to 0
 * @return The polynomial or NULL in case of memory allocation error
 */
struct gf_polynomial* new_gf_polynomial(unsigned int n_coefficients, u_int8_t* coefficients);


/**
 * Frees the memory associated to the given polynomial.
 */
void free_gf_polynomial(struct gf_polynomial* p);

#endif