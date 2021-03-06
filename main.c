#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include "logs.h"
#include "qrcode.h"


int main(int argc, char* argv[]) {

    const char* optstring = "v";
    const struct option lopts[] = {
        { "verbose", no_argument, NULL, 'v' },
        { NULL, no_argument, NULL, 0 }
    };

    char verbose = 0;

    int val, index = -1;
    while (EOF != (val = getopt_long(argc, argv, optstring, lopts, &index))) {
        switch (val) {
            case 'v': verbose = 1; break;
        }
        index = -1;
    }

    if (argc == optind) {
        printf("Usage: qrcode [-v|--verbose] PNG\n");
        printf("\n");
        printf(" -v|--verbose  turns on maximum logging\n");
        printf("\n");
        printf("Given a png image, tries to locate QR codes in it. On success,\n");
        printf("prints on the standard output an html page that shows the matches\n");
        printf("in the image as red rectangles. Hovering a rectangle with the mouse\n");
        printf("will show the decoded message associated with the QR code. Also, the\n");
        printf("finder patterns that were recognized are shown in blue circles.\n");
        printf("\n");
        return 1;
    }

    set_log_level(verbose ? GORY : INFO);

    struct qr_code_match_list* matches;
    struct finder_pattern_list* finder_patterns;
    int res = find_qr_codes(argv[optind], &matches, &finder_patterns);
    if (res == MEMORY_ERROR) {
        error("Memory allocation error\n");
        return 1;
    }
    if (res == CANNOT_LOAD_IMAGE) {
        error("Cannot load image '%s'\n", argv[optind]);
        return 1;
    }

    printf("<html>\n");
    printf("<head></head>\n");
    printf("<body>\n");
    printf("<div style='position:absolute; top:0px; left:0px'>\n");
    printf("<img src='%s'>\n", argv[optind]);
    printf("<div>\n");

    // Let's tag the finder patterns
    struct finder_pattern_list* tmp_finder_patterns = finder_patterns;
    while (tmp_finder_patterns != NULL) {
        int center_x = (int)tmp_finder_patterns->pattern.x;
        int center_y = (int)tmp_finder_patterns->pattern.y;
        int radius = (int)(5 * tmp_finder_patterns->pattern.module_size);
        printf("<div style='position:absolute; top:%dpx; left:%dpx'>\n", center_y - radius, center_x - radius);
        printf("  <svg xmlns='http://www.w3.org/2000/svg' version='1.1' width='%dpx' height='%dpx'>\n", radius * 2, radius * 2);
        printf("    <circle cx='%d' cy='%d' r='%d' style='stroke: blue; stroke-width: 3; fill: none'/>\n", radius, radius, radius - 2);
        printf("  </svg>\n");
        printf("</div>\n");

        tmp_finder_patterns = tmp_finder_patterns->next;
    }

    free_finder_pattern_list(finder_patterns);


    struct qr_code_match_list* tmp = matches;
    while (tmp != NULL) {
        // Let's find the rectangle that is the bounding box
        int min_x = tmp->bottom_left_x;
        if (min_x > tmp->top_left_x) min_x = tmp->top_left_x;
        if (min_x > tmp->top_right_x) min_x = tmp->top_right_x;
        if (min_x > tmp->bottom_right_x) min_x = tmp->bottom_right_x;

        int max_x = tmp->bottom_left_x;
        if (max_x < tmp->top_left_x) max_x = tmp->top_left_x;
        if (max_x < tmp->top_right_x) max_x = tmp->top_right_x;
        if (max_x < tmp->bottom_right_x) max_x = tmp->bottom_right_x;

        int min_y = tmp->bottom_left_y;
        if (min_y > tmp->top_left_y) min_y = tmp->top_left_y;
        if (min_y > tmp->top_right_y) min_y = tmp->top_right_y;
        if (min_y > tmp->bottom_right_y) min_y = tmp->bottom_right_y;

        int max_y = tmp->bottom_left_y;
        if (max_y < tmp->top_left_y) max_y = tmp->top_left_y;
        if (max_y < tmp->top_right_y) max_y = tmp->top_right_y;
        if (max_y < tmp->bottom_right_y) max_y = tmp->bottom_right_y;

        int width = (max_x - min_x);
        int height = (max_y - min_y);
        printf("<div style='position:absolute; top:%dpx; left:%dpx'>\n", min_y, min_x);
        printf("  <svg xmlns='http://www.w3.org/2000/svg' version='1.1' width='%dpx' height='%dpx'>\n", width, height);
        printf("    <rect width='%d' height='%d' style='stroke: red; stroke-width: 4; fill: none'/>\n", width, height);
        printf("    <title>");
        if (contains_text_data(tmp->message)) {
            printf("Text message: ");
            for (unsigned int j = 0 ; j < tmp->message->n_bytes ; j++) {
                u_int8_t c = tmp->message->bytes[j];
                if (c == '<') {
                    printf("&lt;");
                } else if (c == '>') {
                    printf("&gt;");
                } else {
                    printf("%c", c);
                }
            }
        } else {
            printf("Binary message:");
            for (unsigned int j = 0 ; j < tmp->message->n_bytes ; j++) {
                printf(" %02x", tmp->message->bytes[j]);
            }
        }
        printf("</title>\n");
        printf("  </svg>\n");
        printf("</div>\n");

        tmp = tmp->next;
    }

    free_qr_code_match_list(matches);
    printf("</body>\n");
    printf("<html>\n");

    return 0;
}
