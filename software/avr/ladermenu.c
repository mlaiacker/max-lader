
/* 
* max-Lader 
* entwickelt von:
* Maximilian Laiacker post@mlaiacker.de http://mlaiacker.de/
*/

#include <avr/eeprom.h>

#include "global.h"
#include "ladermenu.h"
#include "rtc.h"
#include "laden.h"
#include "lcd.h"
#include "laden.h"
#include "pwmregler.h"
#include "pwm.h"

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

typedef char tModeString[5];

const tModeString modeName[]	PROGMEM = // Lademodus
{
	"LiPo",
	"LiHv",
	"LiFe",
	"NiMh",
	"NiCa",
	"PB  ",
	"UI  ",
	"ENDE",
	"ERR "
};

const s16	param2Max[] =
{
	LIPO_CELL_MAX,
	LIHV_CELL_MAX,
	LIFE_CELL_MAX,
	9900, //mAh*10
	9900, //mAh*10
	PB_CELL_MAX,
	UOUT_MAX
};

const s16 cell_stop[]={
	LIPO_UCELL_STOP,
	LIHV_UCELL_STOP,
	LIFE_UCELL_STOP,
	0,
	0,
	PB_UCELL_STOP,
	0
};

const s16 cell_min[]={
	LIPO_UCELL_MIN,
	LIPO_UCELL_MIN,
	LIFE_UCELL_MIN,
};

typedef char tErrorString[12];
const tErrorString errorName[ERROR_NUM]	PROGMEM = // Fehlermeldungen
{    
	"    Uin    ", 
	"Temperatur ",
	" Ladezeit  ",
	"Kurzschluss",
	"  Zellen   ",
	"  Wandler  "
};
/*
typedef char tFertigString[10];
tFertigString fertigName[FERTIG_NUM]	PROGMEM = // Abschaltgrund
{
	"  Laden  ", 
	" Normal  ",
	"DeltaPeak",
	"  flach  ",
	"  Zeit   ",
	"  Max C  ",
	" Akku ab "
};
*/


// globale Variablen
tTaste	tasteL,tasteM,tasteR;
tSettings settings;
tMenu menu;
// eeprom zeugs
unsigned char eeChargeMode EEPROM;
tSettings eeSettings[MODE_NUM] EEPROM;
unsigned char eeBeepOn EEPROM;

void readTaste(tTaste *t, u08 input);

// setzt die flags im tasten struct, flanken und lang gedrückt
void readTaste(tTaste *t, u08 input)
{
	t->flags &= (1<<TASTE_BIT_ON);
	if (input)
	{
		if(!(t->flags & (1<<TASTE_BIT_ON)))  t->flags |= 1<<TASTE_BIT_FP;
		t->flags |= (1<<TASTE_BIT_ON);
	}
	else 
	{
		if((t->flags & (1<<TASTE_BIT_ON)) && ((t->flags & 1<<TASTE_BIT_LANG)==0))  t->flags |= 1<<TASTE_BIT_FN;
		t->flags &= ~(1<<TASTE_BIT_ON);
	}
	if(einschaltverz(input, TASTE_T_LANG, &t->t)) t->flags |= 1<<TASTE_BIT_LANG;
}
// ändert eine übergebene variable, bei einem tastendruck um "step"
// stellt sicher das sich der wert zwischen "min" bis "max" befindet
void editVar(s16 *var, s16 min, s16 max, s16 step)
{
	if(TASTE_FP(tasteL)||TASTE_LANG(tasteL))
	{
		*var -= step;
	} else
	if(TASTE_FP(tasteR)||TASTE_LANG(tasteR))
	{
		*var+=step;
	}

	if((*var)<min) *var = min;
	else
	if((*var)>max) *var = max;
}
// gibt übergebene sekunden in der form, "HH:MM" H=stunden M=minuten, aus
void showMinutes(s16 minutes)
{
 // 5 zeichen
	lcdNum(minutes/60, 2, 3);
	lcdDataWrite(':');
	lcdNum(minutes%60, 2, 3);
}

// gibt die 4 zeichen des lademodus aus
void printModeString(unsigned char m)
{
	lcdPrintProgString(modeName[m]);
}

// zeigt die wichtigsten parameter des gewälten ladeprogramms in einer zeile an
void showMode(void)
{
	printModeString(charger.mode);
	lcdNum(settings.iMax/10, 4,2);
	lcdPrint("A ");
	if(charger.mode==eModeUI)
	{
		lcdNum(settings.param2/10,3,1);
		lcdDataWrite('V');
	} else
	if(charger.mode<MODE_LI_NUM)
	{
		lcdNum(settings.param2,2,0);
		lcdPrint("Zell");
	} else
	if(charger.mode==eModeNiMh||charger.mode==eModeNiCa)
	{
		lcdPrintSpaces(5);
	} else
	if(charger.mode==eModePB)
	{
		lcdNum(settings.param2*(PB_UCELL/100),3,0);
		lcdDataWrite('V');
	}/*else
	if(charger.mode==eModeTime)
	{
		showMinutes(settings.param2);
	}*/
}

