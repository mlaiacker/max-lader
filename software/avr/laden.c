/* 
* max-Lader 
* entwickelt von:
* Maximilian Laiacker post@mlaiacker.de http://mlaiacker.de/
*/


#include <avr/io.h>
#include <string.h>
#include "global.h"
#include "laden.h"
#include "pwmregler.h"
#include "rtc.h"
#include "a2d.h"
#include "ladermenu.h"

tCharger charger;


void ladenSteuern(void)
{
	if((!charger.on) || (errorn!=0))
	{
		regler.status = REGLER_STATUS_OFF;
	} else
	if(!charger.fertig)
	{
		if(charger.sekunden>=TIME_MAX) errorn = ERROR_ZEIT;
		
		if(charger.mode==eModeUI)
		{
			charger.uSoll = (settings.param2);
			charger.iMax = (settings.iMax);
		} else
		if(charger.mode<MODE_LI_NUM) // Lipo Akkus Laden
		{
			if(charger.sekunden>=LI_TIME_MAX) charger.fertig = FERTIG_ZEIT;
			charger.uSoll = (settings.param2*cell_stop[charger.mode]);
			if(impulsverz((charger.u < settings.param2*cell_min[charger.mode]) || (charger.u > settings.param2*(cell_stop[charger.mode]+10)), 20, &charger.vError)) errorn = ERROR_ZELLEN;
			charger.iMax = settings.iMax;
			if( impulsverz( charger.i <= (settings.iMax>>3), 100, &charger.vChange) ) charger.fertig = FERTIG_NORMAL;
		} else
		if((charger.mode==eModeNiMh)||(charger.mode==eModeNiCa)) // NiCa und NiMh Akkus Laden
		{
			if((charger.sekunden%60==10))
			{
				charger.messen=1;
			}
			charger.uSoll = UOUT_MAX_CHARGE;
			if((charger.c >= settings.param2*10)&&(settings.param2>0)) charger.fertig = FERTIG_MAX_C;
		
			if(charger.sekunden<=NICA_TIME_BEGIN)
			{
				charger.iMax = settings.iMax*(charger.sekunden/8+1)/(NICA_TIME_BEGIN/8+1);
				charger.sekStromaenderung=charger.sekunden;
			} else
			{	
				charger.iMax = settings.iMax;
				if((charger.sekunden-charger.sekStromaenderung)>NICA_TIME_BEGIN2)
				{
					if(charger.mode==eModeNiCa)
					{
					   if( (charger.dut <= NICA_DELTA_PEAK) && (charger.ddutt >= charger.dut) ) charger.fertig = FERTIG_DELTA_PEAK;
//						if(impulsverz((charger.dut<0)&&(charger.ddutt<=0), 120, &charger.vChange)) charger.fertig = FERTIG_NO_CHANGE;
					} else
					{ // NiMh
						if( (charger.dut <= NIMH_DELTA_PEAK) && (charger.ddutt>=charger.dut) ) charger.fertig = FERTIG_DELTA_PEAK;
//						if((charger.dut >= NICA_WENDE_PUNKT) &&(charger.ddutt <= NICA_WENDE_PUNKT/2)) charger.phase = 1; // wendepunkt
//						if(charger.phase == 1)
//						{
//						if((charger.ddutt <= 0) && (charger.dut <= 0)) charger.fertig = FERTIG_NO_CHANGE;
//						}
						if(impulsverz((charger.dut<=0) && (charger.ddutt <= 0) , 254, &charger.vChange)) charger.fertig = FERTIG_NO_CHANGE;
					}
//					if((((charger.dut <= NICA_DELTA_PEAK)&&(charger.mode==eModeNiCa))||((charger.dut <= NIMH_DELTA_PEAK)&&(charger.mode==eModeNiMh))) && (charger.ddutt>=charger.dut)) charger.fertig = FERTIG_DELTA_PEAK;
					
					if( charger.u >= charger.uMax ) charger.uMax += (charger.u-charger.uMax)/4;
					if(impulsverz((charger.u-charger.uMax) <= -8,181,&charger.vDP)) charger.fertig = FERTIG_NORMAL;
				}
			}

		}else
		if(charger.mode==eModePB) // Bleiakkus
		{
			charger.uSoll = (settings.param2*PB_UCELL_STOP);
			charger.iMax = (settings.iMax);
//			if(impulsverz((charger.i<PB_I_FERTIG) && (charger.u>=(charger.uSoll-3)), 200, &charger.vChange))
			if(impulsverz((charger.i<PB_I_FERTIG), 200, &charger.vChange))
			{
				charger.fertig = FERTIG_NORMAL;
			}
		} else
/*		if(charger.mode==eModeTime)
		{
			if(charger.sekunden>=settings.param2*60) charger.fertig = FERTIG_ZEIT;
			charger.uSoll =  UOUT_MAX_CHARGE;
			charger.iMax = settings.iMax;
		} else*/
		{
			charger.on=0;
		}

	} else // wenn das laden fertig ist, erhaltungsladung machen
	if((regler.status!=REGLER_STATUS_OFF))
	{
		if((charger.mode == eModeNiCa) || (charger.mode == eModeNiMh))
		{
			charger.iMax = NICA_I_TICKLE;
		} else
		if(charger.mode == eModePB)
		{
			charger.iMax = PB_I_TICKLE;
			charger.uSoll = settings.param2*PB_UCELL_TICKLE;
		} else
		{
			regler.status = REGLER_STATUS_OFF;
		}
	}
	regler.uSoll = UOUT2BITS(charger.uSoll);
}

