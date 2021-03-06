#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include "big5.h"
#include "bitstream.h"
#include "bitstreamdecoder.h"
#include "eci.h"
#include "euc_kr.h"
#include "galoisfield.h"
#include "gb18030.h"
#include "reedsolomon.h"


/**
 * This example come from:
 * https://en.wikiversity.org/wiki/Reed%E2%80%93Solomon_codes_for_coders
 *
 * This corresponds to test_data + the 10 error codewords obtained when applying
 * medium error correction level.
 */
static u_int8_t test_block[] = {
    // data
    0x40, 0xd2, 0x75, 0x47, 0x76, 0x17, 0x32, 0x06, 0x27, 0x26, 0x96, 0xc6, 0xc6, 0x96, 0x70, 0xec,
    // error correction
    0xbc, 0x2a, 0x90, 0x13, 0x6b, 0xaf, 0xef, 0xfd, 0x4b, 0xe0
};

static char* decoded_text = "'Twas brillig";

// This array corresponds to the text "12345678" encoded with a numeric segment
static u_int8_t numeric_example[] = {
    // data
    0x10, 0x20, 0x7b, 0x72, 0x27, 0x00, 0xec, 0x11, 0xec, 0x11, 0xec, 0x11, 0xec, 0x11, 0xec, 0x11, 0xec, 0x11, 0xec
};

static char* decoded_numeric_example = "12345678";


// This array corresponds to some text containing Japanese characters encoded with a kanji segment
static u_int8_t kanji_example[] = {
    // data
    0x80, 0x70, 0xd9, 0x06, 0x88, 0x31, 0xc1, 0x8a, 0x0e, 0x08, 0x72, 0x40, 0x0c, 0x83, 0x61, 0xa1,
    0x4d, 0x0e, 0x8e, 0x8e, 0x07, 0x45, 0xe5, 0xee, 0xee, 0xee, 0xe5, 0xcc, 0x4d, 0x2c, 0x6c, 0x6c,
    0x2d, 0xac, 0xae, 0x4c, 0x25, 0xcc, 0x6d, 0xed, 0xa5, 0xe0, 0xec, 0x11, 0xec, 0x11, 0xec, 0x11,
    0xec, 0x11, 0xec, 0x11, 0xec, 0x11, 0xec
};

static u_int8_t decoded_kanji_example[] = {
    0xe3, 0x83, 0x93, 0xe3, 0x83, 0x83, 0xe3, 0x82, 0xaf, 0xe3, 0x82, 0xab, 0xe3, 0x83, 0xa1, 0xe3,
    0x83, 0xa9, 0xef, 0xbc, 0x9a, 0x0d, 0x0a, 0x68, 0x74, 0x74, 0x70, 0x3a, 0x2f, 0x2f, 0x77, 0x77,
    0x77, 0x2e, 0x62, 0x69, 0x63, 0x63, 0x61, 0x6d, 0x65, 0x72, 0x61, 0x2e, 0x63, 0x6f, 0x6d, 0x2f,
    0
};


// This the utf8 version of some chinese characters, followed by versions of it
// in various encodings
static u_int8_t chinese_utf8[] = {
    0xe4, 0xbb, 0xa5, 0xe6, 0x98, 0x8e, 0xe9, 0x9f, 0xb3, 0xe9, 0x9f, 0xbb, 0xe7, 0x94, 0xa8, 0xe4,
    0xb9, 0x8b, 0xe4, 0xb8, 0x8d, 0xe6, 0x9b, 0xb4, 0xe8, 0x87, 0xb3, 0xe6, 0xb0, 0x91, 0xe5, 0xad,
    0xa3, 0
};

static u_int8_t chinese_gb18030[] = {
    0xD2, 0xD4, 0xC3, 0xF7, 0xD2, 0xF4, 0xED, 0x8D, 0xD3, 0xC3, 0xD6, 0xAE, 0xB2, 0xBB, 0xB8, 0xFC,
    0xD6, 0xC1, 0xC3, 0xF1, 0xBC, 0xBE
};

