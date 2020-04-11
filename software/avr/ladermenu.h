/* 
* max-Lader 
* entwickelt von:
* Maximilian Laiacker post@mlaiacker.de http://mlaiacker.de/
*/


#ifndef LADERMENU_H
#define LADERMENU_H

#include "laden.h"
#include <avr/pgmspace.h>


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
//extern tSettings eeSettings[] EEPROM;
//extern tTaste	tasteL,tasteM,tasteR;
extern unsigned char eeChargeMode EEPROM;

//typedef char tModeString[5];
//extern const tModeString modeName[MODE_NUM+2]	PROGMEM;
extern const s16 cell_stop[MODE_NUM];
extern const s16 cell_min[MODE_LI_NUM];


/* mus be called once at start */
void readSettings(eMode mode);

void saveSettings(eMode mode);

/* will print all LCD outputs and handle user inputs
 * must be called every MENU_DISPLAY_INT ms
 * */
void showMenu(void);

#endif
