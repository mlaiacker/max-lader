#ifndef HARDWARE_H
#define HARDWARE_H

#if defined (__AVR_ATmega168__)
#include "migration8-168.h"
#endif

#define LADER_VERSION	(19)

#ifndef F_CPU
	#if defined (__AVR_ATmega168__)
		#define F_CPU        16000000L              		// 16MHz processor
	#elif defined (__AVR_ATmega8__)
		#define F_CPU        16000000L               		// 16MHz processor
	#endif
#endif

/* ### ADC ### */

#define UOUT_LOW	3
#define UOUT		1
#define UIN			0
#define IOUT		2
//#define TASTEN	3
#define TEMP		4


// Refferenzspannung 2.500V
#define U_REF	250 // in mv/10

// Strommessungs Shunt
//#define R_SENS		(0.1/3)	//Ohm

// Verstärkung der Strommessung = 11
// 1 bit = 2500mV*3/(4096*11*0.1Ohm) = 9375/5632 ca.: 5/3 mA
// der maximal messbare Strom liegt damit bei 6,826 A



// für SHUNT=0.1ohm/3 Imax=5.5A
//#define IOUT2BITS(x) ((x)*3/5)
#define IOUT2BITS(x) ((x)*(4096L*11L)/(2500L*30L))
//#define BITS2IOUT(x) ((x)*5/3)
#define BITS2IOUT(x) ((x)*2500L*30L/(4096L*11L))
#define IOUT_GRENZWERT	IOUT2BITS(6200L)
// pro bit ausgangsstrom sinkt die Klemmenspannung (in bits) um diesen Wert bei 0.1/3 Ohm shunt
#define U_SENS(I)	((I)/121) //BITS2IOUT(I)*R_SENS*UOUT2BITS(x) = 9375/5632 * 0.1/3 * 2048/1375 = 1/121
#define IOUT_MAX		(6000) // mA

/*
// für doppelten strom
// für SHUNT=0.1ohm/6 Imax=9.95A
#define IOUT2BITS(x) ((x)*(4096L*11L)/(2500L*60L))
#define BITS2IOUT(x) ((x)*2500L*60L/(4096L*11L))
#define IOUT_GRENZWERT	IOUT2BITS(11000L)
// pro bit ausgangsstrom sinkt die Klemmenspannung (in bits) um diesen Wert
#define U_SENS(I)		((I)/60L)
#define IOUT_MAX		(9950L) // mA
*/


//spannungsteiler 10/110
#define UOUT2BITS(x) ((short)(((long)(x)<<11L)/1375L))	

#define BITS2UOUT(x) ((short)(((long)(x)*1375L)>>11L))	//10*mV (250*110)/(10*4096) = 2750/5096 = 1375/2048

// spannungsteiler für eingangsspannung
// gleicher teiler wie am ausgang (1/11)
#if defined (__AVR_ATmega168__)
	#define UIN2BITS(x) UOUT2BITS(x)
	#define BITS2UIN(x) BITS2UOUT(x)
#elif defined (__AVR_ATmega8__)
//250*11/1024...
	#define UIN2BITS(x) (((x)*3)>>1)
	#define BITS2UIN(x) (((x)<<3)/12) //10*mV (Teiler=1/11 )
#endif


// spannungsteiler 10/57
#define BITS2UOUTL(x) ((short)(((long)(x)*1425L)>>12L))	//10*mV (250*57)/(10*4096) = 1425/4096

#define UOUT_LOW_AKTIV	(1300)	//unterhalb dieser spannung uout_low benutzen

// umrechnung von adc werten (10bit) in temeratur in °C
#define BITS2TEMP(x) ((x)*(x)/1864L + (x)*100/-282 + 83L) // aus messwerten ermittelt und dann mit matlab(basic fitting) ausgerechnet

#define ADC_PRESCALE			ADC_PRESCALE_DIV64
#define ADC_REFERENCE			ADC_REFERENCE_AREF

#define A2D_SCAN_CHANNELS	(5)
#define A2D_SCAN_BUF	(16)

// oversampling benutzen statt laufende mittelwert bildung
//#define OVERSAMPLING
#define OVERSAMPLING_RATE		8
/* Laden*/

#define P_MAX			(10000) //10*mW  maximale Ausgangsleisung

/* ### Beep ### */
#define BEEP_DDR		DDRB
#define BEEP_PORT		PORTB
#define BEEP_BIT		0
#define BEEP_ON			sbi(BEEP_PORT,BEEP_BIT)
#define BEEP_OFF		cbi(BEEP_PORT,BEEP_BIT)
#define BEEP_INIT		(BEEP_DDR |= (1<<BEEP_BIT))

#define BEEP_LADEN_COUNT 5 // wie offt peipt's wenn die ladung beendet wurde
#define BEEP_ERROR_COUNT 120

