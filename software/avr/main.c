/*	 main.c
* max-Lader 
* entwickelt von:
* Maximilian Laiacker post@mlaiacker.de http://mlaiacker.de/
*
*/
#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/wdt.h> 
#include <avr/eeprom.h>

#include "global.h"
#include "usart.h"
#include "a2d.h"
#include "lcd.h"
#include "pwm.h"
#include "rtc.h"
#include "pwmregler.h"
#include "laden.h"
#include "ladermenu.h"

#include "migration8-168.h"

struct
{
	t_Time tSekunde,tDisplay;
} maindata;

void uartOutput(void);

//-------------Initalisierung------------------
// da die init fuktionen sowiso nur ein mal im programm aufgerufen werden stehen sie hier "inline"
// ist zwar nicht so übersichtlich spart aber mindestens 4 byte pro init funktion
void init(void){
	#ifdef BEEP_ON
	BEEP_INIT;
	#endif
	
	#ifdef UART
// Serielle Schnittstelle intalisieren
	#if defined (__AVR_ATmega168__)
		UBRRL = (uint8_t)(F_CPU/(UART_BAUD_RATE*8L)-1);
		UBRRH = (F_CPU/(UART_BAUD_RATE*8L)-1) >> 8;
		UCSR0A = _BV(U2X0);
		UCSR0B = _BV(TXEN)|_BV(RXEN)|_BV(RXCIE);
//		UCSRB = 0x98;        // 1001 1000
                       // Receiver enabled, Transmitter enabled
                       // RX Complete interrupt enabled
	#elif defined (__AVR_ATmega8__)
		UBRRL = (uint8_t)(F_CPU/(UART_BAUD_RATE*8L)-1);
		UBRRH = (F_CPU/(UART_BAUD_RATE*8L)-1) >> 8;
		UCSRA = _BV(U2X);
//		UBRRL = UART_UBRRL; // Bautrate setzten
		UCSRB = _BV(TXEN); // senden aktivieren, das Programm macht nur Ausgaben über die serielle Schnittstelle
	#endif

	#endif
	lcdInit();

	oscData.trigger_ch=254;
	oscData.index = 0;

// timer0 für Zeitmessung
	Time=0;
	// Counter Register
	TCNT0=RTC_START_VAL;
	// Overflow enable
	#ifdef TIMSK
	TIMSK |= _BV(TOIE0);
	#else
	TIMSK0 |= _BV(TOIE0);
	#endif
	#ifdef TCCR0
	// prescaler select F_CPU/256
	// ergibt einen overflow alle 16MHz/256/250 = 4 ms
	TCCR0 = 0x04;
	#else
	TCCR0B = 0x04;
	#endif
	
	
	ADMUX = (0<<ADLAR) | (ADC_REFERENCE<<6);
	ADCSR = ((0<<ADFR)|(1<<ADEN)|(1<<ADIE)|(1<<ADSC)|(1<<ADIF) | ADC_PRESCALE);

#ifdef PWM1
#ifdef PWM_DOWN_9BIT
  TCCR1A = (1<<COM1A1|0<<COM1A0|1<<COM1B1|1<<COM1B0|1<<WGM11|0<<WGM10); 
#else
  TCCR1A = (1<<COM1A1|0<<COM1A0|1<<COM1B1|1<<COM1B0|0<<WGM11|1<<WGM10); 
#endif
  TCCR1B = (0<<WGM13|1<<WGM12|1<<CS10|0<<CS11|0<<CS12);
#endif
#ifdef PWM12
#endif
    cbi(PWM_UP_PORT,PWM_UP_BIT); // port = 0
    sbi(PWM_UP_DDR,PWM_UP_BIT); // OC1B
  
	sei(); // interupt enable

	TASTE_L_INIT;
	TASTE_M_INIT;
	TASTE_R_INIT;
	ladenOff();
	BEEP_ON;
	lcdGotoY(0);
	#if defined (__AVR_ATmega168__)
		lcdPrint("   MAX-Lader16  ");
	#elif defined (__AVR_ATmega8__)
		lcdPrint("   MAX-Lader    ");
	#endif
	lcdGotoY(1);
	lcdPrint("Version:");
	lcdNum(LADER_VERSION,2,1);
	rtcDelay(1000);
	BEEP_OFF;
	rtcDelay(1000);

// zu letzt gewählten lade modus aus dem eeprom lesen
	charger.mode = eeprom_read_byte(&eeChargeMode);
	if(charger.mode>(MODE_NUM-1)) charger.mode = 0; // auf plausibilität prüfen
	readSettings(charger.mode); // die Einstellungen zum aktuellen Modus aus eeprom lesen
}// init end

/*---------------- Functions -------------------------*/


#ifdef UART

