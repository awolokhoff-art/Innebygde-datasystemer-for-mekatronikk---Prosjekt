//visst denne ikke funker like bra må man kanskje endre på delay

#define F_CPU 1000000UL

#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h> // Denne header-filen inneholder PROGMEM og pgm_read_byte.... Vil ha det i PROGMEM=Flashminne for det er mye større lagringsplass en SRAM'en

// More aggressive gamma table (2.8 gamma) - spends less time at top brightness
const uint8_t gamma_table[256] PROGMEM = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3,
	3, 4, 4, 4, 4, 5, 5, 5, 5, 6, 6, 6, 7, 7, 7, 8,
	8, 9, 9, 10, 10, 11, 12, 12, 13, 14, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 26, 27, 28, 30, 31, 33, 34, 36, 38, 40, 41,
	43, 45, 47, 49, 52, 54, 56, 59, 61, 64, 66, 69, 72, 75, 78, 81,
	84, 87, 90, 94, 97, 101, 104, 108, 112, 116, 120, 124, 128, 132, 137, 141,
	146, 150, 155, 160, 165, 170, 175, 180, 185, 191, 196, 202, 208, 213, 219, 225,
	231, 237, 243, 249, 255
};

void setup_soft_blink(void) {
	TCCR0A = (1 << WGM00) | (1 << COM0A1);   // WGM00 = Phase correct PWM    COM0A1 = non inverting PWM    | gitt disse vil høyt PWM signal gi gøy verdi på PD6
	TCCR0B = (1 << CS00);    // denne bestemmer prescaler
	OCR0A = 0;   //OCR0A er rigsiter som styrer PD6. Her er det verdier fra [0-255]
	DDRD |= (1 << PD6);    //  angir PD6 som utgang 
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
   


void soft_blink_loop(void) {
	// Fade in
	for (uint8_t i = 0; i < 255; i++) {
		OCR0A = pgm_read_byte(&gamma_table[i]);
		_delay_ms(56);
	}
	
	// Fade out
	for (uint8_t i = 255; i > 0; i--) {
		OCR0A = pgm_read_byte(&gamma_table[i]);
		_delay_ms(56);
	}
}

int main(void) {
	setup_soft_blink();
	while(1) { soft_blink_loop(); }
}