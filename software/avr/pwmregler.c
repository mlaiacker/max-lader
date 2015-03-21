/* 
* max-Lader 
* entwickelt von:
* Maximilian Laiacker post@mlaiacker.de http://mlaiacker.de/
*/


#include "global.h"
#include "pwmregler.h"
#include "pwm.h"
#include "a2d.h"

tRegler regler;

void pwmregler(void)
{
	s16 div;
	regler.iOut = a2dAvg(IOUT);
//	if(regler.iOut<0) regler.iOut = 0;
	regler.uOut = a2dAvg(UOUT) - U_SENS(regler.iOut);
	regler.error = 0;
	if((regler.status == REGLER_STATUS_ON) && (errorn==0))
	{	
		if((regler.uSoll == 0) || (regler.iMax == 0))// || (regler.uOut > PWM_UMAX))
		{
			regler.pwm = 0;
		}
		else
		{
			if(regler.iOut>=IOUT_GRENZWERT)
			{
				if(regler.uOut <= U_KURZSCHLUSS) {
					errorn = ERROR_KURZSCHLUSS;
					regler.pwm =0;
				}
			}
			div=((regler.iMax/(16)) + 1);
			if( (((regler.iOut + div/2) >= regler.iMax) && (regler.uOut < regler.uSoll)) )
			{
				regler.error = (regler.iMax - regler.iOut - div/4) / div ; // strombegrenzung
			} else
			{
//				#ifdef ADS7822
				regler.error = ((regler.uSoll - regler.uOut)); // spannungsregelung
				if(regler.error>0) regler.error /=(regler.uSoll/700+1);// spannung nicht grösser werden lassen als soll
//				#else
//				regler.error = ((regler.uSoll - regler.uOut)/(regler.uSoll/256+1)); // spannungsregelung
//				#endif
			}
			if(regler.error>2*PWM_DIV) regler.pwm += 2*PWM_DIV;
			else if(regler.error<-2*PWM_DIV) regler.pwm -= 2*PWM_DIV;
			else regler.pwm += (regler.error);
		}
	}
	else
	{
		regler.pwm=0;
	}
	
	if(regler.pwm>PWM_FULL_UP) regler.pwm = PWM_FULL_UP;
	else 
	if(regler.pwm<0) regler.pwm = 0;
	pwmSet(regler.pwm/PWM_DIV);
}
// die geladene kapazität aufsummieren, hier sind es aber noch rohdaten, umgerechnet wird in laden.c
/*void pwmreglerCOut(void)
{
}
*/
// jede sekunde de status des reglers überprüfen und eventuell fehler auslösen
 void pwmreglerSekunde(void)
{
	long temp_adc = a2dAvg(TEMP)>>2;
	regler.temp = BITS2TEMP(temp_adc);
	if(errorn) return;
	
	regler.uIn = BITS2UIN(a2dAvg(UIN));
	if((regler.uIn <= UIN_MIN)||(regler.uIn >= UIN_MAX))
	{
		errorn = ERROR_EINGANGSSPANNUNG;
	}
	#ifdef TEMP_SENS
	if(impulsverz(TEMP_KRITISCH(regler.temp),10,&regler.vTemp))
	{
		errorn = ERROR_TEMP;
	}
	#endif
	
	if(impulsverz((regler.error >= REGLER_NORMAL*PWM_DIV) || (regler.error <= -REGLER_NORMAL*PWM_DIV)||(regler.pwm >= PWM_FULL_UP),REGLER_UNORMAL_GRENZ,&regler.unnormal)) errorn = ERROR_WANDLER;
	
}	
