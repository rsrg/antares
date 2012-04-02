//#define STM32F10X_LD
#include <arch/antares.h>
#include "stm32f10x.h"
#include "stm32f10x_tim.h"
#include "stm32f10x_adc.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"

void mdelay(int n)
{
        int i = n*100;													/* About 1/4 second delay */
        while (i-- > 0) {
                asm("nop");													/* This stops it optimising code out */
        }
}


static  GPIO_InitTypeDef gpio_leds = {
	.GPIO_Speed = GPIO_Speed_50MHz,
	.GPIO_Pin = GPIO_Pin_8,
	.GPIO_Mode = GPIO_Mode_Out_PP,
};


inline void led_off()
{
	GPIO_SetBits(GPIOG,GPIO_Pin_8);
}

inline void led_on()
{
	GPIO_ResetBits(GPIOG,GPIO_Pin_8);
}


//TODO: Tuning to custom freqs
void setup_freq()
{
  //Tune up to 76Mhz
  /* enable HSE */
  RCC->CR = RCC->CR | 0x00010001;
  while ((RCC->CR & 0x00020000) == 0); /* for it to come on */
  
  /* enable flash prefetch buffer */
  FLASH->ACR = 0x00000012;

  /* Configure PLL */
  RCC->CFGR = RCC->CFGR | 0x00110400;  /* pll=72Mhz,APB1=36Mhz,AHB=72Mhz */
  RCC->CR = RCC->CR     | 0x01000000;  /* enable the pll */
  while ((RCC->CR & 0x03000000) == 0);         /* wait for it to come on */
  
  /* Set SYSCLK as PLL */
  RCC->CFGR = RCC->CFGR | 0x00000002;
  while ((RCC->CFGR & 0x00000008) == 0); /* wait for it to come on */
}


void test_led(int c)
{
	while(c--)
		{
			led_off();
			mdelay(12000);
			led_on();
			mdelay(12000);
		}
}


#include "fw.h"
// int fw_size = 5;
// const char fw[] = {1,2,3,4,5,6};

/*
JRST CCLK PB4
JTDO DIN PB3
JTDI INITB PA15
JTCK PROGB PA14
JTMS DONE PA13
*/

static  GPIO_InitTypeDef xsscu_pai = {
.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_15,
.GPIO_Mode = GPIO_Mode_IPU,
};

static  GPIO_InitTypeDef xsscu_pao = {
.GPIO_Speed = GPIO_Speed_50MHz,
.GPIO_Pin = GPIO_Pin_14,
.GPIO_Mode = GPIO_Mode_Out_PP,
};

static  GPIO_InitTypeDef xsscu_pbo = {
.GPIO_Speed = GPIO_Speed_50MHz,
.GPIO_Pin = GPIO_Pin_3|GPIO_Pin_4,
.GPIO_Mode = GPIO_Mode_Out_PP,
};

// 

#define DELAY	;;


void test_pin()
{
		while(1)
		{
		led_on();
		GPIO_Write(GPIOA,0x0000);
		mdelay(120);
		GPIO_Write(GPIOA,0xffff);
		led_off();
		mdelay(120);
		}
}

static int xsscu_reset_fpga()
{
	int i = 500;
	GPIO_ResetBits(GPIOA,GPIO_Pin_14); //progb
	mdelay(1200);
	GPIO_SetBits(GPIOA,GPIO_Pin_14); //progb
	mdelay(1000);
	while (i--) {
		if (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15)) //initb
			return 0;
 		mdelay(1);
	}
	return 1;
}

static int xsscu_send_clocks(int c)
{
	while (c--) {
		GPIO_ResetBits(GPIOB,GPIO_Pin_4);
		DELAY;
		GPIO_SetBits(GPIOB,GPIO_Pin_4);
		DELAY;
		if (GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_13))
			return 0;
	}
	return 1;
}

void xsscu_init_pins()
{
  	
	GPIO_Init(GPIOA, &xsscu_pao);
	GPIO_Init(GPIOA, &xsscu_pai);
 	GPIO_Init(GPIOB, &xsscu_pbo);
};

void xsscu_write()
{
	int i=0;
	int k;
	GPIO_WriteBit(GPIOB,GPIO_Pin_4,0);
	GPIO_WriteBit(GPIOB,GPIO_Pin_3,0);
	
	while (i < fw_size) {
		led_off();
		for (k = 7; k >= 0; k--) {
			GPIO_WriteBit(GPIOB,GPIO_Pin_3,
					      ((fw[i] & (1 << k))));
			GPIO_WriteBit(GPIOB,GPIO_Pin_4,1);
			DELAY;
			GPIO_WriteBit(GPIOB,GPIO_Pin_4,0);
			DELAY;
		}
		led_on();
		i++;
	}
}


void hang(int code)
{
	test_led(code);
	if (0==code) led_off();
	while(1);;;
}

int main()
{
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOG, ENABLE);
		GPIO_Init(GPIOG, &gpio_leds);
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);
	    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
		GPIO_PinRemapConfig(GPIO_Remap_SWJ_Disable, ENABLE);
		xsscu_init_pins();
		 //Disable JTAG
// 		setup_freq();
		led_off();
		mdelay(1200);
 		if (xsscu_reset_fpga()) hang(7);
		xsscu_write();
		if ((xsscu_send_clocks(1000))) hang(8);;
		test_led(0);
		led_off();
		while(1);;
        return 0;
}

