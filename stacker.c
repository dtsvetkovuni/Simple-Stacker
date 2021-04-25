/*  Made by Dimitar Tsvetkov, ddt1u19@soton.ac.uk
 *  University of Southampton, 2021
 *
 *  Libraries for LCD ,LED and rotary used are from Steve Gunn.
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdio.h>
#include "lcd.h"
#include "rotary.h"
#include "led.h"


volatile rectangle start_square,square,last_square,down_square,empty_square; // all used squares
volatile uint8_t loop;      // initial loop determins when game ends
volatile uint8_t size=30;   // default size of 30
volatile uint8_t flag;      // flag for determinig if the middle button is pressed
volatile uint8_t level;     // level of stack tower
volatile uint16_t delay;    // delay for each move
volatile int8_t inc;        // variable for moving square on steps
volatile uint8_t max_level; // later used to calculate maximum number of levels for the square size and screensize


ISR(INT6_vect)
{
	if(loop==0)return;      // check to instantly exit interupt on game end condition

	if(flag){               // if button has been pressed it colors the (last_square/down_square) GREEN to save it on screen
		flag=0;
		fill_rectangle(last_square, GREEN);
		if(level>max_level){loop=0; return;}
		char lvl[3];
		sprintf(lvl, "%d", level);
		display_string_xy(lvl, 50, 10);  // updates the current level on top right of screen
	}
	else{
		if(level>max_level){loop=0; return;}
		fill_rectangle(last_square, display.background); //removes previous red square if button has not been pressed
	}

	last_square = square;
	fill_rectangle(square, RED); // colors the new square move
}

ISR(TIMER1_COMPA_vect)
{	
	_delay_ms(delay); // delay between each side move
	if(loop==0)return;

	if(!(PINE & _BV(SWC))){  // activates upon SWC press(middle button)
		if(down_square.left!=square.left || down_square.right!=square.right || level>max_level) { // checks if square press has been missed
			loop--; // to account for first block not having down square
			if(!loop){   //
				led_on();        // led lights up for duration of the delay
				_delay_ms(1250); // delay after final press to freeze the square on screen 
				led_off();
				return; // leaves the interrupt
			}
		}
		level++;
		if(delay>100) delay-=30; // minimum delay is 100ms(pretty fast)
		down_square = square;

		square.top -= size; // moves square up
    	square.bottom -= size;

		led_on();
		_delay_ms(1250); // delay after each press to freeze the square on screen 
		led_off();
		flag=1;          // sets flag for button press, so it can be detected by INT6_vect interrupt
	}

	square.left += inc; // moves square sideways
	square.right += inc;
	if (square.right >= display.width-(size) || square.left <= (size)) inc = -inc; // changes moving sides if reached corner
}

void initStartSquare(){ //Initializes start square size positions
	start_square.left= (LCDWIDTH-size)/2; //1
	start_square.right= (LCDWIDTH+size)/2; //2
	start_square.top= LCDHEIGHT-size-1;//3
	start_square.bottom= LCDHEIGHT-1;//4
}

int main()
{
	/* Clear DIV8 to get 8MHz clock */
	CLKPR = (1 << CLKPCE);
	CLKPR = 0;
	init_rotary();
	init_led();
	init_lcd();
	set_frame_rate_hz(61); /* > 60 Hz  (KPZ 30.01.2015) */
	/* Enable tearing interrupt to get flicker free display */
	EIMSK |= _BV(INT6);
	/* Enable game timer interrupt (Timer 1 CTC Mode 4) */
	TCCR1A = 0;
	TCCR1B = _BV(WGM12);
	TCCR1B |= _BV(CS10);
	TIMSK1 |= _BV(OCIE1A);
	OCR3A = 31250;

	/* Play the game */
	do {
		OCR1A = 65535; //Not sure what this does
		char sizetxt[3]; // used for converting ints to string

		// Main Menu of Simple Stacker
		clear_screen();
		PORTB |= _BV(PB6);
		display_string_xy("Simple STACKER", 75, 60);
		display_string_xy("Press UP to Start", 70, 100);
		display_string_xy("Press CENTER to place block", 40, 120);
		display_string_xy("Block Size",  85, 170);
		sprintf(sizetxt, "%d", size);
		display_string_xy(sizetxt, 110, 155);
		initStartSquare(); 
		fill_rectangle(start_square, RED); 

		while(PINC & _BV(SWE)) //Enters in loop untile UP is pressed
		{
			if (!(PINC & _BV(SWN))){ //left button press
				fill_rectangle(start_square, display.background);
				if(size>10){size-=10;display_string_xy("<", 70, 155);}
				initStartSquare();
				sprintf(sizetxt, "%d", size);
				display_string_xy(sizetxt, 110, 155);
				fill_rectangle(start_square, RED);
				_delay_ms(200);
				display_string_xy(" ", 70, 155);
			}
			if (!(PINC & _BV(SWS))){ //right button press
				fill_rectangle(start_square, display.background);
				if(size<70){size+=10;display_string_xy(">", 155, 155);}
				initStartSquare();
				sprintf(sizetxt, "%d", size);
				display_string_xy(sizetxt, 110, 155);
				fill_rectangle(start_square, RED);
				_delay_ms(200);
				display_string_xy(" ", 155, 155);
			}
		}


		// Variable initialisation 
		initStartSquare();
		max_level = (LCDHEIGHT-1)/size;
		inc=size;
		down_square=empty_square;
		last_square = square = start_square;
		flag = 1;
		loop = 2;
		level = 1;
		delay = 400;      // You can change this to start with different delay(increase for slower movement, decrease for faster)
		clear_screen();
		display_string_xy("Level:", 10, 10);
		display_string_xy("1", 50, 10);
		display_string_xy("/", 60, 10);
		sprintf(sizetxt, "%d", max_level);
		display_string_xy(sizetxt, 70, 10);

		sei();
		while(loop);
		cli();

		led_off(); // just in case to unstuck led
		if(level>max_level) display_string_xy("YOU WIN!", 95, 110); //checks if you have won or lost on end
		else display_string_xy("YOU LOSE!", 95, 110);
		_delay_ms(2500); // Delay before returning to main menu
		clear_screen();
	} while(1);
}