void showParam2(void)
{
	if(charger.mode==eModeUI)
	{
		lcdPrint("Spannung=");
		lcdNum(settings.param2,4,2);
		lcdPrint("V ");
	} else
	if(charger.mode<MODE_LI_NUM || charger.mode==eModePB)
	{
		lcdPrint("Zellen=");
		lcdNum(settings.param2,1,0);
#if defined (__AVR_ATmega168__)
		lcdPrintSpaces(2);
		lcdNum(settings.param2*cell_stop[charger.mode]/10,4,1);
		lcdDataWrite('V');
#else
		lcdPrintSpaces(6);
#endif
	} else
	if(charger.mode==eModeNiMh||charger.mode==eModeNiCa)
	{
		lcdPrint("Max C =");
		lcdNum(settings.param2*10,4,2);
		lcdPrint("Ah   ");
	}/*else
	if(charger.mode==eModeTime)
	{
		lcdPrint("Ladezeit=");
		showMinutes(settings.param2);
		lcdPrintSpaces(2);
	}*/
}

/* read parameters from eeprom for curent mode*/
void readSettings(eMode mode)
{
	eeprom_read_block(&settings, &eeSettings[mode], sizeof(tSettings));
	if(((u08)(settings.iMax+settings.param2)) != settings.checkSum)
	{
		settings.iMax = 0;
		settings.param2 = 0;
	}
}

/* save parameters and 'checksum' to eeprom for curent mode*/
void saveSettings(eMode mode)
{
	eeprom_write_byte(&eeChargeMode, (u08)mode);
	settings.checkSum = (u08)(settings.param2 + settings.iMax);
	eeprom_write_block(&settings, &eeSettings[mode], sizeof(tSettings));
}


void showProgramm(void)
{
	lcdPrint("Programm=");
	printModeString(charger.mode);
	lcdPrintSpaces(3);
}
/*
void showStrom(void)
{
	lcdPrint("Strom=");
	lcdNum(settings.iMax/10,3,2);
	lcdDataWrite('A');
	lcdPrintSpaces(5);
}
*/
void showMeldung(void)
{
	lcdPrintProgString(errorName[errorn-1]);
	lcdPrintSpaces(5);	
}
// lcd ausgaben wenn laden aktiv ist
void showLader(void)
{
		u08 m;
#if defined (__AVR_ATmega168__)
		s16 dif;
#endif
		if(TASTE_FP(tasteM)) // mit mitlerer Taste stromeinstellung aktivieren
		{
			menu.edit = !menu.edit;
		}
		if(menu.edit)
		{ 
			editVar(&settings.iMax, 0, IOUT_MAX, STEP_I); // maximalen Strom ändern
		} else
		{
			editVar(&menu.ladenNum, 0, LADEN_MAX_NUM, 1); // Anzeige der zweiten zeile auswählen
		}
		m = charger.mode;
		if(errorn)
		{
			m=eModeError; // wenn fehler dann "ERR" anzeigen
		} else if(charger.fertig && menu.blinken)
		{
			m=eModeEnde; // wenn fertig dann abwechselnd "ENDE" und progamm anzeigen
		}  
		// erste zeile: Programm Spannung Strom
		printModeString(m);	
		lcdNum(charger.uOut,4,2);
		lcdDataWrite('V');
		if(menu.edit)
		{
			lcdNum(settings.iMax/10,4,2);
		} else
		{
			if((charger.mode==eModeNiCa)||(charger.mode==eModeNiMh)/*||(charger.mode==eModeTime)*/) {
				lcdNum(charger.iMax/10,4,2);
			} else {
				lcdNum(charger.iOut/10,4,2);
			}
		}
		// "A" blinkt wenn strom über tasten geändert werden kann.
		if(menu.edit&&menu.blinken){
			lcdDataWrite(' ');
		} else{
			lcdDataWrite('A');
		}
		
		lcdGotoY(1);// untere zeile
		if(errorn && menu.blinken)
		{
			showMeldung(); // Fehlermeldungen anzeigen
		}else
		if(menu.ladenNum==eLadenTimeC) // Zeit und Kapazität
		{
			showMinutes(charger.sekunden/60);
			lcdPrintSpaces(3);
			lcdNum(charger.c,5,2);
			lcdPrint("Ah");
		} else
		if(menu.ladenNum==eLadenUinT) // eingansspannung und temperatur
		{
			lcdPrint("Uin=");
			lcdNum(regler.uIn,4,2);
			lcdPrint("V ");
			lcdNum(regler.temp,2,0);  
			lcdDataWrite(223);
			lcdPrint("C ");
		} else
#ifdef eLadenPhase
		if(menu.ladenNum==eLadenPhase) // lader zustand
		{
//			lcdPrint("P=");
//			lcdNum(charger.phase,1,0);
//			lcdDataWrite(' ');
			lcdPrintProgString(fertigName[charger.fertig]); // abschaltgrund anzeigen
			lcdPrintSpaces(7);
		} else
#endif
		if(menu.ladenNum==eLadenRaw) // echtzeit werte
		{
			lcdNum(BITS2UOUT(regler.uOut),4,2);
			lcdDataWrite('V');
			lcdNum(BITS2IOUT(regler.iOut),4,3);
			lcdDataWrite('A');
			lcdNum(regler.pwm/PWM_DIV,4,0);
	//		lcdDataWrite('%');
		} else
		if(menu.ladenNum==eLadenDt) // spannungsänderungen
		{
			lcdPrint("du=");
			lcdNum(charger.dut,2,0);
			lcdPrint(" ddu=");
			lcdNum(charger.ddutt,2,0);
			lcdPrintSpaces(4);
		}
#if defined (__AVR_ATmega168__)
		else if(menu.ladenNum==eLadenBalancer) // balancer
		{
			lcdNum(cell_avg,4,3);			
			lcdDataWrite('V');
			lcdDataWrite('1');
			for(m=0;m<3;m++)
			{
				dif = cell_avg-bdata.mv[m];
				if(m>=bdata.cells) dif=0;
				if(dif<0) 
				{
					lcdDataWrite('-');
					dif = -dif;
				} else lcdDataWrite('+');
				lcdNum(dif,2,0);			
			}
		}else
		if(menu.ladenNum==eLadenBalancer2) // balancer
		{
			lcdNum(cell_avg,4,3);			
			lcdDataWrite('V');
			lcdDataWrite('4');
			for(m=3;m<6;m++)
			{
				dif = cell_avg-bdata.mv[m];
				if(m>=bdata.cells) dif=0;
				if(dif<0) 
				{
					lcdDataWrite('-');
					dif = -dif;
				} else lcdDataWrite('+');
				lcdNum(dif,2,0);			
			}
		}
#endif		
}

