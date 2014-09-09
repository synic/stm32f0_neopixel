#include <string.h>
#include <stdint.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/timer.h>

#define DMA1_BASE			DMA_BASE

#include <libopencm3/stm32/dma.h>
#include <libopencm3/cm3/nvic.h>

#include "colors.h"

#define PERIOD 14

uint16_t led_buffer[100];
const uint8_t low = PERIOD * .32;
const uint8_t high = PERIOD * .64;

void setup_clock(void);
void setup_gpio(void);
void setup_timer(void);
void setup_dma(void);
void delay(volatile uint32_t loops);
void ws2812_send(uint8_t (*color)[3], uint16_t len);

void setup_clock(void) {
    rcc_clock_setup_in_hse_8mhz_out_48mhz();
}

void setup_gpio(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    
    // set up the test LED
    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE,
        GPIO5);

    // set up the pin for timer output
    gpio_mode_setup(GPIOA, GPIO_MODE_AF, GPIO_OTYPE_PP, GPIO6);
    gpio_set_output_options(GPIOA, GPIO_OTYPE_PP, GPIO_OSPEED_HIGH, 
        GPIO6);

    gpio_set_af(GPIOA, GPIO_AF1, GPIO6);
}

void setup_timer(void) {
    rcc_periph_clock_enable(RCC_TIM3);
    timer_reset(TIM3);
    timer_set_mode(TIM3, TIM_CR1_CKD_CK_INT, TIM_CR1_CMS_EDGE, TIM_CR1_DIR_UP);
    timer_set_prescaler(TIM3, 0);
    timer_set_clock_division(TIM3, 0);
    timer_set_period(TIM3, PERIOD);
    timer_disable_oc_output(TIM3, TIM_OC1);
    timer_set_oc_mode(TIM3, TIM_OC1, TIM_OCM_TOGGLE);
    timer_set_master_mode(TIM3, TIM_CCMR1_OC1M_PWM1);
}

void setup_dma(void) {
    rcc_periph_clock_enable(RCC_DMA);

    //nvic_enable_irq(NVIC_DMA1_CHANNEL4_5_IRQ);
    dma_channel_reset(DMA1, DMA_CHANNEL4);
    dma_set_priority(DMA1, DMA_CHANNEL4, DMA_CCR_PL_LOW);
    dma_set_memory_size(DMA1, DMA_CHANNEL4, DMA_CCR_MSIZE_16BIT);
    dma_set_read_from_memory(DMA1, DMA_CHANNEL4);
    dma_set_peripheral_size(DMA1, DMA_CHANNEL4, DMA_CCR_PSIZE_16BIT);
    dma_enable_memory_increment_mode(DMA1, DMA_CHANNEL4);
    dma_set_peripheral_address(DMA1, DMA_CHANNEL4, (uint32_t)&TIM3_CCR1);
    dma_set_memory_address(DMA1, DMA_CHANNEL4, (uint32_t)&led_buffer);
    dma_enable_transfer_complete_interrupt(DMA1, DMA_CHANNEL4);
/*    dma_enable_channel(DMA1, DMA_CHANNEL4);*/
    timer_enable_irq(TIM3, TIM_DIER_CC1DE);
}

void ws2812_send(uint8_t (*color)[3], uint16_t len) {
    volatile uint8_t i, j, c;
    uint8_t led;
    uint16_t memaddr;
    uint16_t buffersize;

    buffersize = (len * 24) + 42;
    memaddr = 0;
    led = 0;
    c = 0;
    uint8_t which[] = {1, 0, 2};

    while(len) {
        for(i = 0; i < 3; i++) {
            c = which[i];   
            for(j = 0; j < 8; j++) {
                if((color[led][c] << j) & 0x80) {
                    led_buffer[memaddr] = high;
                }
                else {
                    led_buffer[memaddr] = low;
                }

                memaddr++;
            }
        }

        led++;
        len--;
    }

    while(memaddr < buffersize) {
        led_buffer[memaddr] = 0;
        memaddr++;
    }

    dma_set_number_of_data(DMA1, DMA_CHANNEL4, buffersize);
    dma_enable_channel(DMA1, DMA_CHANNEL4);
    timer_enable_oc_output(TIM3, TIM_OC1);
    timer_enable_counter(TIM3);
    while(!dma_get_interrupt_flag(DMA1, DMA_CHANNEL4, DMA_TCIF));

    timer_disable_oc_output(TIM3, TIM_OC1);
    timer_disable_counter(TIM3);
    dma_disable_channel(DMA1, DMA_CHANNEL4);
    dma_clear_interrupt_flags(DMA1, DMA_CHANNEL4, DMA_TCIF);
}

void delay(volatile uint32_t loops) {
    while(loops--) { }    
}

int main(void) {
    setup_clock();
    setup_gpio();
    setup_timer();
    setup_dma();
    
    gpio_set(GPIOA, GPIO5);

	int16_t i;
	while(1) {
		for(i = 0; i < 766; i += 2)
		{
			ws2812_send(&eightbit[i], 2);
			delay(10000L);
		}

        gpio_clear(GPIOA, GPIO5);

		/* cycle through the colors on only one LED
		 * this time only the first LED that data is
		 * fed into will update
		 */
		for (i = 0; i < 766; i += 1)
		{
			ws2812_send(&eightbit[i], 1);
			delay(10000L);
		}

        gpio_set(GPIOA, GPIO5);
	}

    return 0;
}
