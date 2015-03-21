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
	s16 n,freq,error;
	u32 cOut; // ausgegebene kapazität
	u08 status,unnormal,vTemp;
} tRegler;

extern tRegler regler;

void pwmregler(void);
void pwmreglerSekunde(void);
void pwmreglerCOut(void);

#endif