static u_int8_t chinese_big5[] = {
    0xA5, 0x48, 0xA9, 0xFA, 0xAD, 0xB5, 0xC3, 0xFD, 0xA5, 0xCE, 0xA4, 0xA7, 0xA4, 0xA3, 0xA7, 0xF3,
    0xA6, 0xDC, 0xA5, 0xC1, 0xA9, 0x75
};

static u_int8_t chinese_euc_kr[] = {
    0xEC, 0xA4, 0xD9, 0xA5, 0xEB, 0xE5, 0xEA, 0xA4, 0xE9, 0xC4, 0xF1, 0xFD, 0xDC, 0xF4, 0xCC, 0xDA,
    0xF2, 0xB8, 0xDA, 0xC5, 0xCC, 0xF9
};

static u_int8_t chinese_shift_jis[] = {
    0x88, 0xC8, 0x96, 0xBE, 0x89, 0xB9, 0x89, 0x43, 0x97, 0x70, 0x94, 0x56, 0x95, 0x73, 0x8D, 0x58,
    0x8E, 0x8A, 0x96, 0xAF, 0x8B, 0x47
};



/**
 * Verifies that the syndromes are all 0 when there is no error.
 */
static int test_reed_solomon1() {
    struct gf_polynomial* message = new_gf_polynomial(26, test_block);
    struct gf_polynomial* syndromes = new_gf_polynomial(10, NULL);

    int result = calculate_syndromes(message, syndromes);
    free_gf_polynomial(message);
    free_gf_polynomial(syndromes);
    return result == 0;
}


/**
 * Verifies that the syndromes are not all 0 when there is an error.
 */
static int test_reed_solomon2() {
    struct gf_polynomial* message = new_gf_polynomial(26, test_block);
    message->coefficients[0] = 0;
    struct gf_polynomial* syndromes = new_gf_polynomial(10, NULL);

    int result = calculate_syndromes(message, syndromes);
    free_gf_polynomial(message);
    free_gf_polynomial(syndromes);
    return result != 0;
}


static int test_get_degree() {
    struct gf_polynomial* p = new_gf_polynomial(5, (uint8_t[]){ 0, 0, 0, 0, 0 });
    int ok = 1;
    ok = ok && (0 == get_degree(p));

    p->coefficients[2] = 1;
    ok = ok && (2 == get_degree(p));

    p->coefficients[1] = 1;
    ok = ok && (3 == get_degree(p));

    free_gf_polynomial(p);
    return ok;
}


static int test_get_coefficient() {
    struct gf_polynomial* a = new_gf_polynomial(7, (uint8_t[]){ 0, 1, 1, 0, 1, 1, 1 });
    int ok = 1 == get_coefficient(a, 0)
        && 1 == get_coefficient(a, 1)
        && 1 == get_coefficient(a, 2)
        && 0 == get_coefficient(a, 3)
        && 1 == get_coefficient(a, 4)
        && 1 == get_coefficient(a, 5)
        && 0 == get_coefficient(a, 6);
    free_gf_polynomial(a);
    return ok;
}


static int test_set_coefficient() {
    struct gf_polynomial* a = new_gf_polynomial(7, (uint8_t[]){ 0, 1, 1, 0, 1, 1, 1 });
    int ok = 1 == get_coefficient(a, 1);
    set_coefficient(a, 1, 0);
    ok = ok && 0 == get_coefficient(a, 1);
    free_gf_polynomial(a);
    return ok;
}


static int test_equal_polynomials() {
    struct gf_polynomial* a = new_gf_polynomial(8, (uint8_t[]){ 0, 0, 1, 1, 0, 1, 1, 1 });
    struct gf_polynomial* b = new_gf_polynomial(6, (uint8_t[]){ 1, 1, 0, 1, 1, 1 });
    int ok = equal_polynomials(a, b);
    free_gf_polynomial(a);
    free_gf_polynomial(b);
    return ok;
}


