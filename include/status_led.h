#ifndef STATUS_LED_H
#define STATUS_LED_H

// Start the LED task: fast blink while WiFi is down, otherwise idle until status_led_pulse()
void status_led_start(void);

// Request a short blink (call when data has been received from the meter)
void status_led_pulse(void);

#endif
