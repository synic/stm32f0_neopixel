#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

#define DMA1_BASE			DMA_BASE

#include <libopencm3/stm32/dma.h>

typedef struct {
    int num_leds;
    uint8_t brightness;
    uint16_t dma_buffer[1000];
} ws2812;

ws2812 strip;

const uint8_t period = 29;
const uint8_t low = 9;
const uint8_t high = 17;

void setup_clock(void);
void setup_gpio(void);
void setup_timer(void);
void setup_dma(void);
void delay(volatile uint32_t loops);
void ws2812_send(uint8_t (*color)[3], uint16_t len);
void ws2812_set_color(uint8_t led, uint8_t r, uint8_t g, uint8_t b);
void ws2812_set_color_single(uint8_t led, uint32_t color);
uint32_t ws2812_color(uint8_t r, uint8_t b, uint8_t g);
void ws2812_clear(void);
void ws2812_show(void);
void rainbow(uint32_t wait);

uint32_t wheel(char pos);

void ws2812_set_color(uint8_t led, uint8_t r, uint8_t g, uint8_t b) {
    uint32_t n = (led * 24);
    uint8_t i;

    if(strip.brightness > 0) {
        r = (r * strip.brightness) >> 8;
        g = (g * strip.brightness) >> 8;
        b = (b * strip.brightness) >> 8;
    }

    for(i = 8; i-- > 0; n++) {
        strip.dma_buffer[n] = g & (1 << i) ? high : low;
    }

    for(i = 8; i-- > 0; n++) {
        strip.dma_buffer[n] = r & (1 << i) ? high : low;
    }

    for(i = 8; i-- > 0; n++) {
        strip.dma_buffer[n] = b & (1 << i) ? high : low;
    }
}

void ws2812_set_color_single(uint8_t led, uint32_t c) {
    uint8_t
        r = (uint8_t)(c >> 16),
        g = (uint8_t)(c >> 8),
        b = (uint8_t)(c);

    ws2812_set_color(led, r, g, b);
}

uint32_t ws2812_color(uint8_t r, uint8_t b, uint8_t g) {
    return ((uint32_t)r << 16) | ((uint32_t) g << 8) | b;
}

void ws2812_clear(void) {
    uint16_t i;

    // initialize all the colors as off
    for(i = 0; i < strip.num_leds; i++) {
        ws2812_set_color(i, 0, 0, 0);
    }
}

uint32_t wheel(char pos) {
    if(pos < 85) {
        return ws2812_color(pos * 3, 255 - pos * 3, 0);
    }
    else if(pos < 170) {
        pos -= 85;
        return ws2812_color(255 - pos * 3, 0, pos * 3);
    }
    else {
        pos -= 170;
        return ws2812_color(0, pos * 3, 255 - pos * 3);
    }
}

void rainbow(uint32_t wait) {
    uint16_t i, j;

    for(j = 0; j < 256 * 5; j++) {
        for(i = 0; i < strip.num_leds; i++) {
            ws2812_set_color_single(
                i, wheel(((i * 256 / strip.num_leds) + j) & 255));
        }
        ws2812_show();
        delay(wait);
    }
}

void setup_clock(void) {
    rcc_clock_setup_in_hse_8mhz_out_48mhz();
}

void setup_gpio(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    
    // set up the test LED
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        GPIO5);

    // set up the pin for timer output
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, 
        GPIO6);

    gpio_set_af(GPIOA, GPIO_AF1, GPIO6);
}

void setup_timer(void) {
    rcc_periph_clock_enable(RCC_TIM3);
    timer_reset(TIM3);
    timer_disable_oc_output(TIM3, TIM_OC1);
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_period(TIM3, period);
    timer_set_oc_polarity_high(TIM3, TIM_OC1);
    timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_PWM1);
    timer_set_oc_value(TIM3, TIM_OC1, 0);
    timer_enable_oc_output(TIM3, TIM_OC1);
}

void setup_dma(void) {
    rcc_periph_clock_enable(RCC_DMA);

    dma_channel_reset(DMA1, DMA_CHANNEL4);
    dma_set_priority(DMA1, DMA_CHANNEL4, DMA_CCR_PL_HIGH);
    dma_set_peripheral_address(DMA1, DMA_CHANNEL4, (uint32_t)&TIM3_CCR1);
    dma_set_memory_address(DMA1, DMA_CHANNEL4, (uint32_t)strip.dma_buffer);
    dma_set_memory_size(DMA1, DMA_CHANNEL4, DMA_CCR_MSIZE_16BIT);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL4, DMA_CCR_PSIZE_16BIT);
    dma_set_read_from_memory(DMA1, DMA_CHANNEL4);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL4);
    timer_enable_irq(TIM3, TIM_DIER_CC1DE);
}

void ws2812_show(void) {
    uint16_t memaddr;
    uint16_t buffersize;

    buffersize = (strip.num_leds * 24) + 42;
    memaddr = (strip.num_leds * 24);

    while(memaddr < buffersize) {
        strip.dma_buffer[memaddr] = 0;
        memaddr++;
    }

    timer_set_oc_value(TIM3, TIM_OC1, 0);
    dma_set_number_of_data(DMA1, DMA_CHANNEL4, buffersize);
    dma_enable_channel(DMA1, DMA_CHANNEL4);
    timer_enable_counter(TIM3);
    while(!dma_get_interrupt_flag(DMA1, DMA_CHANNEL4, DMA_TCIF));

    dma_disable_channel(DMA1, DMA_CHANNEL4);
    timer_disable_counter(TIM3);
    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL4, DMA_TCIF);
}

void delay(volatile uint32_t loops) {
    while(loops--) { }    
}

int main(void) {
    strip.num_leds = 60;
    strip.brightness = 40; 

    setup_clock();
    setup_gpio();
    setup_timer();
    setup_dma();

    ws2812_clear();

    while(1) {
        rainbow(13500);
        gpio_toggle(GPIOA, GPIO5);
    }

    return 0;
}
