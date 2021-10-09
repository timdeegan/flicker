#pragma once

/* AD5220 digital potentiometer. */
#define AD5220_DIR_PIN 15
bi_decl(bi_1pin_with_name(AD5220_DIR_PIN, "AD5220 up/down"));
#define AD5220_CLOCK_PIN 14
bi_decl(bi_1pin_with_name(AD5220_CLOCK_PIN, "AD5220 clock"));

/* Osram SFH300 phototransistor. */
#define PT_PIN 26
bi_decl(bi_1pin_with_name(PT_PIN, "SFH300 phototransistor"));
