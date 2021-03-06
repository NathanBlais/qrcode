#ifndef _QRCODEFINDER_H
#define _QRCODEFINDER_H

#include <stdlib.h>
#include "bitmatrix.h"
#include "errors.h"
#include "finderpattern.h"


/**
 * This structure represents a QR code identified in an image.
 */
struct qr_code {
    // A binary matrix where each cell represents one QR code module
    struct bit_matrix* modules;

    // The coordinates of the QR code corners in the original image
    int bottom_left_x, bottom_left_y;
    int top_left_x, top_left_y;
    int top_right_x, top_right_y;
    int bottom_right_x, bottom_right_y;
};


/**
 * Given a binary image and the coordinates of 3 finder patterns,
 * this function looks if it can find a QR code.
 * Returns SUCCESS on success and place the QR code in *qr_code
 *         DECODING_ERROR if no QR code can be found
 *         MEMORY_ERROR in case of memoey allocation error
 */
int get_qr_code(struct finder_pattern bottom_left,
                            struct finder_pattern top_left,
                            struct finder_pattern top_right,
                            struct bit_matrix* image,
                            struct qr_code* *qr_code);


/**
 * Frees all the memory associated to the given code.
 */
void free_qr_code(struct qr_code* code);

#endif
