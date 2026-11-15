#ifndef STATUS_LED_H
#define STATUS_LED_H

/**
 * @file status_led.h
 * @brief Status LED: fast blink while WiFi is down, short pulse when meter data arrives.
 */

/**
 * @brief Starts the LED task.
 *
 * The LED blinks fast while WiFi is down and is otherwise off until status_led_pulse() is called.
 */
void status_led_start(void);

/**
 * @brief Requests a short blink; call when data has been received from the meter.
 *
 * Has no visible effect while WiFi is down. Safe to call from any task.
 */
void status_led_pulse(void);

#endif
