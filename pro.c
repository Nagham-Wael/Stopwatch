/*
 * pro.c
 *
 *  Created on: Oct 19, 2020
 *      Author: Nagham
 */

#include<avr/io.h>
#include<util/delay.h>
#include<avr/interrupt.h>

unsigned char begin_flag=0, pause_flag=0,reset_flag=0;

void INT0_init(void)
{
	SREG&= ~(1<<7); /*Initially close other interrupts(Disable I-bit)*/
	DDRD&= ~(1<<2); /*Configure INT0(PD2) as input pin*/
	PORTD|=(1<<2); /*Configure it(PD2) as internal pull up*/
	MCUCR= (1<<ISC01); /*The falling edge of INT0 generates an interrupt request*/
	GICR|=(1<<INT0); /*Enable external interrupt INT0*/
	SREG|=(1<<7); /* Enable again I-bit*/
}

ISR(INT0_vect)
{
	reset_flag=1;
}
void INT1_init(void)
{
	SREG&= ~(1<<7); /*Initially close other interrupts(Disable I-bit)*/
	DDRD&=~(1<<3); /*Configure INT1(PD3) as input pin*/
	MCUCR |= (1<<ISC11) | (1<<ISC10); /*The rising edge of INT1 generates an interrupt request*/
	GICR|= (1<<INT1); /*Enable external interrupt INT1*/
	SREG|=(1<<7); /* Enable again I-bit*/
}

ISR(INT1_vect)
{
	pause_flag=1; /*Pause the stop watch*/
}
void INT2_init(void)
{
	SREG&= ~(1<<7); /*Initially close other interrupts(Disable I-bit)*/
	DDRB&= ~(1<<2); /*Configure INT2(PB2) as input pin*/
	PORTB|=(1<<2); /*Configure it(PB2) as internal pull up*/
	GICR|=(1<<INT2); /*Enable external interrupt INT2*/
	MCUCSR&= ~(1<<ISC2); /*The falling edge of INT2 generates an interrupt request*/
	SREG|=(1<<7); /* Enable again I-bit*/
}

ISR(INT2_vect)
{
	pause_flag=0; /*Resume the stop watch*/
}

void Timer1_CTC() /*Timer1 compare mode initialization*/
{
	SREG&= ~(1<<7);  /*Initially close other interrupts(Disable I-bit)*/
	TCCR1A= (1<<FOC1A)| (1<<FOC1B) ;  /*Non PWM */
	TCCR1B= (1<<WGM12) | (1<<CS11) | (1<<CS10); /*Mode 4(CTC): WGM12=1
												 *prescalar=64: CS11=1 & CS10=1
												 */
	TCNT1 = 0;  /*Initial value=0*/
	OCR1A = 15625; /*Value that timer compare with as Fcpu=1M & prescalar=64*/
	TIMSK=(1<<OCIE1A); /* Enable compare interrupt*/
	SREG|=(1<<7); /* Enable again I-bit*/
}

ISR(TIMER1_COMPA_vect)
{
	begin_flag=1; /*It increases the stop watch every 1 second*/
}

