#pragma once

/* These are GPIO pin numbers on the RP2040 itself.
 * The pin numbers on the Pico board are given in the comments. */

/* AD5220 digital potentiometer. */
#define AD5220_DIR_PIN 14 /* Pico 19 */
bi_decl(bi_1pin_with_name(AD5220_DIR_PIN, "AD5220 up/down"));
#define AD5220_CLOCK_PIN 15 /* Pico 20 */
bi_decl(bi_1pin_with_name(AD5220_CLOCK_PIN, "AD5220 clock"));

/* Osram SFH300 phototransistor. */
#define PT_PIN 26 /* Pico 31 */
bi_decl(bi_1pin_with_name(PT_PIN, "SFH300 phototransistor"));

/* Pico's on-board switched mode power supply. */
#define SMPS_PIN 23
bi_decl(bi_1pin_with_name(SMPS_PIN, "Pico SMPS control"));

/* Pico's on-board LED. */
#define LED_PIN 25
bi_decl(bi_1pin_with_name(LED_PIN, "Pico LED"));