void ladenOff(void)
{
	ladenOn();
	charger.on = 0;
	regler.status = REGLER_STATUS_OFF;
}

void ladenOn(void)
{
	BEEP_ON;
	memset(&charger,0,sizeof(charger)-4);
	charger.on = 1;
	charger.uOutTmp=0;
	charger.uOut=charger.u;
	regler.cOut = 0;
	if(menu.changed)
	{
		menu.changed = 0;
		saveSettings(charger.mode);
	}
	menu.edit=0;
	regler.status = REGLER_STATUS_ON;
}

void ladenSekunde(void)
{
	//u16 tmp;
	// spannung ist unter 1 volt gefallen also kann einen neuer akku erkannt werden
	if((charger.u<100)) charger.uDetect = 1; 
	// laden automatisch starten
	if((!charger.on)&&(charger.u>=(AKKU_DETECT_U))&&(charger.uDetect)&&(charger.mode!=eModeUI)) ladenOn();
	if(charger.on)
	{
		// wenn akku abgezogen wurde bereit machen zum neustart
		if((charger.uDetect)&&(charger.fertig>0)&&(charger.beep==BEEP_LADEN_COUNT*2)) ladenOff();
		// spannungsabffal beim messen, könnte man zur stromanpassung benutzen!?
		// bei langen kabel ist der wert gross
//		charger.spannungsabfallRelativ = (charger.u-charger.uOut)*10/(charger.uOut/10); // in Prozent*10 der leerlaufspannung
		// ausgangsleistung errechnen, nicht besonders tolle auflösung wegen integer overflow
//		charger.pOut=((charger.u/100)*(charger.i/10));// leistung
		//ausgangsleistung verringern wenn zu heiss geworden
		if((/*(charger.pOut>P_MAX)||*/(TEMP_KRITISCH(regler.temp-5)))&&(settings.iMax>50))
		{
			settings.iMax-=10;
		}
		charger.iOut = (charger.iOut + charger.i*3)/4;
	}
	ladenSteuern();
	if(charger.fertig == 0)
	{
		// uout wird am display angezeigt bei den Nixx modi wird der stromlose wert angezeigt, sonst die ausgangsspannung
		if(charger.mode==eModeUI || charger.mode<MODE_LI_NUM) 	charger.uOut = charger.u;
		charger.sekunden++;
		// geladenen amperstunden zählen, es wird in zwei stufen gearbeitet...
		charger.c=BITS2COUT(regler.cOut);
	} else
	{// wenn laden fertig dann rumpiepen
		if((charger.beep<BEEP_LADEN_COUNT*2) && charger.on) charger.beep++;
	}
}

void laden(void)
{
	s16 tmp;

	charger.i = (BITS2IOUT(regler.iOut) + charger.i*3)/4;
	charger.u = BITS2UOUT(regler.uOut);
	#ifdef UOUT_LOW
	charger.uOutLow = BITS2UOUTL(a2dAvg(UOUT_LOW));
	#endif
	if(charger.on)
	{
		// wenn der strom auf 0 sinkt wurde der akku abgezogen und die ladung wird beendet
		if(impulsverz((charger.i <= IOUT_FERTIG) && (regler.iMax>0) && (charger.mode != eModeUI) && (!charger.messen) && (charger.sekunden>20),
				40, &charger.vAb))
		{
			if(charger.fertig==0) charger.fertig = FERTIG_AKKU_AB;
			regler.status=REGLER_STATUS_OFF;
		}
		// wenn gemessen werden soll, sollstrom auf 0 setzen
		if((charger.messen) && (charger.mode<=eModePB))
		{
			regler.iMax = 0;
		} else 	regler.iMax = IOUT2BITS(charger.iMax);

// stromlose messung der akku spannung
		if((charger.messen) && (regler.iOut<=I_MESSEN))
		{ 
			charger.uOut = charger.u;
			#ifdef UOUT_LOW
			if(charger.u<UOUT_LOW_AKTIV) charger.uOut=charger.uOutLow;
			#endif
			tmp = (charger.dut + (charger.uOut - charger.uOutTmp)*10)/2;
			if(charger.uOutTmp!=0)
			{
				charger.ddutt = tmp - charger.dut; // zweite ableitung u''(t) = u'(t) - u'(t-1)
				charger.dut = tmp; // erste ableitung der spannung u'(t) = u(t) - u(t-1)
			}
			charger.messen=0;
			charger.uOutTmp = charger.uOut;
		} 
	}
}
