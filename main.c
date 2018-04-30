/*
 * testled.c
 *
 * Created: 6/1/2017 5:16:46 PM
 * Author : Eric
 */

#include <avr/io.h>
#include "usart_ATmega1284.h"
#define F_CPU 1000000
#include <util/delay.h>
// Permission to copy is granted provided that this header remains intact.
// This software is provided with no warranties.

////////////////////////////////////////////////////////////////////////////////


#include <avr/interrupt.h>

volatile unsigned char TimerFlag = 0; // TimerISR() sets this to 1. C programmer should clear to 0.

// Internal variables for mapping AVR's ISR to our cleaner TimerISR model.
unsigned long _avr_timer_M = 1; // Start count from here, down to 0. Default 1ms
unsigned long _avr_timer_cntcurr = 0; // Current internal count of 1ms ticks

// Set TimerISR() to tick every M ms
void TimerSet(unsigned long M) {
	_avr_timer_M = M;
	_avr_timer_cntcurr = _avr_timer_M;
}

void TimerOn() {
	// AVR timer/counter controller register TCCR1
	TCCR1B 	= 0x0B;	// bit3 = 1: CTC mode (clear timer on compare)
	// bit2bit1bit0=011: prescaler /64
	// 00001011: 0x0B
	// SO, 8 MHz clock or 8,000,000 /64 = 125,000 ticks/s
	// Thus, TCNT1 register will count at 125,000 ticks/s

	// AVR output compare register OCR1A.
	OCR1A 	= 125;	// Timer interrupt will be generated when TCNT1==OCR1A
	// We want a 1 ms tick. 0.001 s * 125,000 ticks/s = 125
	// So when TCNT1 register equals 125,
	// 1 ms has passed. Thus, we compare to 125.
	// AVR timer interrupt mask register

	TIMSK1 	= 0x02; // bit1: OCIE1A -- enables compare match interrupt

	//Initialize avr counter
	TCNT1 = 0;

	// TimerISR will be called every _avr_timer_cntcurr milliseconds
	_avr_timer_cntcurr = _avr_timer_M;

	//Enable global interrupts
	SREG |= 0x80;	// 0x80: 1000000
}

void TimerOff() {
	TCCR1B 	= 0x00; // bit3bit2bit1bit0=0000: timer off
}

void TimerISR() {
	TimerFlag = 1;
}

// In our approach, the C programmer does not touch this ISR, but rather TimerISR()
ISR(TIMER1_COMPA_vect)
{
	// CPU automatically calls when TCNT0 == OCR0 (every 1 ms per TimerOn settings)
	_avr_timer_cntcurr--; 			// Count down to 0 rather than up to TOP
	if (_avr_timer_cntcurr == 0) { 	// results in a more efficient compare
		TimerISR(); 				// Call the ISR that the user uses
		_avr_timer_cntcurr = _avr_timer_M;
	}
}



void delay_ms(int miliSec) //for 8 Mhz crystal

{
	int i,j;
	for(i=0;i<miliSec;i++)
	for(j=0;j<775;j++)
	{
		asm("nop");
	}
}

//**********SHIFT REGISTER FUNCTION**********
void transmit_data(unsigned char data) {
	int i;
	for (i = 0; i < 8 ; ++i) {
		// Sets SRCLR to 1 allowing data to be set
		// Also clears SRCLK in preparation of sending data
		PORTB = 0x08;
		// set SER = next bit of data to be sent.
		PORTB |= ((data >> i) & 0x01);
		// set SRCLK = 1. Rising edge shifts next bit of data into the shift register
		PORTB |= 0x02;
	}
	// set RCLK = 1. Rising edge copies data from “Shift” register to “Storage” register
	PORTB |= 0x04;
	// clears all lines in preparation of a new transmission
	PORTB = 0x00;
}
//**********END SHIFT REGISTER CODE**********

char Col_1_C[4] = {0};
char Col_1_A[4] = {0};
char Col_2_C[4] = {0};
char Col_2_A[4] = {0};
char Col_3_C[4] = {0};
char Col_3_A[4] = {0};
char Col_4_C[4] = {0};
char Col_4_A[4] = {0};

unsigned char count = 0;
unsigned char position = 0x00;
unsigned char score = 0;
unsigned char done = 0;

unsigned char pressed1 = 0;
unsigned char pressed2 = 0;
unsigned char pressed3 = 0;
unsigned char pressed4 = 0;

unsigned long Column1 = 0; // left most column
unsigned long Column2 = 0;
unsigned long Column3 = 0;
unsigned long Column4 = 0;

