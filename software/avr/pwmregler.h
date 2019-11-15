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
	s16 n,freq,error,errorI,error_last;
	u32 cOut; // ausgegebene kapazität
	u08 status,unnormal,vTemp;
} tRegler;

extern tRegler regler;

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

void oscReset(u08 trigger_ch, u08 edge, s16 trigger_level);

void pwmregler(void);
void pwmreglerSekunde(void);
void pwmreglerCOut(void);

#endif