static int test_add_polynomials() {
    struct gf_polynomial* a = new_gf_polynomial(7, (uint8_t[]){       1, 1, 0, 0, 0, 1, 1 });
    struct gf_polynomial* b = new_gf_polynomial(9, (uint8_t[]){ 0, 0, 0, 1, 1, 0, 0, 1, 0 });
    struct gf_polynomial* expected = new_gf_polynomial(7, (uint8_t[]){ 1, 0, 1, 0, 0, 0, 1 });
    struct gf_polynomial* c = add_polynomials(a, b);
    int ok = equal_polynomials(expected, c);

    free_gf_polynomial(c);
    c = add_polynomials(b, a);
    ok = ok && equal_polynomials(expected, c);

    free_gf_polynomial(a);
    free_gf_polynomial(b);
    free_gf_polynomial(c);
    free_gf_polynomial(expected);
    return ok;
}


static int test_add_polynomials2() {
    struct gf_polynomial* a = new_gf_polynomial(7, (uint8_t[]){           1, 1, 0, 0, 0, 1, 1 });
    struct gf_polynomial* b = new_gf_polynomial(8, (uint8_t[]){        1, 1, 0, 0, 0, 1, 1, 0 });
    struct gf_polynomial* expected = new_gf_polynomial(8, (uint8_t[]){ 1, 0, 1, 0, 0, 1, 0, 1 });
    struct gf_polynomial* c = add_polynomials(a, b);

    int ok = equal_polynomials(expected, c);

    free_gf_polynomial(c);
    c = add_polynomials(b, a);
    ok = ok && equal_polynomials(expected, c);

    free_gf_polynomial(a);
    free_gf_polynomial(b);
    free_gf_polynomial(c);
    free_gf_polynomial(expected);
    return ok;
}


/**
 * Verifies that given a polynomial A, we have A.X+A = A.(X+1)
 */
static int test_multiply_polynomials() {
    struct gf_polynomial* a = new_gf_polynomial(7, (uint8_t[]){ 1, 1, 0, 0, 0, 1, 1 });
    struct gf_polynomial* x = new_gf_polynomial(2, (uint8_t[]){ 1, 0 });
    struct gf_polynomial* x_1 = new_gf_polynomial(2, (uint8_t[]){ 1, 1 });
    struct gf_polynomial* ax = multiply_polynomials(a, x);
    struct gf_polynomial* ax_a1 = add_polynomials(ax, a);
    struct gf_polynomial* ax_a2 = multiply_polynomials(a, x_1);
    struct gf_polynomial* expected = new_gf_polynomial(8, (uint8_t[]){ 1, 0, 1, 0, 0, 1, 0, 1 });

    int ok = equal_polynomials(expected, ax_a1);
    ok = ok && equal_polynomials(expected, ax_a2);

    free_gf_polynomial(a);
    free_gf_polynomial(x);
    free_gf_polynomial(x_1);
    free_gf_polynomial(ax);
    free_gf_polynomial(ax_a1);
    free_gf_polynomial(ax_a2);
    free_gf_polynomial(expected);
    return ok;
}


static int test_multiply_by_monomial() {
    struct gf_polynomial* a = new_gf_polynomial(10, (uint8_t[]) {
        0,
        gf_power(142),
        gf_power(151),
        gf_power(92),
        gf_power(169),
        gf_power(212),
        gf_power(51),
        gf_power(213),
        gf_power(205),
        gf_power(34)
    });
    struct gf_polynomial* x = new_gf_polynomial(2, (uint8_t[]){ gf_power(156), 0 });
    struct gf_polynomial* product = multiply_polynomials(a, x);
    struct gf_polynomial* expected = new_gf_polynomial(10, (uint8_t[]) {
        gf_multiply(gf_power(156), gf_power(142)),
        gf_multiply(gf_power(156), gf_power(151)),
        gf_multiply(gf_power(156), gf_power(92)),
        gf_multiply(gf_power(156), gf_power(169)),
        gf_multiply(gf_power(156), gf_power(212)),
        gf_multiply(gf_power(156), gf_power(51)),
        gf_multiply(gf_power(156), gf_power(213)),
        gf_multiply(gf_power(156), gf_power(205)),
        gf_multiply(gf_power(156), gf_power(34)),
        0
    });

    int ok = equal_polynomials(expected, product);

    free_gf_polynomial(a);
    free_gf_polynomial(x);
    free_gf_polynomial(expected);
    return ok;
}


