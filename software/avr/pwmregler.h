/* 
* max-Lader 
* entwickelt von:
* Maximilian Laiacker post@mlaiacker.de http://mlaiacker.de/
*/


#ifndef PWMREGLER_H
#define PWMREGLER_H

#include "ladermenu.h"
#include "global.h"

#define REGLER_STATUS_OFF	0
#define REGLER_STATUS_ON	1
//#define REGLER_STATUS_UIN	2
//#define REGLER_STATUS_TEMP	3



// alles in bits, nicht in echten einheiten
typedef struct 
{
	s16 uSoll,iMax; // sollwete in bits
	s16 pwm,uIn,uOut,iOut,temp; // messwete in bits
	s16 error,errorI;
	u08 status, current_active, mode;
	u08 et_unnormal,et_uin, et_temperature; // error timer state
} tRegler;

extern tRegler regler;

#ifdef DEBUG_OSC
#define OSC_COUNT	120
typedef struct
{
	u16 dt; ///< in us
	u08 index;
	u08 trigger_index;
	s16 trigger;
	u08 trigger_ch; // 0 means triggered; 255 means finished;254 means send via uart
	u08 trigger_edge; // 3=any, 1=rising ,2=falling, 4=external
	s16 data[2][OSC_COUNT];
}osc_data_t;
extern osc_data_t oscData;
void oscReset(u08 trigger_ch, u08 edge, s16 trigger_level,  u16 dt);
#endif


/* is called from ADC interrupt every 80us */
void pwmregler(void);

/* must be called every second */
void pwmreglerSekunde(void);

#endif