enum {InitCol, CheckPress, Increment, DispScore, End} GM_State;
void GM_Tick(){
	switch(GM_State){
		case InitCol:
			Column1 = 0x258E0300;
			Column2 = 0xB9645E00;
			Column3 = 0xF9ACA400;
			Column4 = 0x8935A800;
			USART_Send(0x00, 0);
			GM_State = CheckPress;
			break;
		case CheckPress:
			if((Column1 & 0x01) != 0){
				if((~PIND & 0x08) != 0){
					if(!pressed1){
						score++;
						pressed1 = 1;
					}
				}
			}
			if((Column2 & 0x01) != 0){
				if((~PIND & 0x10) != 0){
					if(!pressed2){
						score++;
						pressed2 = 1;
					}
				}
			}
			if((Column3 & 0x01) != 0){
				if((~PIND & 0x20) != 0){
					if(!pressed3){
						score++;
						pressed3 = 1;
					}
				}
			}
			if((Column4 & 0x01) != 0){
				if((~PIND & 0x40) != 0){
					if(!pressed4){
						score++;
						pressed4 = 1;
					}
				}
			}
			GM_State = Increment;
			break;
		case Increment:
			if(count == 25 && position <= 31){
				Column1 = Column1 >> 1;
				Column2 = Column2 >> 1;
				Column3 = Column3 >> 1;
				Column4 = Column4 >> 1;
				position++;
				count = 0;
				pressed1 = 0; pressed2 = 0; pressed3 = 0; pressed4 = 0;
				GM_State = Increment;
			}
			else if(position > 31){
				GM_State = DispScore;
			}
 			else{
 				count++;
				GM_State = CheckPress;
 			}
			break;
		case DispScore:
			transmit_data(score);
			GM_State = End;
			break;
		case End:
			if(~PIND & 0x80){
				USART_Flush(0);
				USART_Send(0x00, 0);
				score = 0;
				transmit_data(score);
				position = 0;
				GM_State = InitCol;
			}
			else{
				if(USART_IsSendReady(0)){
					USART_Send(0x01, 0);
				}
				USART_Flush(0);
				GM_State = End;
			}
			break;
	};
}