/**
 * Verifies that dividing a polynomial equal to A.(X+1) by
 * (X+1) given A as the quotient and 0 as the remainder.
 */
static int test_divide_polynomials() {
    struct gf_polynomial* a = new_gf_polynomial(8, (uint8_t[]){ 1, 0, 1, 0, 0, 1, 0, 1 });
    struct gf_polynomial* b = new_gf_polynomial(2, (uint8_t[]){ 1, 1 });
    struct gf_polynomial* expected_q = new_gf_polynomial(7, (uint8_t[]){ 1, 1, 0, 0, 0, 1, 1 });
    struct gf_polynomial* expected_r = new_gf_polynomial(1, (uint8_t[]){ 0 });

    struct gf_polynomial* quotient;
    struct gf_polynomial* remainder;
    int res = divide_polynomials(a, b, &quotient, &remainder);
    int ok = res == 1
        && equal_polynomials(expected_q, quotient)
        && equal_polynomials(expected_r, remainder);

    free_gf_polynomial(a);
    free_gf_polynomial(b);
    free_gf_polynomial(expected_q);
    free_gf_polynomial(expected_r);
    free_gf_polynomial(quotient);
    free_gf_polynomial(remainder);
    return ok;
}


/**
 * Verifies that dividing a polynomial equal to A.(X+1)+1 by
 * (X+1) given A as the quotient and 1 as the remainder.
 */
static int test_divide_polynomials2() {
    struct gf_polynomial* a = new_gf_polynomial(8, (uint8_t[]){ 1, 0, 1, 0, 0, 1, 0, 0 });
    struct gf_polynomial* b = new_gf_polynomial(2, (uint8_t[]){ 1, 1 });
    struct gf_polynomial* expected_q = new_gf_polynomial(7, (uint8_t[]){ 1, 1, 0, 0, 0, 1, 1 });
    struct gf_polynomial* expected_r = new_gf_polynomial(1, (uint8_t[]){ 1 });

    struct gf_polynomial* quotient;
    struct gf_polynomial* remainder;
    int res = divide_polynomials(a, b, &quotient, &remainder);

    int ok = res == 1
        && equal_polynomials(expected_q, quotient)
        && equal_polynomials(expected_r, remainder);

    free_gf_polynomial(a);
    free_gf_polynomial(b);
    free_gf_polynomial(expected_q);
    free_gf_polynomial(expected_r);
    free_gf_polynomial(quotient);
    free_gf_polynomial(remainder);
    return ok;
}


int test_calculate_sigma_omega() {
    struct gf_polynomial* codewords = new_gf_polynomial(26, test_block);
    if (codewords == NULL) {
        return 0;
    }
    struct gf_polynomial* syndromes = new_gf_polynomial(10, NULL);
    if (syndromes == NULL) {
        free_gf_polynomial(codewords);
        return 0;
    }

    codewords->coefficients[0] = 0;
    if (0 == calculate_syndromes(codewords, syndromes)) {
        free_gf_polynomial(codewords);
        free_gf_polynomial(syndromes);
        return 0;
    }

    struct gf_polynomial* sigma;
    struct gf_polynomial* omega;

    int ok = SUCCESS == calculate_sigma_omega(syndromes, 10, &sigma, &omega);
    if (!ok) {
        free_gf_polynomial(codewords);
        free_gf_polynomial(syndromes);
        return 0;
    }

    struct gf_polynomial* expected_sigma = new_gf_polynomial(2, (u_int8_t[]){ gf_power(25), 1 });
    if (expected_sigma == NULL) {
        free_gf_polynomial(codewords);
        free_gf_polynomial(syndromes);
        return 0;
    }

    struct gf_polynomial* expected_omega = new_gf_polynomial(1, (u_int8_t[]){ gf_power(6) });
    if (expected_omega == NULL) {
        free_gf_polynomial(codewords);
        free_gf_polynomial(syndromes);
        free_gf_polynomial(expected_omega);
        return 0;
    }

    ok = equal_polynomials(expected_sigma, sigma) && equal_polynomials(expected_omega, omega);

    free_gf_polynomial(codewords);
    free_gf_polynomial(syndromes);
    free_gf_polynomial(sigma);
    free_gf_polynomial(omega);
    free_gf_polynomial(expected_sigma);
    free_gf_polynomial(expected_omega);
    return ok;
}


