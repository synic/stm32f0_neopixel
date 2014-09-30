#include "stm32f0xx.h"

typedef struct {
    int num_leds;
    uint8_t brightness;
    volatile uint8_t dma_buffer[2000];
} ws2812;

ws2812 strip;

const uint8_t period = 59;
const uint8_t low = 17;
const uint8_t high = 25;
const uint8_t reset_len = 25;
uint8_t set = 0;


/*#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t* file , uint32_t line)
{
  while (1);
}
#endif*/

void delay(volatile long delay) {
    delay *= 1000;
    while(delay--) {}
}

void ws2812_set_color(uint8_t led, uint8_t r, uint8_t g, uint8_t b) {
    volatile uint32_t n = (led * 24);
    volatile uint8_t i;

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
    volatile uint16_t i;

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

void ws2812_show(void) {
    volatile uint16_t memaddr;
    volatile uint16_t buffersize;

    buffersize = (strip.num_leds * 24) + reset_len;
    memaddr = (strip.num_leds * 24);

    while(memaddr < buffersize) {
        strip.dma_buffer[memaddr] = 0;
        memaddr++;
    }

    DMA_SetCurrDataCounter(DMA1_Channel4, buffersize);
    DMA_Cmd(DMA1_Channel4, ENABLE);
    TIM_Cmd(TIM3, ENABLE);

    while(!DMA_GetFlagStatus(DMA1_FLAG_TC4));

    DMA_Cmd(DMA1_Channel4, DISABLE);
    TIM_Cmd(TIM3, DISABLE);
    DMA_ClearFlag(DMA1_FLAG_TC4);
}

void rainbow(uint32_t wait) {
    volatile uint16_t i, j;

    for(j = 0; j < 256 * 5; j++) {
        for(i = 0; i < strip.num_leds; i++) {
            ws2812_set_color_single(
                i, wheel(((i * 256 / strip.num_leds) + j) & 255));
        }
        ws2812_show();
        delay(wait);
    }
}

void setup_gpio(void)
{
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    // set up pin 5 for the status LED
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // set up pin 6 for the timer
    GPIO_InitTypeDef GPIO_InitStructure2;
    GPIO_InitStructure2.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure2.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure2.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure2.GPIO_Speed = GPIO_Speed_2MHz;
    GPIO_InitStructure2.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOA, &GPIO_InitStructure2);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource6, GPIO_AF_1);

}

void setup_timer(void) {
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    TIM_TimeBaseInitTypeDef TIM_InitStructure;
    TIM_InitStructure.TIM_Period = period;
    TIM_InitStructure.TIM_Prescaler = 0;
    TIM_InitStructure.TIM_ClockDivision = 0;
    TIM_InitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM3, &TIM_InitStructure);

    // configure the output channel
    TIM_OCInitTypeDef TIM_OCInitStructure;
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 0;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM3, &TIM_OCInitStructure);
}

void setup_dma(void) {
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_InitTypeDef DMA_InitStructure;

    DMA_DeInit(DMA1_Channel4);
    
    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&TIM3->CCR1;
/*    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)TIM3_CCR1_Address;*/
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)strip.dma_buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel4, &DMA_InitStructure);

    TIM_DMACmd(TIM3, TIM_DMA_CC1, ENABLE);
}

int main(void)
{
  SystemInit();
  strip.num_leds = 144;
  strip.brightness = 30;

  setup_gpio();
  setup_timer();
  setup_dma();
  
  ws2812_set_color(89, 0, 0, 0);

  ws2812_clear();

  while (1)
  {
    rainbow(0);
    if(set) {
        GPIO_ResetBits(GPIOA, GPIO_Pin_5);
        set = 0;
    }
    else {
        GPIO_SetBits(GPIOA, GPIO_Pin_5);
        set = 1;
    }
  }

  return 0;
}

