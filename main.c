#ifndef F_CPU
#define F_CPU 16000000UL
#endif

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdio.h> 

#define LCD_DATA_PORT PORTC					//data port
#define LCD_CTRL_PORT PORTD					//control port
#define LCD_DATA_DDR DDRC					//data direction register for data port
#define LCD_CTRL_DDR DDRD					//data direction register for control port

#define RS PD5
#define RW PD6
#define E PD7
#define BUTTON_01 PD2
#define BUTTON_02 PD3
#define BUTTON_03 PB2

volatile uint16_t second_t = 0;
volatile uint16_t minute_t = 0;
volatile uint16_t hour_t = 0;
volatile uint16_t day_t = 1;
volatile uint16_t month_t = 1;
volatile uint16_t year_t = 2024;
volatile uint8_t toggle_flag = 0;
static int field = 0;


void lcd_command(unsigned char cmnd){
	LCD_DATA_PORT = cmnd;
	LCD_CTRL_PORT &= ~(1 << RS);			//RS = 0 for command mode
	LCD_CTRL_PORT &= ~(1 << RW);			// RW = 0 for write mode
	LCD_CTRL_PORT |= (1 << E);				//generating enable pulse
	_delay_us(1);
	LCD_CTRL_PORT &= ~( 1<< E);
	_delay_us(200);
}

void lcd_data(unsigned char data){
	LCD_DATA_PORT = data;
	LCD_CTRL_PORT |= (1 << RS);				//RS = 1 for write mode
	LCD_CTRL_PORT &= ~(1 << RW);			// RW = 0 for write mode
	LCD_CTRL_PORT |= (1 << E);				//generating enable pulse
	_delay_us(1);
	LCD_CTRL_PORT &= ~(1 << E);
	_delay_us(200);
	}
	
void set_cursor(unsigned char row, unsigned char column){
	unsigned char position;
	if(row==1){
		position = 0x80 + column -1;
	}else if(row==2){
		position = 0xC0 + column -1;
	}
	lcd_command(position);
}

void display_string(const char *str){
	while (*str){
		lcd_data(*str++);
	}
}

ISR(INT0_vect){
	if(toggle_flag){
		if(field==0){
			second_t = (second_t+1) % 60;
		}else if(field==1){
			minute_t= (minute_t+1) % 60;
		}else if(field == 2){
			hour_t= (hour_t+1) % 24;
		}else if(field == 3){
			if(month_t == 2){
				if ((year_t % 4 == 0 && year_t % 100 != 0) || (year_t % 400 == 0)){
					day_t = (day_t%29) +1;
				}else{day_t = (day_t%28) +1;}
			}else if(month_t == 1 || month_t == 3 || month_t == 5 || month_t == 7 || month_t == 8 || month_t == 10 || month_t == 12){
					day_t = (day_t%31) + 1;
			}else{
				day_t = (day_t%30) + 1;
			}
		}else if(field == 4){
			month_t= (month_t%12) + 1;
		}else if(field == 5){
			year_t= (year_t+1)%2030;
		}
			
	}
}
ISR(INT1_vect){
	if(toggle_flag){
		field = (field + 1) % 6;}
}
ISR(INT2_vect){
	toggle_flag=!toggle_flag;
}
	
void lcd_init(void){
	LCD_DATA_DDR = 0xFF;					//setting data ports as output
	LCD_CTRL_DDR = 0xFF;					//setting control pins as output
	_delay_ms(1);
	
	lcd_command(0x38);						//initialization of 8-bit mode
	_delay_ms(1);						
	lcd_command(0x0C);						//display on, cursor off
	_delay_ms(1);
	lcd_command(0X01);						//clear display
	_delay_ms(1);
	lcd_command(0X06);						//increment cursor
	_delay_ms(1);
}
//void time_date_calculation(){}			//incomplete,further job is being done.....
void setup(){
	DDRD &= ~(1 << BUTTON_01);
	DDRD &= ~(1 << BUTTON_02);				// set PD2, PD3, PB2 button as input
	DDRB &= ~(1 << BUTTON_03);
	
	PORTD |= (1 << BUTTON_01);
	PORTD |= (1 << BUTTON_02);
	PORTB |= (1 << BUTTON_03);				//internal pull-up resistors high
	
	//enable external interrupt INT0
	MCUCR |= (1 << ISC01);
	MCUCR &= ~(1 << ISC00);					//ISC01 = 1, ISC00 = 0 for falling edge trigger for INT0
	GICR |= (1 << INT0);					//INT0 interrupt enabled
	
	//enable external interrupt INT1
	MCUCR |= (1 << ISC11);
	MCUCR &= ~(1 << ISC10);					//ISC11 = 1, ISC10 = 0 for falling edge trigger for INT1
	GICR |= (1 << INT1);
	
	//enable external interrupt INT2
	MCUCSR &= ~(1 << ISC2);
	GICR |= (1 << INT2);
	
	sei();									//enable global interrupt
}
int main(){
	setup();
	lcd_init();
	_delay_ms(5);
	char buffer1[17];
	char buffer2[17];
	
	while(1){
		set_cursor(1,1);
		snprintf(buffer1, 17, "%02u : %02u : %02u", second_t, minute_t, hour_t);
		display_string(buffer1);
		set_cursor(2,1);
		snprintf(buffer2, 17, "%02u : %02u : %04u", day_t, month_t, year_t);
		display_string(buffer2);
		_delay_ms(10);
	}

	return 0;
}