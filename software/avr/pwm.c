/* 
* max-Lader 
* entwickelt von:
* Maximilian Laiacker post@mlaiacker.de http://mlaiacker.de/
*/


#include <avr/io.h>
#include "global.h"
#include "PWM.h"


/*
void pwmInit(void){
  OCR1AH = 0; // highbyte setzen
  OCR1BL = 0;
// Cleared on compare match, up-counting. Set on compare match, down-counting (non-inverted PWM). 8Bit FastPWM
  TCCR1A = (1<<COM1A1|0<<COM1A0|1<<COM1B1|1<<COM1B0|0<<WGM11|1<<WGM10); 
// fTCK1 = CK 00000001
  TCCR1B = (0<<WGM13|1<<WGM12|1<<CS10|0<<CS11|0<<CS12);

  
//  OCR2 = 0; // pwm=0;
// fast PWM
//  TCCR2 = 1<<WGM20|1<<WGM21|1<<COM21|1<<COM20|1<<CS20|0<<CS21|0<<CS22; 
// F=Fcpu

  cbi(PORTB,2); // port = 0
  sbi(DDRB,2); // OC1B
}
*/

/* 0..511 */
void pwmSet(signed short value)
{
  if(value>PWM_DOWN_MAX)
  {
	if (value > PWM_UP_MAX) value = PWM_UP_MAX;
	sbi(PWM_UP_DDR,PWM_UP_BIT); // OC1B = output
    sbi(PWM_DOWN_DDR,PWM_DOWN_BIT); // OC1A = output
	OCR1AL = PWM_DOWN_MAX;
	OCR1BL = (value - PWM_DOWN_MAX); // up
  } else
  {
    OCR1BL = 0;
	if (value<=0)
	{
		cbi(PWM_DOWN_DDR,PWM_DOWN_BIT); // OC1A = input
		OCR1AL = 0; // down
	} else 
	{
		sbi(PWM_DOWN_DDR,PWM_DOWN_BIT); // OC1A = output
		OCR1AL = value; // down
	}
  }
}

