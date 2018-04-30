/*
 * UsartLCD_Final.c
 *
 * Created: 6/7/2017 2:05:12 AM
 * Author : Eric
 */

#include <avr/io.h>
#include "io.c"
#include "timer.h"
#include "usart_ATmega1284.h"

int main(void)
{
	DDRA = 0xFF; PORTA = 0x00;
	DDRC = 0xFF; PORTC = 0x00;
	DDRD = 0x00;
	unsigned char temp = 0;
	char GO[] = "GAME OVER.....";
	char RE[] = "PRESS TO RESTART";
	//char Greet[] = "ENJOY THE       GAME";
    TimerSet(1);
	TimerOn();
	initUSART(0);
	LCD_init();
    while (1)
    {
		if(USART_HasReceived(0)){
			temp = USART_Receive(0);
		}
		USART_Flush(0);
		if(temp == 0X01){
			for(int i = 1; i < 15; i++){
				LCD_Cursor(i);
				LCD_WriteData(GO[i-1]);
			}
			for(int i = 1; i < 17; i++){
				LCD_Cursor(i + 16);
				LCD_WriteData(RE[i-1]);
			}
		}
		else if(temp == 0x00){
			LCD_ClearScreen();
		}
		while(!TimerFlag){}
		TimerFlag = 0;
	}
}
