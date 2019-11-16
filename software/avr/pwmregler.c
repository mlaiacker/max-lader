/* 
* max-Lader 
* entwickelt von:
* Maximilian Laiacker post@mlaiacker.de http://mlaiacker.de/
*/


#include "global.h"
#include "pwmregler.h"
#include "pwm.h"
#include "a2d.h"

/* pwm control parameter for current */
#define I_KI	16
#define I_KP	16
#define I_KD	256

/* pwm control parameter for voltage */
#define U_KI	8
#define U_KP	2
#define U_KD	256

tRegler regler;

#if defined (__AVR_ATmega168__)

osc_data_t oscData;

void oscReset(u08 trigger_ch, u08 edge, s16 trigger_level)
{
	oscData.trigger = trigger_level;
	oscData.trigger_edge = edge;
	if(trigger_ch==0)
	{
		oscData.trigger_edge = 4;
		trigger_ch = 1;
	}
	oscData.dt = 0;
	oscData.trigger_ch = trigger_ch;
}

void oscSample(s16 ch1, s16 ch2, u16 dt, u08 trigger)
{
	if((oscData.trigger_ch==1 || oscData.trigger_ch==2) && oscData.dt!=0){
		u08 data = ch1;
		u08 triggerd=0;
		if(oscData.trigger_ch==2) data = ch2;
		if(oscData.trigger_edge&1) // rising
		{
			if(oscData.data[oscData.trigger_ch-1][oscData.index] <= oscData.trigger && // old value
					data > oscData.trigger	){ // new value
				triggerd=1;
			}
		}
		if(oscData.trigger_edge&2) // falling
		{
			if(oscData.data[oscData.trigger_ch-1][oscData.index] >= oscData.trigger && // old value
					data < oscData.trigger	){ // new value
				triggerd=1;
			}
		}
		if(oscData.trigger_edge==4 && trigger) triggerd=1;
		if(triggerd){
			oscData.trigger_index = oscData.index;
			oscData.trigger_ch=0;
		}
	}
	oscData.dt = dt;
	if(oscData.trigger_ch!=255)
	{
		oscData.index++;
		if(oscData.index>=OSC_COUNT){
			oscData.index=0;
		}
		if(oscData.trigger_ch==0 && oscData.trigger_index==(oscData.index+OSC_COUNT/2)%OSC_COUNT)
		{
			oscData.trigger_ch=255; // finished
		}
		oscData.data[0][oscData.index] = ch1;
		oscData.data[1][oscData.index] = ch2;
	}

}
#else
#define oscSample(a, b, c, d)
#define oscReset(a,b,c)
#endif

void pwmregler(void)
{
	s16 div;
	regler.iOut = a2dAvg(IOUT);
	regler.uOut = a2dAvg(UOUT) - U_SENS(regler.iOut);
	regler.error = 0;
	if((regler.status == REGLER_STATUS_ON) && (errorn==0))
	{	
		if((regler.uSoll == 0) || (regler.iMax == 0))// || (regler.uOut > PWM_UMAX))
		{
			regler.pwm = 0;
		} else
		{
			if(regler.iOut>=IOUT_GRENZWERT)
			{
				if(regler.uOut <= U_KURZSCHLUSS) {
					errorn = ERROR_KURZSCHLUSS;
					regler.pwm =0;
				}
			}
			div=((regler.iMax/(16)) + 1);
			if( (((regler.iOut + div) >= regler.iMax) && (regler.uOut < regler.uSoll-1)) )
			{
				regler.error = (regler.iMax - regler.iOut) ; // strombegrenzung
//				s16 errorD = regler.error - regler.error_last;
				if((regler.pwm + regler.error/I_KI)<=PWM_FULL_UP && (regler.pwm + regler.error/I_KI)>=-PWM_DIV) // anti windup
					regler.errorI += regler.error/I_KI;
				regler.pwm = regler.error/I_KP + regler.errorI;// + errorD/I_KD;
//				regler.error_last = regler.error;
			} else
			{
				regler.error = (regler.uSoll - regler.uOut); // spannungsregelung
				//s16 errorD = regler.error - regler.error_last;
				if((regler.pwm + regler.error/U_KI)<=PWM_FULL_UP && (regler.pwm + regler.error/U_KI)>=-PWM_DIV) // anti windup
					regler.errorI += (regler.error-(U_KI-1))/U_KI;
				regler.pwm = regler.error/U_KP + regler.errorI;// + errorD/KD;
				//regler.error_last = regler.error;
			}
		}
	}
	else
	{
		regler.pwm=0;
		regler.errorI = 0;
	}
	
	oscSample(regler.uOut,regler.iOut,80,0);
	if(regler.pwm>PWM_FULL_UP){
		regler.pwm = PWM_FULL_UP;
	}else if(regler.pwm<0){
		regler.pwm = 0;
	}
	pwmSet(regler.pwm/PWM_DIV);
}

// jede sekunde de status des reglers überprüfen und eventuell fehler auslösen
 void pwmreglerSekunde(void)
{
	long temp_adc = a2dAvg(TEMP)>>2;
	regler.temp = BITS2TEMP(temp_adc);
	if(errorn) return;
	
	regler.uIn = BITS2UIN(a2dAvg(UIN));
	/* check input voltage is outside range for 5 seconds */
	if(impulsverz((regler.uIn <= UIN_MIN)||(regler.uIn >= UIN_MAX), 5, &regler.et_uin))
	{
		errorn = ERROR_EINGANGSSPANNUNG;
	}
	#ifdef TEMP_SENS
	/* check temperature to high fo 10 seconds */
	if(impulsverz(TEMP_KRITISCH(regler.temp),10,&regler.et_temperature))
	{
		errorn = ERROR_TEMP;
	}
	#endif
	/* check if pwm controller error is too high */
	if(impulsverz((regler.error >= REGLER_NORMAL*PWM_DIV) || (regler.error <= -REGLER_NORMAL*PWM_DIV)/* abs error to high */
			|| (regler.pwm >= PWM_FULL_UP) /* overload */
//			 ||(regler.pwm<PWM_DIV && regler.status==REGLER_STATUS_ON && regler.error !=0) /* pwm at zero but still error */
			,REGLER_UNORMAL_GRENZ, &regler.et_unnormal)) errorn = ERROR_WANDLER;
	
#if defined (__AVR_ATmega168__)
	if(oscData.trigger_ch==254)
	{
		oscReset(2,3,regler.iMax*9/10+10);
	}
#endif
}	
