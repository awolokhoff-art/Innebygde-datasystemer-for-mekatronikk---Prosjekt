
#define F_CPU 1000000UL
#include <avr/io.h>
#include <util/delay.h>

// Precomputed gamma (?2.2) correction table for 8-bit brightness values
const uint8_t gamma_table[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2,
	2, 2, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5, 6, 6, 7, 7,
	8, 8, 9, 9, 10, 11, 11, 12, 13, 13, 14, 15, 16, 17, 18, 19,
	20, 21, 22, 23, 24, 25, 27, 28, 29, 31, 32, 34, 35, 37, 38, 40,
	42, 43, 45, 47, 49, 51, 53, 55, 57, 59, 61, 64, 66, 68, 71, 73,
	76, 78, 81, 84, 87, 89, 92, 95, 98, 101, 104, 108, 111, 114, 117, 121,
	124, 128, 132, 135, 139, 143, 147, 151, 155, 159, 163, 168, 172, 177, 181, 186,
	191, 195, 200, 205, 210, 215, 220, 225, 231, 236, 241, 247, 252, 255
};

int main(void) {
	DDRD |= (1 << PD6); // PD6 = OC0A output

	// Fast PWM, non-inverting mode, prescaler = 8
	TCCR0A = (1 << WGM00) | (1 << WGM01) | (1 << COM0A1);
	TCCR0B = (1 << CS01);

	while (1) {
		
		// Fade up
		for (uint8_t i = 0; i < 255; i++) {
			OCR0A = gamma_table[i];
			_delay_ms(100);
		}
		// Fade down
		for (int i = 255; i >= 0; i--) {
			OCR0A = gamma_table[i];
			_delay_ms(160);

		}
	}
}