void showMenu(void)
{
	#ifdef BEEP_ON
	if( (charger.beep%2 == 1) && charger.on) BEEP_ON; else BEEP_OFF;
	if(errorn)
	{
		if(charger.beep<BEEP_ERROR_COUNT*2) charger.beep++;
	}
	#endif
	readTaste(&tasteL,TASTE_L);
	readTaste(&tasteM,TASTE_M);
	readTaste(&tasteR,TASTE_R);
	//			if(TASTE_FP(tasteL)||TASTE_FP(tasteM)||TASTE_FP(tasteR)) BEEP_ON;
	lcdGotoY(0);
	if(TASTE_LANG(tasteM))	// laden starten/stoppen wenn mitlere taste lange gedrükt wird
	{
		tasteM.flags &= ~(1<<TASTE_BIT_LANG);
		tasteM.t = 0;
		errorn = 0;
		if(charger.on)
		{
			ladenOff();
		} else
		{
			ladenOn();
		}
	}
	if(menu.edit)
	{
		if(TASTE_ON(tasteL)||TASTE_ON(tasteR))  
		{
			menu.changed = 1;// es wurden änderungen am programm gemacht
			charger.sekStromaenderung=charger.sekunden;
		}
	}
	if(charger.on) // laden aktiv
	{
		showLader();
	} else
	{
		if(!menu.edit) editVar(&menu.topNum, 0, MENU_MAX_NUM, 1);

		if(TASTE_FN(tasteM) && (menu.topNum!=eMenuStart)) 
		{
			if(menu.edit)
			{
				if((menu.topNum == eMenuProg) && menu.changed) // wenn das ladeprogramm gewechselt wurde, parameter laden
				{
					readSettings(charger.mode);
					menu.changed = 0;
				}
			}
			menu.edit = !menu.edit;
		}
		
		if(menu.topNum==eMenuStart)
		{
			showMode(); // Startbild
		} else
		if(menu.topNum==eMenuI) // strom einstellen
		{
			if(menu.edit) editVar(&settings.iMax, 0, IOUT_MAX, STEP_I);
			lcdPrint("Strom=");
			lcdNum(settings.iMax/10,3,2);
			lcdDataWrite('A');
			lcdPrintSpaces(5);
		}
		else
		if(menu.topNum==eMenuParam2) // parameter 2 einstellen
		{
			if(menu.edit)
			{
				if(charger.mode == eModeUI) editVar(&settings.param2,STEP_U, param2Max[charger.mode],STEP_U);
				else editVar(&settings.param2,0, param2Max[charger.mode],1);
			}
			showParam2();
		} else
		if(menu.topNum==eMenuProg) // ladeprogramm einstellen
		{
			if(menu.edit) editVar(&charger.mode,0,MODE_NUM-1,1);
			showProgramm();
		}

		lcdGotoY(1); // zweite zeile
		if(errorn && menu.blinken) {
			showMeldung(); // fehlermeldungen
		} else if(menu.edit) {
			lcdPrint("  -    OK    +  ");
		} else {
			if(menu.topNum==eMenuStart)	{
				lcdNum(regler.uIn/10,3,1);
				lcdPrint("V ");
				lcdPrint("Start  -> ");
			} else {
				lcdPrint(" <-   Einst.");
			}
			if((menu.topNum==eMenuI)||(menu.topNum==eMenuParam2)) {
				lcdPrint(" -> ");
			} else {
				lcdPrintSpaces(4);
			}
		}
		
	}

}