enum {C1, C2, C3, C4} Disp_State;
void Disp_Tick(){
	switch(Disp_State){
		case C1:
			if((Column1 & 0x0001) != 0){
				Col_1_A[3] = 0xE0;
			}
			else{
				Col_1_A[3] = 0xF0;
			}

			if((Column1 & 0x0002) != 0){
				Col_1_A[2] = 0xD0;
			}
			else{
				Col_1_A[2] = 0xF0;
			}

			if((Column1 & 0x0004) != 0){
				Col_1_A[1] = 0xB0;
			}
			else{
				Col_1_A[1] = 0xF0;
			}

			if((Column1 & 0x0008) != 0){
				Col_1_A[0] = 0x70;
			}
			else{
				Col_1_A[0] = 0xF0;
			}

			//C portion of column1
			if((Column1 & 0x0010) != 0){
				Col_1_C[3] = 0xE2;
			}
			else{
				Col_1_C[3] = 0xF2;
			}
			if((Column1 & 0x0020) != 0){
				Col_1_C[2] = 0xD2;
			}
			else{
				Col_1_C[2] = 0xF2;
			}
			if((Column1 & 0x0040) != 0){
				Col_1_C[1] = 0xB2;
			}
			else{
				Col_1_C[1] = 0xF2;
			}
			if((Column1 & 0x0080) != 0){
				Col_1_C[0] = 0x72;
			}
			else{
				Col_1_C[0] = 0xF2;
			}
			PORTA = 0xF0;
			for(unsigned i = 0; i < 4; i++){
				PORTC = Col_1_C[i];
				while(!TimerFlag){}
				TimerFlag = 0;
			}
			PORTC = 0xF2;
			for(unsigned i = 0; i < 4; i++){
				PORTA = Col_1_A[i];
				while(!TimerFlag){}
				TimerFlag = 0;
			}
			Disp_State = C2;
			break;

		case C2:
			if((Column2 & 0x0001) != 0){
				Col_2_A[3] = 0xE0;
			}
			else{
				Col_2_A[3] = 0xF0;
			}
			if((Column2 & 0x0002) != 0){
				Col_2_A[2] = 0xD0;
			}
			else{
				Col_2_A[2] = 0xF0;
			}
			if((Column2 & 0x0004) != 0){
				Col_2_A[1] = 0xB0;
			}
			else{
				Col_2_A[1] = 0xF0;
			}
			if((Column2 & 0x0008) != 0){
				Col_2_A[0] = 0x70;
			}
			else{
				Col_2_A[0] = 0xF0;
			}

			if((Column2 & 0x0010) != 0){
				Col_2_C[3] = 0xE1;
			}
			else{
				Col_2_C[3] = 0xF1;
			}
			if((Column2 & 0x0020) != 0){
				Col_2_C[2] = 0xD1;
			}
			else{
				Col_2_C[2] = 0xF1;
			}
			if((Column2 & 0x0040) != 0){
				Col_2_C[1] = 0xB1;
			}
			else{
				Col_2_C[1] = 0xF1;
			}
			if((Column2 & 0x0080) != 0){
				Col_2_C[0] = 0x71;
			}
			else{
				Col_2_C[0] = 0xF1;
			}

			PORTA = 0xF0;
			for(unsigned i = 0; i < 4; i++){
				PORTC = Col_2_C[i];
				while(!TimerFlag){}
				TimerFlag = 0;
			}
			PORTC = 0xF1;
			for(unsigned i = 0; i < 4; i++){
				PORTA = Col_2_A[i];
				while(!TimerFlag){}
				TimerFlag = 0;
			}
			Disp_State = C3;
			break;

		case C3:
			if((Column3 & 0x0001) != 0){
				Col_3_A[3] = 0xE8;
			}
			else{
				Col_3_A[3] = 0xF8;
			}
			if((Column3 & 0x0002) != 0){
				Col_3_A[2] = 0xD8;
			}
			else{
				Col_3_A[2] = 0xF8;
			}
			if((Column3 & 0x0004) != 0){
				Col_3_A[1] = 0xB8;
			}
			else{
				Col_3_A[1] = 0xF8;
			}
			if((Column3 & 0x0008) != 0){
				Col_3_A[0] = 0x78;
			}
			else{
				Col_3_A[0] = 0xF8;
			}

			if((Column3 & 0x0010) != 0){
				Col_3_C[3] = 0xE0;
			}
			else{
				Col_3_C[3] = 0xF0;
			}
			if((Column3 & 0x0020) != 0){
				Col_3_C[2] = 0xD0;
			}
			else{
				Col_3_C[2] = 0xF0;
			}
			if((Column3 & 0x0040) != 0){
				Col_3_C[1] = 0xB0;
			}
			else{
				Col_3_C[1] = 0xF0;
			}
			if((Column3 & 0x0080) != 0){
				Col_3_C[0] = 0x70;
			}
			else{
				Col_3_C[0] = 0xF0;
			}
			PORTA = 0xF8;
			for(unsigned i = 0; i < 4; i++){
				PORTC = Col_3_C[i];
				while(!TimerFlag){}
				TimerFlag = 0;
			}
			PORTC = 0xF0;
			for(unsigned j = 0; j < 4; j++){
				PORTA = Col_3_A[j];
				while(!TimerFlag){}
				TimerFlag = 0;
			}
			Disp_State = C4;
			break;

		case C4:
			if((Column4 & 0x0001) != 0){
				Col_4_A[3] = 0xE4;
			}
			else{
				Col_4_A[3] = 0xF4;
			}
			if((Column4 & 0x0002) != 0){
				Col_4_A[2] = 0xD4;
			}
			else{
				Col_4_A[2] = 0xF4;
			}
			if((Column4 & 0x0004) != 0){
				Col_4_A[1] = 0xB4;
			}
			else{
				Col_4_A[1] = 0xF4;
			}
			if((Column4 & 0x0008) != 0){
				Col_4_A[0] = 0x74;
			}
			else{
				Col_4_A[0] = 0xF4;
			}

			if((Column4 & 0x0010) != 0){
				Col_4_C[3] = 0xE0;
			}
			else{
				Col_4_C[3] = 0xF0;
			}
			if((Column4 & 0x0020) != 0){
				Col_4_C[2] = 0xD0;
			}
			else{
				Col_4_C[2] = 0xF0;
			}
			if((Column4 & 0x0040) != 0){
				Col_4_C[1] = 0xB0;
			}
			else{
				Col_4_C[1] = 0xF0;
			}
			if((Column4 & 0x0080) != 0){
				Col_4_C[0] = 0x70;
			}
			else{
				Col_4_C[0] = 0xF0;
			}
			PORTA = 0xF4;
			for(unsigned i = 0; i < 4; i++){
				PORTC = Col_4_C[i];
				while(!TimerFlag){}
				TimerFlag = 0;
			}
			PORTC = 0xF0;
			for(unsigned j = 0; j < 4; j++){
				PORTA = Col_4_A[j];
				while(!TimerFlag){}
					TimerFlag = 0;
			}
			Disp_State = C1;
			break;
	};
}

int main(void)
{
	DDRA = 0xFF;
	DDRB = 0xFF;
	DDRC = 0xFF;
	DDRD = 0x07; PORTD = 0xF8;
	TimerSet(1);
	TimerOn();
	initUSART(0);
    while (1)
    {
		GM_Tick();
		Disp_Tick();
		while(!TimerFlag){}
		TimerFlag = 0;
    }
}
