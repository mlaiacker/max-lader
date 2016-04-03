/* 
* max-Lader 
* entwickelt von:
* Maximilian Laiacker post@mlaiacker.de http://mlaiacker.de/
*/


#ifndef LADERMENU_H
#define LADERMENU_H

#include "laden.h"
#include <avr/pgmspace.h>

#define TASTE_BIT_ON	0 // taste gedrückt
#define TASTE_BIT_FP	1 // tasten flanke positiv
#define TASTE_BIT_FN	2 // tasten flanke negativ
#define TASTE_BIT_LANG	3 // taste lang gedrückt
#define TASTE_T_LANG		(20) // lang gedrückt [aufrufe]
#define TASTE_T_INTERVAL	(MENU_DISPLAY_INT)	// abfrage interwall ms

#define TASTE_ON(taste)		(taste.flags&(1<<TASTE_BIT_ON))
#define TASTE_FP(taste)		(taste.flags&(1<<TASTE_BIT_FP))
#define TASTE_FN(taste)		(taste.flags&(1<<TASTE_BIT_FN))
#define TASTE_LANG(taste)		(taste.flags&(1<<TASTE_BIT_LANG))

#define STEP_I	(50) //mA
#define STEP_U	(10) // mV/10

#define MENU_DISPLAY_INT	(50L)

typedef struct 
{
	s16 param2,iMax;
	u08 checkSum;
} tSettings;

#define MENU_MAX_NUM		(3)
typedef enum
{
	eMenuStart,
	eMenuI,
	eMenuParam2,
	eMenuProg
} eMenu;

typedef enum
{
	eLadenTimeC,
	eLadenUinT,
//	eLadenPhase,
	eLadenRaw,
	eLadenDt,
	eLadenBalancer,
	eLadenBalancer2
} eLaden;
#define LADEN_MAX_NUM		(eLadenBalancer2)

typedef struct 
{
	short	t;
	u08 flags;
} tTaste;


typedef struct 
{
	u08 edit,blinken;
	u08 changed;
	s16 ladenNum;
	s16 topNum;
	s16 uOut,iOut;
} tMenu;


// globale Variablen
extern tSettings settings;
extern tMenu menu;
extern tSettings eeSettings[MODE_NUM] EEPROM;
extern tTaste	tasteL,tasteM,tasteR;
extern unsigned char eeChargeMode EEPROM;

typedef char tModeString[5];
extern const tModeString modeName[MODE_NUM+2]	PROGMEM;

// eeprom zeugs

void ladermenuInit(void);
void ladermenu(void);
void saveSettings(eMode mode);
void readSettings(eMode mode);
void readTaste(tTaste *t, u08 input);
void showMenu(void);
#endif
