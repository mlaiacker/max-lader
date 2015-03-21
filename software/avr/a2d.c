/*! \file a2d.c \brief Analog-to-Digital converter function library. */
//*****************************************************************************
//
// File Name	: 'a2d.c'
// Title		: Analog-to-digital converter functions
// Author		: Pascal Stang - Copyright (C) 2002
// Created		: 2002-04-08
// Revised		: 2002-09-30
// Version		: 1.1
// Target MCU	: Atmel AVR series
// Editor Tabs	: 4
//
// This code is distributed under the GNU Public License
//		which can be found at http://www.gnu.org/licenses/gpl.txt
//
//*****************************************************************************

#include <avr/io.h>
#include <avr/interrupt.h>

#include "global.h"
#include "a2d.h"
#include "pwmregler.h"
#include "pwm.h"

// global variables

//! software flag used to indicate when
//! the a2d conversion is complete
//volatile unsigned short a2dCompleteFlag;

#ifdef OVERSAMPLING
short unsigned	a2dBuf[];
short unsigned a2dChan[];
#else
unsigned short	a2dBuf[A2D_SCAN_CHANNELS][A2D_SCAN_BUF];
#endif
unsigned char a2dIndex=0;
unsigned char a2dChannel=0;
#ifdef ADS7822
short adsResult, adsTmpVal;
#endif
//extern volatile unsigned short a2dIntFlag = 0;

// functions

//! initialize a2d converter
/*
void a2dInit(void)
{
//	sbi(ADCSR, ADEN);				// enable ADC (turn on ADC power)
//	cbi(ADCSR, ADFR);				// default to single sample convert mode

//	a2dSetPrescaler(ADC_PRESCALE);	// set default prescaler
//	a2dSetReference(ADC_REFERENCE);	// set default reference
//	cbi(ADMUX, ADLAR);				// set to right-adjusted result
	ADMUX = (0<<ADLAR) | (ADC_REFERENCE<<6);
//	sbi(ADCSR, ADIE);				// enable ADC interrupts
//	a2dIndex = 0;
//	a2dChannel = 0;
//	a2dIntFlag=0;
//	sbi(ADCSR, ADFR);				// enable free runnig mode
//	a2dSetChannel(0);
//	sei();							// turn on interrupts (if not already on)
//	a2dStartConvert();
//	sbi(ADCSR, ADIF);	// clear hardware "conversion complete" flag 
//	sbi(ADCSR, ADSC);	// start conversion
	ADCSR = ((0<<ADFR)|(1<<ADEN)|(1<<ADIE)|(1<<ADSC)|(1<<ADIF) | ADC_PRESCALE);
}
*/


#ifdef OVERSAMPLING
short a2dAvg(unsigned char ch)
{
	return (short)a2dChan[ch];
}
#else
short a2dAvg(unsigned char ch)
{
	unsigned long sum = 0;
	unsigned char i;
	for(i=0;i<A2D_SCAN_BUF;i++)	sum += a2dBuf[ch][i];
	#ifdef ADS7822
	if(ch==ADS_CHANNEL) 	return (short)(sum/(A2D_SCAN_BUF)); // echte 12 bit
	#endif
	return (short)(sum/(A2D_SCAN_BUF/4)); // virtuelle 12 bit
}
#endif
//! interrupt handler for ADC complete interrupt
#ifdef OVERSAMPLING
ISR(ADC_vect)
{
	unsigned char i;
	a2dBuf[a2dChannel] += (ADCL) | ((ADCH)<<8);
	a2dChannel++;
	a2dChannel %= A2D_SCAN_CHANNELS;
	ADMUX = ((ADMUX) & ~ADC_MUX_MASK) | (a2dChannel & ADC_MUX_MASK);
	sbi(ADCSR, ADSC);	// start conversion
	adsResult = ads7822Step(a2dChannel);
	if(a2dChannel == 0)
	{ 
		a2dBuf[ADS_CHANNEL] += adsTmpVal;
		a2dIndex++;
		if(a2dIndex>=OVERSAMPLING_RATE)
		{
			for(i=0;i<=A2D_SCAN_CHANNELS;i++)
			{ 
				a2dChan[i] = a2dBuf[i]/(OVERSAMPLING_RATE);
				a2dBuf[i] = 0;
			}
			a2dIntFlag=1;
			a2dIndex=0;
		}
		
	} else 	if(a2dChannel==1)
	{
		adsTmpVal = adsResult<<8;
	} else if(a2dChannel==2)
	{
		adsTmpVal = ((adsTmpVal+adsResult)>>1)&0x0fff;		
	}

}
#else
ISR(ADC_vect)
{
	a2dBuf[a2dChannel][a2dIndex] = (ADCL) | ((ADCH)<<8);
	a2dChannel++;
	a2dChannel %= A2D_SCAN_CHANNELS;
	ADMUX = ((ADMUX) & ~ADC_MUX_MASK) | (a2dChannel & ADC_MUX_MASK);
	sbi(ADCSR, ADSC);	// start conversion
#ifdef ADS7822
	adsResult = ads7822Step(a2dChannel);
#endif
	if(a2dChannel == 0)
	{ 
#ifdef ADS7822
		a2dBuf[ADS_CHANNEL][a2dIndex] = adsTmpVal;
#endif
		a2dIndex =(a2dIndex+1)&(A2D_SCAN_BUF-1);
		
		if(a2dIndex%4==0) 	pwmregler(); // nach 4 werten aufrufen 
	}
#ifdef ADS7822
	else 	if(a2dChannel==1)
	{
		adsTmpVal = adsResult<<8;
	} else if(a2dChannel==2)
	{
		adsTmpVal = ((adsTmpVal+adsResult)>>1)&0x0fff;		
	}
#endif
}
#endif

#define bits2uout BITS2UOUT
#define bits2iout BITS2IOUT

