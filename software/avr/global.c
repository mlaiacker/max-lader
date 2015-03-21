
#include "global.h"
//globale sachen

unsigned char errorn; // fehler nummer !=0 bedeutet fehler, angezeigt im menu


s16 sign(s16 x)
{
	if(x>0) return 1;
	if(x<0) return -1;
	return 0;
}
/*
s16 max(s16 a,s16 b)
{
	if(a>b) return a;
	else return b;
}
*/
// wird für einen Aufruf wahr wenn "bedingung" für "verz" aufrufe wahr war.
unsigned char impulsverz(unsigned char bedingung,unsigned char verz,unsigned char *timevar)
{
	if(!bedingung)
	{
		*timevar=0;
	}
	if(*timevar>verz)
	{
		*timevar = 0;
		return 1;
	} 
	*timevar+=1;

	return 0;
}

// wird wahr wenn "bedingung" für "verz" aufrufe wahr war.
unsigned char einschaltverz(unsigned char bedingung,unsigned char verz, short *timevar)
{
	if(!bedingung)
	{
		*timevar=0;
	}
	
	if(*timevar>verz)
	{
		return 1;
	} 
	*timevar+=1;

	return 0;
}