int test_find_error_locations() {
    struct gf_polynomial* sigma = new_gf_polynomial(2, (u_int8_t[]){ gf_power(25), 1 });
    if (sigma == NULL) {
        return 0;
    }
    u_int8_t t[1];
    int ok = SUCCESS == find_error_locations(sigma, t) && gf_log(t[0]) == 25;

    free_gf_polynomial(sigma);
    return ok;
}


int test_find_error_magnitudes() {
    struct gf_polynomial* omega = new_gf_polynomial(1, (u_int8_t[]){ gf_power(6) });
    if (omega == NULL) {
        return 0;
    }
    u_int8_t error_locations[1] = { gf_power(25) };
    u_int8_t error_magnitudes[1];
    find_error_magnitudes(omega, 1, error_locations, error_magnitudes);
    free_gf_polynomial(omega);
    return 0x40 == error_magnitudes[0];
}


// Test with no error in the message
int test_error_correction() {
    struct block b;
    b.codewords = test_block;
    b.n_data_codewords = 16;
    b.n_error_correction_codewords = 10;

    return 0 == error_correction(&b);
}


// Test with one error in the message
int test_error_correction2() {
    struct block b;
    b.codewords = (u_int8_t*)malloc(26 * sizeof(u_int8_t));
    if (b.codewords == NULL) {
        return 0;
    }
    memcpy(b.codewords, test_block, 26);
    b.n_data_codewords = 16;
    b.n_error_correction_codewords = 10;

    b.codewords[1] ^= 63;

    int ok = 1 == error_correction(&b) && b.codewords[1] == test_block[1];
    free(b.codewords);
    return ok;
}

// Test with two errors in the message
int test_error_correction3() {
    struct block b;
    b.codewords = (u_int8_t*)malloc(26 * sizeof(u_int8_t));
    if (b.codewords == NULL) {
        return 0;
    }
    memcpy(b.codewords, test_block, 26);
    b.n_data_codewords = 16;
    b.n_error_correction_codewords = 10;

    b.codewords[1] ^= 63;
    b.codewords[14] ^= 33;

    int ok = 2 == error_correction(&b) && b.codewords[1] == test_block[1] && b.codewords[14] == test_block[14];
    free(b.codewords);
    return ok;
}


// Test with too many errors in the message
int test_error_correction4() {
    struct block b;
    b.codewords = (u_int8_t*)malloc(26 * sizeof(u_int8_t));
    if (b.codewords == NULL) {
        return 0;
    }
    memcpy(b.codewords, test_block, 26);
    b.n_data_codewords = 16;
    b.n_error_correction_codewords = 10;

    for (unsigned int i = 1 ; i < 26 ; i++) {
        b.codewords[i] ^= i;
    }

    int res = error_correction(&b);
    free(b.codewords);

    return res == DECODING_ERROR;
}


int test_bitstream() {
    struct bitstream* s = new_bitstream(4);
    if (s == NULL) {
        return 0;
    }
    s->bytes[0] = 0x12;
    s->bytes[1] = 0x34;
    s->bytes[2] = 0x56;
    s->bytes[3] = 0x78;

    int ok = 32 == remaining_bits(s);
    ok = ok && 0x1 == read_bits(s, 4) && 28 == remaining_bits(s);
    ok = ok && 0x234 == read_bits(s, 12) && 16 == remaining_bits(s);
    ok = ok && 0x567 == read_bits(s, 12) && 4 == remaining_bits(s);
    ok = ok && 2 == read_bits(s, 2) && 2 == remaining_bits(s);
    ok = ok && 0 == read_bits(s, 2) && 0 == remaining_bits(s);

    free_bitstream(s);
    return ok;
}


int test_decode_bitstream() {
    struct bitstream* s = new_bitstream(16);
    if (s == NULL) {
        return 0;
    }
    memcpy(s->bytes, test_block, 16);

    u_int8_t* decoded;
    int n = decode_bitstream(s, 1, &decoded);
    free_bitstream(s);
    if (n < 0) {
        return 0;
    }

    int ok = 0 == memcmp(decoded, decoded_text, n);
    free(decoded);
    return ok;
}


