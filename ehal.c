#include <stdint.h>
#include <stdlib.h>
#include <chopstx.h>

#include "board.h"

static uint8_t main_finished;

#define PERIPH_BASE	0x40000000
#define APBPERIPH_BASE   PERIPH_BASE
#define APB2PERIPH_BASE	(PERIPH_BASE + 0x10000)
#define AHBPERIPH_BASE	(PERIPH_BASE + 0x20000)
#define AHB2PERIPH_BASE	(PERIPH_BASE + 0x08000000)

struct GPIO {
  volatile uint32_t MODER;
  volatile uint16_t OTYPER;
  uint16_t dummy0;
  volatile uint32_t OSPEEDR;
  volatile uint32_t PUPDR;
  volatile uint16_t IDR;
  uint16_t dummy1;
  volatile uint16_t ODR;
  uint16_t dummy2;
  volatile uint16_t BSRR;
  uint16_t dummy3;
  volatile uint32_t LCKR;
  volatile uint32_t AFR[2];
  volatile uint16_t BRR;
  uint16_t dummy4;
};
#define GPIOA_BASE	(AHB2PERIPH_BASE + 0x0000)
#define GPIOA		((struct GPIO *) GPIOA_BASE)
#define GPIOF_BASE	(AHB2PERIPH_BASE + 0x1400)
#define GPIOF		((struct GPIO *) GPIOF_BASE)

static struct GPIO *const GPIO_LED = ((struct GPIO *)GPIO_LED_BASE);
static struct GPIO *const GPIO_OTHER = ((struct GPIO *)GPIO_OTHER_BASE);

static chopstx_mutex_t mtx;
static chopstx_cond_t cnd0, cnd1;

#define BUTTON_PUSHED 1
static uint8_t button_state;

static uint8_t
user_button (void)
{
  return button_state;
}


static uint8_t l_data[5];
#define LED_FULL ((0x1f << 20)|(0x1f << 15)|(0x1f << 10)|(0x1f << 5)|0x1f)

static void
set_led_display (uint32_t data)
{
  l_data[0] = (data >>  0) & 0x1f;
  l_data[1] = (data >>  5) & 0x1f;
  l_data[2] = (data >> 10) & 0x1f;
  l_data[3] = (data >> 15) & 0x1f;
  l_data[4] = (data >> 20) & 0x1f;
}

static void
scroll_led_display (uint8_t row)
{
  l_data[0] = (l_data[0] << 1) | ((row >> 0) & 1);
  l_data[1] = (l_data[1] << 1) | ((row >> 1) & 1);
  l_data[2] = (l_data[2] << 1) | ((row >> 2) & 1);
  l_data[3] = (l_data[3] << 1) | ((row >> 3) & 1);
  l_data[4] = (l_data[4] << 1) | ((row >> 4) & 1);
}


static void
wait_for (uint32_t usec)
{
  chopstx_usec_wait (usec);
}

static void
led_prepare_row (uint8_t col)
{
  uint16_t data = 0x1f;

  data |= ((l_data[0] & (1 << col)) ? 1 : 0) << 5;
  data |= ((l_data[1] & (1 << col)) ? 1 : 0) << 6;
  data |= ((l_data[2] & (1 << col)) ? 1 : 0) << 7;
  data |= ((l_data[3] & (1 << col)) ? 1 : 0) << 9;
  data |= ((l_data[4] & (1 << col)) ? 1 : 0) << 10;
  GPIO_LED->ODR = data;
}


static void
led_enable_column (uint8_t col)
{
  GPIO_LED->BRR = (1 << col);
}

static void *
led (void *arg)
{
  (void)arg;

  chopstx_mutex_lock (&mtx);
  chopstx_cond_wait (&cnd0, &mtx);
  chopstx_mutex_unlock (&mtx);

  while (!main_finished)
    {
      int i;

      for (i = 0; i < 5; i++)
	{
	  led_prepare_row (i);
	  led_enable_column (i);
	  wait_for (1000);
	}
    }

  GPIO_LED->ODR = 0x0000;	/* Off all LEDs.  */
  GPIO_LED->OSPEEDR = 0;
  GPIO_LED->OTYPER = 0;
  GPIO_LED->MODER = 0;		/* Input mode.  */
  GPIO_OTHER->PUPDR = 0x0000;	/* No pull-up.  */

  return NULL;
}


static uint8_t get_button_sw (void) { return (GPIO_OTHER->IDR & 1) == 0; }

static void *
button (void *arg)
{
  uint8_t last_button = 0;

  (void)arg;

  chopstx_mutex_lock (&mtx);
  chopstx_cond_wait (&cnd1, &mtx);
  chopstx_mutex_unlock (&mtx);

  while (!main_finished)
    {
      uint8_t button = get_button_sw ();

      if (last_button == button && button != button_state)
	{
	  wait_for (1000);
	  button = get_button_sw ();
	  if (last_button == button)
	    button_state = button;
	}

      wait_for (2000);
      last_button = button;
    }

  return NULL;
}

#define PRIO_LED 3
#define PRIO_BUTTON 2

extern uint8_t __process1_stack_base__[], __process1_stack_size__[];
extern uint8_t __process2_stack_base__[], __process2_stack_size__[];

#define STACK_ADDR_LED ((uint32_t)__process1_stack_base__)
#define STACK_SIZE_LED ((uint32_t)__process1_stack_size__)

