/* 
* max-Lader 
* entwickelt von:
* Maximilian Laiacker post@mlaiacker.de http://mlaiacker.de/
*/
#ifndef LADEN_H
#define LADEN_H

#include "global.h"
#include "rtc.h"
#include "a2d.h"
#include "hardware.h"

#define CHARGER_MIN_C	(10)	//mAh Auflösung
#define FEHLER_SCHWELLE	(200)

#define UOUT_MAX		(2500) // mV/10
#define UOUT_MAX_CHARGE (2700) // mV/10
#define TIME_MAX		(60*60*8)	//8 stunden

#define IOUT_FERTIG		(6)	//mA
#define AKKU_DETECT_U	(400) //mV/10
#define I_MESSEN		(2)	//bits

#define LIPO_UCELL		(370)	//mV/10
#define LIPO_UCELL_STOP	(420)	//mV/10
#define LIPO_UCELL_MIN	(280)	//mV/10
#define LIPO_CELL_MAX	(6)		//zellen
#define LIPO_TIME_MAX	(3*60*60)	//sekunden
#define LIPO_TIME_BEGIN	(30) //sekunden
#define LIPO_I_END		(150) //mA


#define LIFE_UCELL		(330)	//mV/10 Nennspannung
#define LIFE_UCELL_STOP	(360)	//mV/10 Ladeschlussspannung
#define LIFE_UCELL_MIN	(200)	//mV/10 Minimale Spannung für Erkennung von falschen Zellenanzahlen
#define LIFE_CELL_MAX	(7)		//zellen
#define LIFE_TIME_MAX	(3*60*60)	//sekunden
#define LIFE_TIME_BEGIN	(30) //sekunden
#define LIFE_I_END		(150) //mA

#define NICA_TIME_BEGIN			(30) // sekunden
#define NICA_TIME_BEGIN2		(180) //sekunden bis zur abschaltung möglich,nach stromänderung
#define NICA_DELTA_TOP			(-10)
#define NICA_DELTA_PEAK			(-60)
#define NIMH_DELTA_PEAK			(-40)
#define NICA_WENDE_PUNKT		(100)
#define NICA_I_TICKLE	(BITS2IOUT(20)) //mA

#define PB_UCELL		(200)	//mV/10
#define PB_UCELL_STOP	(240)	//mV/10
#define PB_UCELL_TICKLE	(230)	//mV/10
#define PB_I_TICKLE		(150)	//mA
#define PB_I_FERTIG		(200)	//mA
#define PB_CELL_MAX		(6)		// 12V blei Akku


#define FERTIG_NORMAL	 	(1)
#define FERTIG_DELTA_PEAK	(2)
#define FERTIG_NO_CHANGE 	(3)
#define FERTIG_ZEIT 		(4)
#define FERTIG_MAX_C 		(5)
#define FERTIG_AKKU_AB 		(6)
#define FERTIG_NUM	7

typedef enum 
{
	eModeLiPo,
	eModeLiFe,
	eModeNiMh,
	eModeNiCa,
	eModePB,
	eModeUI,
	eModeEnde,
	eModeError
} eMode;
#define MODE_NUM	(eModeEnde) // Anzahl der Lademodi

#define CHARGER_BUF_LEN	(2)
typedef struct 
{
	u08 on,fertig,phase,ctmp,beep,bufIndex,uDetect,messen;
	u16	sekunden,sekStromaenderung;
	u08 vChange,vError,vDP,vAb; // verzögerungen
	s16 dut,ddutt,c,uOutTmp,uOut,iOut,i,uMax,pOut;
	u16 uSoll,iMax;
#ifdef UOUT_LOW
	s16 uOutLow;
#endif
	s16 mode;
	s16 u;
} tCharger;


#if defined (__AVR_ATmega168__)
#define MAX_BALANCER_CELL 14
typedef struct
{
  unsigned char byte4;
  unsigned char cells;
  s16 mv[MAX_BALANCER_CELL];
  s16 last_data;
}t_balancer_data;

typedef struct
{
  unsigned char state;
  unsigned char index;
  unsigned char cell;
  char cell_string[5];
  t_balancer_data data;
}t_balancer_state;
s16 cell_avg;

t_balancer_data bdata;
#endif

extern tCharger charger;

void laden(void);
void ladenSekunde(void);
void ladenMinute(void);
void ladenOn(void);
void ladenOff(void);

#endif