int test_decode_bitstream_numeric() {
    struct bitstream* s = new_bitstream(18);
    if (s == NULL) {
        return 0;
    }
    memcpy(s->bytes, numeric_example, 18);

    u_int8_t* decoded;
    int n = decode_bitstream(s, 1, &decoded);
    free_bitstream(s);
    if (n < 0) {
        return 0;
    }

    int ok = 0 == memcmp(decoded, decoded_numeric_example, n);
    free(decoded);
    return ok;
}


int test_decode_bitstream_kanji() {
    struct bitstream* s = new_bitstream(55);
    if (s == NULL) {
        return 0;
    }
    memcpy(s->bytes, kanji_example, 55);

    u_int8_t* decoded;
    int n = decode_bitstream(s, 1, &decoded);
    free_bitstream(s);
    if (n < 0) {
        return 0;
    }

    int ok = 0 == memcmp(decoded, decoded_kanji_example, n);
    free(decoded);
    return ok;
}


int test_decode_bitstream_gb18030() {
    struct bitstream* s = new_bitstream(22);
    if (s == NULL) {
        return 0;
    }
    memcpy(s->bytes, chinese_gb18030, 22);

    struct bytebuffer* buffer = new_bytebuffer();
    int n = decode_gb18030_segment(s, 22, buffer);
    free_bitstream(s);
    if (n < 0) {
        return 0;
    }

    int ok = strlen((const char*)chinese_utf8) == buffer->n_bytes && 0 == memcmp(buffer->bytes, chinese_utf8, buffer->n_bytes);
    free_bytebuffer(buffer);
    return ok;
}


int test_decode_bitstream_big5() {
    struct bitstream* s = new_bitstream(22);
    if (s == NULL) {
        return 0;
    }
    memcpy(s->bytes, chinese_big5, 22);

    struct bytebuffer* buffer = new_bytebuffer();
    int n = decode_big5_segment(s, 22, buffer);
    free_bitstream(s);
    if (n < 0) {
        return 0;
    }

    int ok = strlen((const char*)chinese_utf8) == buffer->n_bytes && 0 == memcmp(buffer->bytes, chinese_utf8, buffer->n_bytes);
    free_bytebuffer(buffer);
    return ok;
}


int test_decode_bitstream_euc_kr() {
    struct bitstream* s = new_bitstream(22);
    if (s == NULL) {
        return 0;
    }
    memcpy(s->bytes, chinese_euc_kr, 22);

    struct bytebuffer* buffer = new_bytebuffer();
    int n = decode_euc_kr_segment(s, 22, buffer);
    free_bitstream(s);
    if (n < 0) {
        return 0;
    }

    int ok = strlen((const char*)chinese_utf8) == buffer->n_bytes && 0 == memcmp(buffer->bytes, chinese_utf8, buffer->n_bytes);
    free_bytebuffer(buffer);
    return ok;
}


int test_decode_bitstream_chinese_shift_jis() {
    struct bitstream* s = new_bitstream(22);
    if (s == NULL) {
        return 0;
    }
    memcpy(s->bytes, chinese_shift_jis, 22);

    struct bytebuffer* buffer = new_bytebuffer();
    int n = decode_byte_segment(s, 11, SJIS, buffer);
    free_bitstream(s);
    if (n < 0) {
        return 0;
    }

    int ok = strlen((const char*)chinese_utf8) == buffer->n_bytes && 0 == memcmp(buffer->bytes, chinese_utf8, buffer->n_bytes);
    free_bytebuffer(buffer);
    return ok;
}


int test_decode_eci_designator1() {
    struct bitstream* s = new_bitstream(1);
    if (s == NULL) {
        return 0;
    }
    s->bytes[0] = 33;

    // We should decode 1 byte
    int ok = 33 == read_eci_designator(s);
    // and fail to read again when the stream is empty
    ok = ok && DECODING_ERROR == read_eci_designator(s);
    free_bitstream(s);
    return ok;
}