#define STACK_ADDR_BUTTON ((uint32_t)__process2_stack_base__)
#define STACK_SIZE_BUTTON ((uint32_t)__process2_stack_size__)

#define DATA55V(x0,x1,x2,x3,x4) (x0<<0)|(x1<<5)|(x2<<10)|(x3<< 15)|(x4<< 20)

#define CHAR_SPC 0
#define CHAR_e   1
#define CHAR_TR  2
#define CHAR_H   3 
#define CHAR_A   4 
#define CHAR_L   5 

static uint8_t ehal[] = {
  CHAR_e, CHAR_TR, CHAR_H, CHAR_A, CHAR_L,
  CHAR_SPC, 
};

struct { uint8_t width; uint32_t data; } chargen[] = {
  { 3, 0 },						/* SPACE */
  { 4, DATA55V (0x0e, 0x15, 0x15, 0x0d, 0x00) },	/* e */
  { 3, DATA55V (0x04, 0x04, 0x04, 0x00, 0x00) },        /* - */
  { 4, DATA55V (0x1f, 0x04, 0x04, 0x1f, 0x00) },	/* H */
  { 4, DATA55V (0x1f, 0x14, 0x14, 0x1f, 0x00) },	/* A */
  { 4, DATA55V (0x1f, 0x01, 0x01, 0x01, 0x00) },	/* L */
};


static int
text_display (uint8_t kind)
{
  unsigned int i, j;
  uint8_t *text;
  uint8_t len;
  uint8_t state = 0;

  if (kind)
    {  
      text = ehal;
      len = sizeof (ehal);
    }
  set_led_display (0);
  while (1)
    for (i = 0; i < len; i++)
      {
	for (j = 0; j < chargen[text[i]].width; j++)
	  {
	    if (user_button ())
	      {
		set_led_display (LED_FULL);
		state = 1;
	      }
	    else if (state == 1)
	      return 0;
	    else
	      scroll_led_display ((chargen[text[i]].data >> j * 5) & 0x1f);
	    wait_for (120*2000);
	  }

	if (user_button ())
	  {
	    set_led_display (LED_FULL);
	    state = 1;
	  }
	else if (state == 1)
	  return 0;
	else
	  scroll_led_display (0);
	wait_for (120*2000);
      }

  return 1;
}


static void setup_scr_sleepdeep (void);

int
main (int argc, const char *argv[])
{
  chopstx_t led_thd;
  chopstx_t button_thd;
  uint8_t happy = 1;
  (void)argc;
  (void)argv;

  chopstx_mutex_init (&mtx);
  chopstx_cond_init (&cnd0);
  chopstx_cond_init (&cnd1);

  led_thd = chopstx_create (PRIO_LED, STACK_ADDR_LED,
			    STACK_SIZE_LED, led, NULL);
  button_thd = chopstx_create (PRIO_BUTTON, STACK_ADDR_BUTTON,
			       STACK_SIZE_BUTTON, button, NULL);

  chopstx_usec_wait (200*1000);

  chopstx_mutex_lock (&mtx);
  chopstx_cond_signal (&cnd0);
  chopstx_cond_signal (&cnd1);
  chopstx_mutex_unlock (&mtx);

  wait_for (100*1000);
  if (user_button ())
    {
      /* Wait button release.  */
      while (user_button ())
	wait_for (100*1000);

      happy = 0;
      goto do_text;
    }

  while (1)
    {
    do_text:
      if (text_display (happy))
	break;
    }

  main_finished = 1;
  chopstx_join (button_thd, NULL);
  chopstx_join (led_thd, NULL);

  setup_scr_sleepdeep ();
  for (;;)
    asm volatile ("wfi" : : : "memory");

  return 0;
}

struct SCB
{
  volatile uint32_t CPUID;
  volatile uint32_t ICSR;
  volatile uint32_t VTOR;
  volatile uint32_t AIRCR;
  volatile uint32_t SCR;
  volatile uint32_t CCR;
  volatile uint8_t  SHP[12];
  volatile uint32_t SHCSR;
  volatile uint32_t CFSR;
  volatile uint32_t HFSR;
  volatile uint32_t DFSR;
  volatile uint32_t MMFAR;
  volatile uint32_t BFAR;
  volatile uint32_t AFSR;
  volatile uint32_t PFR[2];
  volatile uint32_t DFR;
  volatile uint32_t ADR;
  volatile uint32_t MMFR[4];
  volatile uint32_t ISAR[5];
};

#define SCS_BASE	(0xE000E000)
#define SCB_BASE	(SCS_BASE +  0x0D00)
static struct SCB *const SCB = ((struct SCB *)SCB_BASE);

#define SCB_SCR_SLEEPDEEP (1 << 2)

struct PWR
{
  volatile uint32_t CR;
  volatile uint32_t CSR;
};
#define PWR_CR_PDDS 0x0002
#define PWR_CR_CWUF 0x0004

#define PWR_BASE	(APBPERIPH_BASE + 0x00007000)
#define PWR		((struct PWR *) PWR_BASE)

static void setup_scr_sleepdeep (void)
{
  PWR->CR |= PWR_CR_CWUF;
  PWR->CR |= PWR_CR_PDDS;
  SCB->SCR |= SCB_SCR_SLEEPDEEP;
}
