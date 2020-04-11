/* 
* max-Lader 
* entwickelt von:
* Maximilian Laiacker post@mlaiacker.de http://mlaiacker.de/
*/


#include "global.h"
#include "pwmregler.h"
#include "pwm.h"
#include "a2d.h"

/* pwm control parameter for current (inverse gain)*/
#define I_KI	64
#define I_KP	32

/* pwm control parameter for voltage (inverse gain)*/
#define U_KI	16
#define U_KP	4

#define CONTROL_HIST 5

tRegler regler;

#ifdef DEBUG_OSC

osc_data_t oscData;

void oscReset(u08 trigger_ch, u08 edge, s16 trigger_level, u16 dt)
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
	oscData.dt = dt;
}

void oscSample(s16 ch1, s16 ch2, u08 trigger)
{
	if((oscData.trigger_ch==1 || oscData.trigger_ch==2) && oscData.dt!=0){
		s16 data = ch1;
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
		if(trigger) triggerd=1;
		if(triggerd){
			oscData.trigger_index = oscData.index;
			oscData.trigger_ch=0;
		}
	}
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
#define oscSample(a, b, c)
#define oscReset(a,b,c,d)
#endif

void pwmregler(void)
{
	s16 div;
	u08 trigger = 0;
	regler.iOut = a2dAvg(IOUT);
	regler.uOut = a2dAvg(UOUT) - U_SENS(regler.iOut);
	regler.error = 0;
	if((regler.status == REGLER_STATUS_ON) && (errorn==0) && (regler.uSoll > 0)){
			if((regler.iOut>=IOUT_GRENZWERT) && (regler.uOut <= U_KURZSCHLUSS)) {
					errorn = ERROR_KURZSCHLUSS;
					regler.pwm =0;
					trigger = 1;
			}

			div=((regler.iMax/(16)) + 1);
			if((regler.iOut + div) >= regler.iMax && regler.current_active<CONTROL_HIST){
				regler.current_active++;
				if(regler.current_active>=CONTROL_HIST){
					regler.mode = 1; // current control
					trigger = 1;
				}
			}
			if(regler.uOut > regler.uSoll && regler.current_active>0){
				regler.current_active--;
				if(regler.current_active==0){
					regler.mode = 0; // voltage control
					trigger = 1;
				}
			}

			s16 error_u = (regler.uSoll - regler.uOut);
			if(regler.mode)
			{
				regler.error = (regler.iMax - regler.iOut) ; // strombegrenzung
				if(error_u<0)
					regler.error += error_u*2;
				s16 changeI = regler.error/I_KI;
				if(changeI==0){
					if(regler.error>0) changeI=1;
					if(regler.error<0) changeI=-1;
				}
				if((regler.pwm + changeI)<=PWM_FULL_UP && (regler.pwm + changeI)>=-PWM_DIV) // anti windup
					regler.errorI += changeI;
				regler.pwm = regler.error/I_KP + regler.errorI;
				if(regler.iMax==0)  regler.pwm=0;
			} else {
				regler.error = error_u; // spannungsregelung
				s16 changeU = regler.error/U_KI;
				if(changeU==0){
					if(regler.error>0) changeU = 1;
					if(regler.error<0) changeU = -1;
				}
				if((regler.pwm + changeU)<=PWM_FULL_UP && (regler.pwm + changeU)>=-PWM_DIV) // anti windup
					regler.errorI += changeU;
/*				if((regler.uOut-1) > regler.uSoll && regler.errorI>0){
					regler.errorI--; // prevent over voltage
				}*/
				regler.pwm = regler.error/U_KP + regler.errorI;
			}
	} else {
		regler.pwm=0;
		regler.errorI = 0;
	}
	
//	oscSample(regler.error, regler.iOut, trigger);
	oscSample(regler.uOut, regler.iOut, trigger);
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
	if(impulsverz(((regler.error >= REGLER_NORMAL*PWM_DIV) || (regler.error <= -REGLER_NORMAL*PWM_DIV)/* abs error to high */
			|| (regler.pwm >= PWM_FULL_UP) /* overload */
			|| (regler.iOut > (regler.iMax + IOUT2BITS(500))) /* overcurrent */
			)
			&& (regler.status==REGLER_STATUS_ON)
//			 ||(regler.pwm<PWM_DIV && regler.status==REGLER_STATUS_ON && regler.error !=0) /* pwm at zero but still error */
			,REGLER_UNORMAL_GRENZ, &regler.et_unnormal)) errorn = ERROR_WANDLER;
	
#ifdef DEBUG_OSC
	if(oscData.trigger_ch==254)
	{
		oscReset(2,3,regler.iMax*9/10+10, 80); // trigger on current
//		oscReset(1,3,0, 80); // trigger on ch1 0 crossing (error)
	}
#endif
}	