int test_decode_eci_designator2() {
    struct bitstream* s = new_bitstream(3);
    if (s == NULL) {
        return 0;
    }
    // First value
    s->bytes[0] = 128 + 1;
    s->bytes[1] = 1;

    // Beginning of a second value
    s->bytes[2] = 128 + 1;

    // We should decode 2 bytes
    int ok = 257 == read_eci_designator(s);
    // and fail to read again when the stream does not have enough bits
    ok = ok && DECODING_ERROR == read_eci_designator(s);
    free_bitstream(s);
    return ok;
}


int test_decode_eci_designator3() {
    struct bitstream* s = new_bitstream(6);
    if (s == NULL) {
        return 0;
    }
    // First value
    s->bytes[0] = 128 + 64 + 1;
    s->bytes[1] = 0;
    s->bytes[2] = 1;

    // Invalid byte starting with 111
    s->bytes[3] = 128 + 64 + 32;

    // Beginning of a second value
    s->bytes[4] = 128 + 64 + 1;
    s->bytes[5] = 0;

    // We should decode 3 bytes
    int ok = 65537 == read_eci_designator(s);

    // and fail because of the 111 prefix (but the wrong byte
    // would still have been read from the stream)
    ok = ok && DECODING_ERROR == read_eci_designator(s);

    // We should fail to read again because the stream does not have enough bits
    int n;
    ok = ok && DECODING_ERROR == (n = read_eci_designator(s));
    free_bitstream(s);
    return ok;
}


int test_decode_percents_in_FNC1_mode() {
    struct bytebuffer* b = new_bytebuffer();
    if (b == NULL) {
        return 0;
    }
    write_byte(b, '%');
    write_byte(b, '%');
    write_byte(b, '%');
    write_byte(b, 'a');
    write_byte(b, '%');
    write_byte(b, 'b');
    write_byte(b, 'c');
    write_byte(b, '%');
    write_byte(b, '%');
    write_byte(b, 'd');
    write_byte(b, 'e');
    write_byte(b, '%');

    uint8_t expected[] = { '%', '%', '%', 'a', 0x1D, 'b', 'c', '%', 'd', 'e', 0x1D };
    decode_percents_in_FNC1_mode(b, 3);
    int ok = b->n_bytes == 11 && 0 == memcmp(expected, b->bytes, 11);

    free_bytebuffer(b);
    return ok;
}



typedef int (*test)();

int main() {
    printf("Running tests...\n");
    test tests[] = {
        test_reed_solomon1,
        test_reed_solomon2,
        test_get_degree,
        test_get_coefficient,
        test_set_coefficient,
        test_equal_polynomials,
        test_add_polynomials,
        test_add_polynomials2,
        test_multiply_polynomials,
        test_multiply_by_monomial,
        test_divide_polynomials,
        test_divide_polynomials2,
        test_calculate_sigma_omega,
        test_find_error_locations,
        test_find_error_magnitudes,
        test_error_correction,
        test_error_correction2,
        test_error_correction3,
        test_error_correction4,
        test_bitstream,
        test_decode_bitstream,
        test_decode_bitstream_numeric,
        test_decode_bitstream_kanji,
        test_decode_bitstream_gb18030,
        test_decode_bitstream_big5,
        test_decode_bitstream_euc_kr,
        test_decode_bitstream_chinese_shift_jis,
        test_decode_eci_designator1,
        test_decode_eci_designator2,
        test_decode_eci_designator3,
        test_decode_percents_in_FNC1_mode,
        NULL
    };
    int total = 0;
    int failed = 0;
    for (int i = 0 ; tests[i] != NULL ; i++) {
        total++;
        printf("---------------- test #%d ----------------\n", i);
        if (!(tests[i]())) {
            failed++;
            printf("============ test #%d failed! ============\n", i);
        }
    }

    if (failed) {
        printf("%d/%d test%s failed\n\n", failed, total, (failed > 1) ? "s" : "");
        exit(1);
    } else {
        printf("%d/%d test%s successful\n\n", total, total, (total > 1) ? "s" : "");
        exit(0);
    }
}
