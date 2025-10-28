#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <math.h>
#include <avr/pgmspace.h>

#define LED_PIN PD6

// More aggressive gamma table (2.8 gamma) - spends less time at top brightness
const uint8_t gamma_table[64] PROGMEM = {
	0,   0,   0,   0,   1,   1,   2,   2,
	3,   4,   5,   6,   7,   8,   10,  11,
	13,  15,  17,  19,  21,  23,  26,  28,
	31,  34,  37,  40,  43,  47,  50,  54,
	58,  62,  66,  70,  75,  79,  84,  89,
	94,  99,  104, 110, 115, 121, 127, 133,
	139, 145, 152, 158, 165, 172, 179, 186,
	193, 201, 208, 216, 224, 232, 240, 248
};
void setup_soft_blink(void) {
	TCCR0A = (1 << WGM00) | (1 << COM0A1);
	TCCR0B = (1 << CS01) | (1 << CS00);
	OCR0A = 0;
	DDRD |= (1 << LED_PIN);
}

	/*
	// Prescaler = 1
	TCCR0B |= (1 << CS00);

	// Prescaler = 8
	TCCR0B |= (1 << CS01);
*/
	// Prescaler = 64
	//TCCR0B |= (1 << CS01) | (1 << CS00);

	// Prescaler = 256
//	TCCR0B |= (1 << CS02);

	// Prescaler = 1024
	//TCCR0B |= (1 << CS02) | (1 << CS00);
   


void set_gamma_corrected_brightness(uint8_t linear_level) {
	// Scale 0-255 to 0-63 and lookup
	uint8_t index = linear_level >> 2;  // Divide by 4
	OCR0A = pgm_read_byte(&gamma_table[index]);
}

void soft_blink_loop(void) {
	// Fade in
	for (uint16_t i = 0; i < 256; i++) {
		set_gamma_corrected_brightness(i);
		_delay_ms(56);
	}
	
	// Fade out
	for (uint16_t i = 255; i > 0; i--) {
		set_gamma_corrected_brightness(i);
		_delay_ms(56);
	}
}

int main(void) {
	setup_soft_blink();
	while(1) { soft_blink_loop(); }
}