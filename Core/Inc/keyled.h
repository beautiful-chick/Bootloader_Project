#ifndef KEYLED_H_
#define KEYLED_H_

#include <stdint.h>
#include "keyled.h"

#define delay_ms(ms)  R_BSP_SoftwareDelay(ms, BSP_DELAY_UNITS_MILLISECONDS)


/*+----------------+
 *|    Leds API    |
 *+----------------+*/

/** Information on how many LEDs and what pins they are on. */
typedef struct st_leds
{
    uint16_t         led_count;        ///< The number of LEDs on this board
    uint16_t const * p_leds;           ///< Pointer to an array of IOPORT pins for controlling LEDs
} leds_t;

/** Available user-controllable LEDs on this board. These enums can be can be used to index into the array of LED pins
 * found in the bsp_leds_t structure. */
typedef enum e_bsp_led
{
    LEDBLUE,                      ///< LED1
    LEDGREEN,                     ///< LED2
    LEDRED,                       ///< LED3
} lednum_t;

/** Available user-controllable LEDs on this board. These enums can be used to turn on/off LED. */
typedef enum e_led_status
{
    OFF,                      ///< Turn off LED
    ON,                       ///< Turn on  LED
} led_status_t;

extern const leds_t g_bsp_leds;

extern void turn_led(lednum_t which, led_status_t status);

extern void led_heartbeat(void);

/*+----------------+
 *|    Keys API    |
 *+----------------+*/

#define USER_KEY1_IRQ_NUMBER        10
#define USER_KEY2_IRQ_NUMBER        11

#define KEY1        (1<<0)
#define KEY2        (1<<1)

extern int  g_key_pressed;

extern int key_init(void);

extern void key_deinit(void);

#endif /* KEYLED_H_ */
