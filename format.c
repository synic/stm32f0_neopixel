#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

uint16_t buffer[1000];

void set_color(uint8_t led, uint8_t brightness, uint8_t r, uint8_t g,
        uint8_t b) {

    uint32_t n = (led * 24);
    uint8_t i;

    if(brightness > 0) {
        r = (r * brightness) >> 8;
        g = (g * brightness) >> 8;
        b = (b * brightness) >> 8;
    }

    for(i = 8; i-- > 0; n++) {
        buffer[n] = g & (1 << i) ? 1 : 0;
    }

    for(i = 8; i-- > 0; n++) {
        buffer[n] = r & (1 << i) ? 1 : 0;
    }

    for(i = 8; i-- > 0; n++) {
        buffer[n] = b & (1 << i) ? 1 : 0;
    }
}

void print_index(uint8_t index) {
    uint8_t i;
    uint32_t n = (index * 24);
    for(i = n; i < 24; i++) {
        if(i % 8 == 0 && i) printf(" ");
        printf("%d", buffer[i]);
    }

    printf("\n");
}

int main(int argc, char *argv[]) {
    uint8_t brightness, r, g, b;

    if(argc != 5) {
        printf("Invalid number of command line arguments specified: %d.\n",
            argc);
        return 1;
    }

    brightness = atoi(argv[1]);
    r = atoi(argv[2]);
    g = atoi(argv[3]);
    b = atoi(argv[4]);

    set_color(0, brightness, r, g, b);
    print_index(0);

    return 0;
}