#if defined (__AVR_ATmega168__)
#include <stdlib.h>
#include <string.h>
int balancer_parse(unsigned char c, t_balancer_state *state)
{
  switch(state->state)
  {
    // sync
    case 0:if(c==0x0a){ state->state++; state->index=0; state->cell=0;} break;
    case 1:if(c==0xf5) state->state++; else state->state=0; break;
    case 2:if(c==0x0c) state->state++; else state->state=0; break;
    // unkown byte
    case 3:if(c!=0) { state->state++; state->data.byte4=c; } else state->state=0; break;
    // cell count char
    case 4:if(c!=0)
           { 
             state->state++; 
             state->data.cells = c-'0'; 
             if((state->data.cells == 0) || (state->data.cells > MAX_BALANCER_CELL)) state->state=0;
           }
           else state->state=0; 
           break;
    // reading of cell data
    case 5:if(c!=0)
           {
             state->cell_string[state->index]=c;
             state->cell_string[state->index+1]=0;
             state->index++;
             // the cell voltage is 4 bytes long 
             if(state->index>=4)
             {
               state->data.mv[state->cell] = strtol(state->cell_string, NULL, 16);
               state->cell++; 
               state->index=0;
               if(state->cell>=state->data.cells)
               {
                 state->state++;
               }
             }
           }
           else state->state=0; 
           break;
     // last 4 bytes
     case 6:if(c!=0)
           {
             state->cell_string[state->index]=c;
             state->cell_string[state->index+1]=0;
             state->index++;
             //  4 bytes long 
             if(state->index>=4) 
             {
               state->data.last_data = strtol(state->cell_string, NULL, 16);
               state->index=0;
               state->state++;
             }
           }
           else state->state=0; 
           break;
     case 7:state->state=0;
           if(c==0x0d) return -1;
           break;
     default: state->state=0; break;
  }
  return 0;
}
#endif

u16 uart_data_send=0;
u16 uart_busy=0;
void uartOsc(void)
{
	if(oscData.trigger_ch==255 && uart_data_send==0 && uart_busy==0)
	{
		usartPutc(0xff);
		usartPutc(0xaf);
		usartPutc(0xbf);
		uart_busy=1;
	}
	if(uart_busy && uart_data_send<sizeof(oscData))
	{
		usartPutc(((u08*)&oscData)[uart_data_send]);
		uart_data_send++;
	}
	if(uart_data_send==sizeof(oscData) && uart_busy==1)
	{
		uart_busy = 0;
		oscData.trigger_ch=254;
		uart_data_send = 0;
	}
}

void uartOutput(void)
{
	if(uart_busy) return;
	//usart_puts_prog(modeName[charger.mode]);
	usartNum(charger.sekunden,5,0); // Ladezeit
	usartNum(charger.u,4,2); // Ausgangsspanung aktuell
	usartNum(charger.i,4,3); // Ausgangsstrom
	usartNum(charger.c,4,2); // geladene Kapazität
//	usartNum(charger.dut,0,0); // Spanungsänderung in mv/Minute
//	usartNum(charger.ddutt,0,0); // Änderung der Spannungsänderung in mv/Minute²
	usartNum(regler.error,0,0);
	usartNum(regler.errorI,0,0);
//	usartNum(regler.pwm<<5,3,0); // pwm zustand
	usartNum(regler.uIn,4,2); // Eingangsspannung
//	usartNum(charger.uMax,4,2); //Maximalspannung
	usartNum(regler.temp,3,0); // "Temperatur" der Endstufe
	usartNum(charger.uOut,4,2); // Ausgangsspanung stromlos
#ifdef UOUT_LOW
//	usartNum(charger.uOutLow,4,2); //Ausgangsspanung unterer Bereich
#endif
	usartPutc('\r');
	usartPutc('\n');
}

#endif
/*----------------------------------------------------*/

/*----------------Timer Interrupts--------------------*/
ISR(TIMER0_OVF_vect)
{
  TCNT0=RTC_START_VAL;
  Time+=RTC_OVERFLOW_TIME;
#ifdef RTC_OVERFLOW_FRACTION
  Time_us+=RTC_OVERFLOW_FRACTION;
  if(Time_us>=1000)
  {
	Time+=1;
	Time_us-=1000;
  }
#endif
}
/*----------------------------------------------------*/

/*----------------Main Loop---------------------------*/
int main(void)
{
#if defined (__AVR_ATmega168__)
	t_balancer_state	bstate;
	int i;
#endif
	init();
	// watchdog einschalten
	wdt_enable(WDTO_1S);
	
	for(;;)
	{
		wdt_reset();
#if defined (__AVR_ATmega168__)
		if(usart_unread_data()!=0)
		{
			if(balancer_parse(usart_getc(),&bstate))
			{
				memcpy(&bdata, &(bstate.data), sizeof(bdata));
				if(bdata.cells!=0)
				{
					cell_avg = 0;
					for(i=0;i<bdata.cells;i++)
					{
						cell_avg += bdata.mv[i];
					}
					cell_avg /= bdata.cells;
				}
			}
		}
#endif
		if(maindata.tDisplay <= Time)
		{
			laden();
			if(regler.status == REGLER_STATUS_ON)
			{
				regler.cOut += charger.i;
			}
			readTaste(&tasteL,TASTE_L);
			readTaste(&tasteM,TASTE_M);
			readTaste(&tasteR,TASTE_R);
			#ifdef BEEP_ON
			if( (charger.beep%2 == 1) && charger.on) BEEP_ON; else BEEP_OFF;
			if(errorn)
			{
				if(charger.beep<BEEP_ERROR_COUNT*2) charger.beep++;
			}
//			if(TASTE_FP(tasteL)||TASTE_FP(tasteM)||TASTE_FP(tasteR)) BEEP_ON;
			#endif
			showMenu();
			maindata.tDisplay += MENU_DISPLAY_INT;
		}
		if(maindata.tSekunde <= Time) // jede sekunde
		{
			maindata.tSekunde += 1000L;
			menu.blinken = !menu.blinken;
			#ifdef UART
			if(regler.status==REGLER_STATUS_ON)	uartOutput();
			#endif
			ladenSekunde();
			pwmreglerSekunde();
		}
		uartOsc();
	}// for(ever) 
}// main