int main(void)
{
	Timer1_CTC();
	INT0_init();
	INT1_init();
	INT2_init();

	DDRC|=0x0F; /*The first 4 pins in PORTC are outputs*/
	PORTC&= 0xF0; /*The initial value of the first 4 pins in PORTC is 0*/
	DDRA|=0x3F; /*The first 6 pins in PORTA are outputs*/
	PORTA= 0; /*Initially the values of the whole pins in PORTA are 0*/

	unsigned int count_sec,count_min,count_hour,num_sec,num_min,num_hour;
	unsigned int stopwatch[6]; /*array where we save hours, minutes and seconds*/
	unsigned char i;

	while(1)
	{
		count_hour=0; /*Initially start hours from 0*/
		/*The final value for hour is 59, then the stop watch begin from 0 again*/
		while(count_hour<60)
		{
			count_min=0; /*Initially start minutes from 0*/
			/*The final value of minutes is 59, then it return to 0 & hour increases by 1*/
			while(count_min<60)
			{
				count_sec=0; /*Initially start seconds from 0*/
				/*The final value for seconds is 59, then it return to 0 & minutes increases by 1*/
				while(count_sec<60)
				{
					/*Here we get hours, minutes and seconds and save their values in array stopwatch*/
					num_sec=count_sec; /*Here we get the number of the seconds to divide them into sec1 & sec2*/
					stopwatch[0]= num_sec%10; /*Here sec1 is the unit of the number(the first digit from right)*/
					num_sec/=10; /*Divide the number by 10 to get the second number from right*/
					stopwatch[1]=num_sec%10; /*Here sec2 is the tenth of the number*/

					num_min=count_min; /*Here we get the number of the minutes to divide them into min1 & min2*/
					stopwatch[2]= num_min%10; /*Here min1 is the unit of the number(the first digit from right)*/
					num_min/=10; /*Divide the number by 10 to get the second number from right*/
					stopwatch[3]=num_min%10; /*Here min2 is the tenth of the number*/

					num_hour=count_hour; /*Here we get the number of the hours to divide them into hour1 & hour2*/
					stopwatch[4]= num_hour%10; /*Here hour1 is the unit of the number(the first digit from right)*/
					num_hour/=10; /*Divide the number by 10 to get the second number from right*/
					stopwatch[5]=num_hour%10; /*Here hour2 is the tenth of the number*/

					/* The idea is that we enable only one pin in PORTA and put on PORTC the correct
					 * output for 5ms, then we shift to enable the following pin in PORTA and put
					 * the correct value on PORTC and so on.
					 */
					PORTA=0x01; /*Start by putting one in the first pin (sec1)*/
					for(i=0;i<6;i++)
					{
						PORTC= stopwatch[i]; /*Get the value from the array and output it on PORTC*/
						_delay_ms(5);
						/*As long as we haven't finished the 6 ports, shift to enable the next pin*/
						if(PORTA!=0x20)
						{
							PORTA=(PORTA<<1);
						}
					}
					/*if INT1 is enabled, the seconds won't increase and the stop watch will pause
					 *otherwise it will increase if INT1 isn't enabled and 1 second have passed (Timer1_CTC)
					 */
					if((pause_flag==0) && (begin_flag==1))
					{
						count_sec++;
						begin_flag=0;/*Return the flag to 0, so if 1 second passed again it will be set to 1*/
					}
					if(pause_flag==1) /*In order when we resume, the next number have time to be displayed*/
					{
						begin_flag=0;/*Return the flag to 0, so if 1 second passed again it will be set to 1*/
					}
					if(reset_flag==1) /*if INT0 is enabled,reset all counters and return reset flag to 0*/
					{
						count_sec=0;
						count_min=0;
						count_hour=0;
						reset_flag=0; /*Return the flag to 0 to be able to count again*/
					}
				}
				begin_flag=1; /*In order to be able to increment the minutes*/

				/*if INT1 is enabled, the minutes won't increase and the stop watch will pause
				 *otherwise it will increase if INT1 isn't enabled and 1 second have passed (Timer1_CTC)
				 */
				if((pause_flag==0) && (begin_flag==1))
				{
					count_min++;
					begin_flag=0;/*Return the flag to 0, so if 1 second passed again it will be set to 1*/
				}
				if(pause_flag==1) /*In order when we resume, the next number have time to be displayed*/
				{
					begin_flag=0;/*Return the flag to 0, so if 1 second passed again it will be set to 1*/
				}
				if(reset_flag==1) /*if INT0 is enabled,reset all counters and return reset flag to 0*/
				{
					count_sec=0;
					count_min=0;
					count_hour=0;
					reset_flag=0; /*Return the flag to 0 to be able to count again*/
				}
			}
			begin_flag=1; /*In order to be able to increment the hours*/

			/*if INT1 is enabled, the hours won't increase and the stop watch will pause
			 * otherwise it will increase if INT1 isn't enabled and 1 second have passed (Timer1_CTC)
			 */
			if((pause_flag==0) && (begin_flag==1))
			{
				count_hour++;
				begin_flag=0;/*Return the flag to 0, so if 1 second passed again it will be set to 1*/
			}
			if(pause_flag==1) /*In order when we resume, the next number have time to be displayed*/
			{
				begin_flag=0;/*Return the flag to 0, so if 1 second passed again it will be set to 1*/
			}
			if(reset_flag==1) /*if INT0 is enabled, reset all counters and return reset flag to 0*/
			{
				count_sec=0;
				count_min=0;
				count_hour=0;
				reset_flag=0; /*Return the flag to 0 to be able to count again*/
			}
		}
	}
	return 0;
}