/* ### LCD ### */
#define LCD_PORT_INTERFACE
#define LCD_CTRL_PORT	PORTB
#define LCD_CTRL_DDR	DDRB
#define LCD_CTRL_RS		3
#define LCD_CTRL_RW		4
#define LCD_CTRL_E		5
#define LCD_CTRL_E_PORT	PORTB
#define LCD_CTRL_E_DDR	DDRB
// port you will use for data lines
#define LCD_DATA_POUT	PORTD
#define LCD_DATA_PIN	PIND
#define LCD_DATA_DDR	DDRD
// access mode you will use (default is 8bit unless 4bit is selected)
#define LCD_DATA_4BIT

#define LCD_LINES				2	// visible lines
#define LCD_LINE_LENGTH			16	// line length (in characters)

//#define LCD_DELAY	asm volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n nop\n");
#define LCD_DELAY	asm volatile ("nop\n nop\n nop\n nop\n nop\n nop\n nop\n");

/* ### Tasten ### */
//#define TASTEN_ADC
// Tasten sind am ADC3
#define TASTE_L	(bit_is_clear(TASTE_L_PIN,TASTE_L_BIT))
#define TASTE_M	(bit_is_clear(TASTE_M_PIN,TASTE_M_BIT))
#define TASTE_R	(bit_is_clear(TASTE_R_PIN,TASTE_R_BIT))

// tasten sind an einzelnen eingängen
#define TASTE_L_PORT	PORTC
#define TASTE_L_PIN		PINC
#define TASTE_L_BIT		5
// pullup einschalten
#define TASTE_L_INIT (TASTE_L_PORT|=(1<<TASTE_L_BIT))

#define TASTE_M_PORT	PORTD
#define TASTE_M_PIN		PIND
#define TASTE_M_BIT		2
#define TASTE_M_INIT (TASTE_M_PORT|=(1<<TASTE_M_BIT))

#define TASTE_R_PORT	PORTD
#define TASTE_R_PIN		PIND
#define TASTE_R_BIT		3
#define TASTE_R_INIT (TASTE_R_PORT|=(1<<TASTE_R_BIT))


/* ### PWM ### */
#define PWM1 // PWM1A und PWM1B sind für UP und DOWN

//#define PWM12 // timer2 ist für PWM_UP //timer 1 ist für down

#define PWM_UP_PORT		PORTB
#define PWM_UP_DDR		DDRB
#define PWM_UP_BIT		2
//#define PWM_UP_9BIT

#define PWM_DOWN_PORT	PORTB
#define PWM_DOWN_DDR	DDRB
#define PWM_DOWN_BIT	1
//#define PWM_DOWN_9BIT

#define PWM_MAX		(255) // maximum der up stufe
#define PWM_DOWN_MAX	(255)
#define PWM_UP_MAX		((PWM_MAX+PWM_DOWN_MAX)-PWM_MAX*3/10) // maximale pulsbreite für die UP stufe

#define PWM_DIV	(64)
#define PWM_FULL_DOWN	((PWM_DOWN_MAX)*PWM_DIV)
#define PWM_FULL_UP		((PWM_UP_MAX)*PWM_DIV)
#define PWM2PROZENT(x)		(((x)/PWM_DIV)*10/51)

#define PWM_FREQ	(F_CPU/256L)

#define PWM_REGEL_FREQ	(F_CPU/(128L*A2D_SCAN_CHANNELS*4L)) // F_CPU/ADC_PRESCALE/A2D_SCAN_CHANNELS/4

/* ### PWM REGLER ### */

#define REGEL_COUT_INT	(MENU_DISPLAY_INT)
#define REGEL_COUT_FREQ  (1000L/REGEL_COUT_INT)
#define BITS2COUT(x)	(((u32)(x))/(REGEL_COUT_FREQ*60L*60L*10L)) //mAh (REGEL_FREQ*60*60)
#define COUT2BITS(x)	(((u32)(x))*(REGEL_COUT_FREQ*60L*60L*10L)) //mAh (REGEL_FREQ*60*60)

#define UIN_MIN		( 950) // minimale eingangsspannung in 10*mV
#define UIN_MAX		(1700) // maximale eingangsspannung in 10*mv
#define U_KURZSCHLUSS	UOUT2BITS(100)
#define PWM_UMAX	(4091)

/* ### Überwachung ### */
#define TEMP_SENS // wir haben einen temperatur sensor
#define TEMP_KRITISCH(temp) (temp>68)	// wann spricht die Temperatur Überwachung an
#define REGLER_NORMAL	8 // normale abweichung der Regelung vom sollwert
// wieviele Sekunden muss eine unnormale Regler Situation bestehen damit ein Fehler ausgelöst wird
#define REGLER_UNORMAL_GRENZ 10

/* ### uart ### */
#define UART // wir machen Ausgaben über die serielle Schnittstelle
// Baudrate
#define UART_BAUD_RATE	38400
#if defined (__AVR_ATmega168__)
//	#define DEBUG_OSC

#elif defined (__AVR_ATmega8__)
#if UART_BAUD_RATE==9600
	#define UART_UBRRL	(103) // 9600@16MHz U2X = 0
#elif UART_BAUD_RATE==38400
	#define UART_UBRRL	(25) // 38400@16MHz U2X = 0
#else
#error "unsupported baudrate"
#endif
#endif


#endif
