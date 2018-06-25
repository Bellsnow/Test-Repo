#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <util/twi.h>

#define sbi(x,y) (x|=(1<<y)) //set to 1 at y th bit of x
#define cbi(x,y) (x&=~(1<<y)) //clear to 0 at y th bit of x
#define F_CPU 16000000UL //define AVR clock to constant number
#define sla_r 0x91 //define ADDR_GND+R address of adc
#define sla_w 0x90 //define ADDR_GND+W address of adc

void init_AVR(); //to initiate avr !!

char adc_config(unsigned int stat); //configuring 16bit_adc !!
int adc_conversion(); //conversion function of twi from 16bit_adc !!
char adc_change_to_con(); //function for using con. reg. !!
char adc_comparator_L(int Lo_thresh); //function for Lo_thresh !!
char adc_comparator_H(int Hi_thresh); //function for Hi_thresh !!

char twi_addressing(unsigned char addr, unsigned char ack, unsigned char nack); //addressing function of twi !!
char twi_reg_pointer(unsigned char pointer); //pointer function of 16bit_adc !!

void uart_trans(char data); //transfering function of uart to PC !!
void put_string(char *str); //function to print string !!
void uart_adc(int data); //transfering 16bits data of weight function !!

volatile unsigned char count_1sec; //counter variable
volatile unsigned char count_10sec;

volatile int current_data; //adc_data variable
volatile int past_data;

unsigned char count_feed; //feed & having count
unsigned char count_meal;

volatile char err; //error variable


int main() {

	/////////////////initial setting//////////////////
	init_AVR();
	
	//setting SCL_F to 100kHz
	TWBR |= 0x48; //TWBR is 72
	cbi(TWSR,1);
	cbi(TWSR,0); //freescaler is 4^0=1

	sei(); //enable interrupt

	//setting status of 16bits adc
	//OS:0 / MUX:AINp=AIN0, AINn=AIN1 / FS=0.256
	//MODE:continuous conversion / 128SPS, window comparator
	//active low, non-latching, assert one conversion
	unsigned int stat = 0x0E90;
	err = adc_config(stat);

	//recieve initial data
	err = adc_change_to_con();
	past_data = adc_conversion(); //initial data

	//print current status
	put_string("Current Mass: ");
	uart_adc(past_data);
	put_string(" Feed Count: ");
	uart_trans(count_feed);
	put_string(" Meal Count: ");
	uart_trans(count_meal);
	put_string("\r\n");

	//initiate external interrupt
	sbi(EIMSK,INT4); //enable int4

	while(1) {
		if(count_1sec >= 61) {
			count_1sec -= 61;
			count_10sec++;
		}

		if(count_10sec >= 10) {
			cbi(EIMSK,INT4); //unable int4
			cbi(TIMSK,TOIE0); //unable overflow interrupt
			TCCR0 &= ~(0x07); //stop timer/counter
			sbi(TIFR,TOV0); //clear overflow flag
			TCNT0 &= 0x00; //initiate counting reg.
			count_1sec = 0; //initiate 1 sec counter
			count_10sec = 0; //initiate 10 sec counter

			current_data = adc_conversion();
			
			if(current_data > past_data)
				count_feed++;

			else if(current_data < past_data)
				count_meal++;

			err = adc_comparator_H((current_data+1));
			err = adc_comparator_L((current_data-1));
			
			put_string("Current Mass: ");
			uart_adc(current_data);
			put_string(" Feed Count: ");
			uart_trans(count_feed);
			put_string(" Meal Count: ");
			uart_trans(count_meal);
			put_string("\r\n");

			past_data = current_data; //reset past_data

			sbi(EIMSK,INT4); //enable INT4
		}
	}
}

void init_AVR() {
	// Declare your local variables here

	// Declare your local variables here

	// Input/Output Ports initialization
	// Port A initialization
	// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
	// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
	PORTA=0x00;
	DDRA=0x00;
	
	// Port B initialization
	// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
	// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
	PORTB=0x00;
	DDRB=0x00;
	
	// Port C initialization
	// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
	// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
	PORTC=0x00;
	DDRC=0x00;
	
	// Port D initialization
	// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
	// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
	PORTD=0x00;
	DDRD=0x00;
	
	// Port E initialization
	// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
	// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
	PORTE=0x00;
	DDRE=0x00;
	
	// Port F initialization
	// Func7=In Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=In 
	// State7=T State6=T State5=T State4=T State3=T State2=T State1=T State0=T 
	PORTF=0x00;
	DDRF=0x00;
	
	// Port G initialization
	// Func4=In Func3=In Func2=In Func1=In Func0=In 
	// State4=T State3=T State2=T State1=T State0=T 
	PORTG=0x00;
	DDRG=0x00;
	

	// Timer/Counter 0 initialization
	// Clock source: System Clock
	// Clock value: Timer 0 Stopped
	// Mode: Normal top=FFh
	// OC0 output: Disconnected
	ASSR=0x00;
	TCCR0=0x00;
	TCNT0=0x00;
	OCR0=0x00;
	
	// Timer/Counter 1 initialization
	// Clock source: System Clock
	// Clock value: Timer 1 Stopped
	// Mode: Normal top=FFFFh
	// OC1A output: Discon.
	// OC1B output: Discon.
	// OC1C output: Discon.
	// Noise Canceler: Off
	// Input Capture on Falling Edge
	// Timer 1 Overflow Interrupt: Off
	// Input Capture Interrupt: Off
	// Compare A Match Interrupt: Off
	// Compare B Match Interrupt: Off
	// Compare C Match Interrupt: Off
	TCCR1A=0x00;
	TCCR1B=0x00;
	TCNT1H=0x00;
	TCNT1L=0x00;
	ICR1H=0x00;
	ICR1L=0x00;
	OCR1AH=0x00;
	OCR1AL=0x00;
	OCR1BH=0x00;
	OCR1BL=0x00;
	OCR1CH=0x00;
	OCR1CL=0x00;
	
	// Timer/Counter 2 initialization
	// Clock source: System Clock
	// Clock value: Timer 2 Stopped
	// Mode: Normal top=FFh
	// OC2 output: Disconnected
	TCCR2=0x00;
	TCNT2=0x00;
	OCR2=0x00;
	
	// Timer/Counter 3 initialization
	// Clock source: System Clock
	// Clock value: Timer 3 Stopped
	// Mode: Normal top=FFFFh
	// Noise Canceler: Off
	// Input Capture on Falling Edge
	// OC3A output: Discon.
	// OC3B output: Discon.
	// OC3C output: Discon.
	// Timer 3 Overflow Interrupt: Off
	// Input Capture Interrupt: Off
	// Compare A Match Interrupt: Off
	// Compare B Match Interrupt: Off
	// Compare C Match Interrupt: Off
	TCCR3A=0x00;
	TCCR3B=0x00;
	TCNT3H=0x00;
	TCNT3L=0x00;
	ICR3H=0x00;
	ICR3L=0x00;
	OCR3AH=0x00;
	OCR3AL=0x00;
	OCR3BH=0x00;
	OCR3BL=0x00;
	OCR3CH=0x00;
	OCR3CL=0x00;
	
	// External Interrupt(s) initialization
	// INT0: Off
	// INT1: Off
	// INT2: Off
	// INT3: Off
	// INT4: Off
	// INT5: Off
	// INT6: Off
	// INT7: Off
	EICRA=0x00;
	EICRB=0x00;
	EIMSK=0x00;
	
	// Timer(s)/Counter(s) Interrupt(s) initialization
	TIMSK=0x00;
	ETIMSK=0x00;

	UCSR0A = 0x02; //U2X is setted
	UCSR0B = 0xF8; //[0b11111000]
	UCSR0C = 0x06; //[0b00000110]asynchronized mode, no parity, 1 stop bit, 8bit data
	UBRR0H = 0x03; //16Mhz, UBRR = 832
	UBRR0L = 0x40; //Baud rate is 2400bps
	
	// Analog Comparator initialization
	// Analog Comparator: Off
	// Analog Comparator Input Capture by Timer/Counter 1: Off
	ACSR=0x80;
	SFIOR=0x00;
}

//////////////////TWI///////////////////

int adc_conversion() {
	char x; //error variable
	int higher, lower, data;

	x = twi_addressing(sla_r,TW_MR_SLA_ACK,TW_MR_SLA_NACK);
	if(x < 0)
		return x; //address error

	TWCR = (1<<TWINT) | (1<<TWEN); //accept to recieve data packet
	while(!(TWCR & (1<<TWINT))); //waiting
	if(TW_STATUS != TW_MR_DATA_ACK) {
		if(TW_STATUS != TW_MR_DATA_NACK)
			return -3;
		else
			return -5; //recieve high data NACK
	}

	higher = TWDR; //load higher weight from TWDR buffer

	TWCR = (1<<TWINT) | (1<<TWEN); //accept to recieve data packet
	while(!(TWCR & (1<<TWINT))); //waiting
	if(TW_STATUS != TW_MR_DATA_ACK) {
		if(TW_STATUS != TW_MR_DATA_NACK)
			return -3;
		else
			return -6; //recieve low data NACK
	}

	lower = TWDR; //load lower weight from TWDR buffer
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); //send stop cond.
	while(TWCR & (1<<TWSTO));
	cbi(TWCR,TWSTA);

	data = (higher<<8) | lower;

	return data; //TWI transmitting success
}

char adc_config(unsigned int stat) {
	char x;
	char higher, lower;

	x = twi_addressing(sla_w,TW_MT_DATA_ACK,TW_MT_DATA_NACK);
	if(x < 0)
		return x;

	x = twi_reg_pointer(0x01);
	if(x < 0)
		return x; //pointer error

	higher = (stat>>8); //MSB of stat reg.
	TWDR = higher; //writing to buffer MSB of stat

	TWCR = (1<<TWINT) | (1<<TWEN); //accept to send higher stat packet
	while(!(TWCR & (1<<TWINT))); //waiting
	if(TW_STATUS != TW_MT_DATA_ACK) {
		if(TW_STATUS != TW_MT_DATA_NACK)
			return -3;
		else
			return -7; //recieve higher stat packet NACK
	}

	lower = stat; //LSB of stat reg.
	TWDR = lower; //writing to buffer LSB of stat

	TWCR = (1<<TWINT) | (1<<TWEN); //accept to send lower stat packet
	while(!(TWCR & (1<<TWINT))); //waiting
	if(TW_STATUS != TW_MT_DATA_ACK) {
		if(TW_STATUS != TW_MT_DATA_NACK)
			return -3;
		else
			return -8; //recieve lower stat packet NACK
	}
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); //send stop cond.
	while(TWCR & (1<<TWSTO));
	cbi(TWCR,TWSTA);

	return 0; //TWI transmitting success
}

char adc_change_to_con() {
	char x;

	x = twi_addressing(sla_w,TW_MT_SLA_ACK,TW_MT_SLA_NACK);
	if(x < 0)
		return x;

	x = twi_reg_pointer(0x00);
	if(x < 0)
		return x;
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); //send stop cond.
	while(TWCR & (1<<TWSTO));
	cbi(TWCR,TWSTA);

	return 0; //TWI transmitting success
}

char twi_addressing(unsigned char addr, unsigned char ack, unsigned char nack) {
	sbi(TWCR,TWIE); //TWI interrupt enable
	sbi(TWCR,TWEA); //TWI Ack enable

	TWCR = (1<<TWINT) | (1<<TWSTA) | (1<<TWEN); //TWI start
	while(!(TWCR & (1<<TWINT))); //waiting TWINT to set to 1
	if((TW_STATUS != TW_START) && (TW_STATUS != TW_REP_START))//check start cond.
		return -1; //error return

	TWDR = addr; //write address+WRITE to data register
	TWCR = (1<<TWINT) | (1<<TWEN); //clearing TWINT
	while(!(TWCR & (1<<TWINT))); //waiting
	if(TW_STATUS != ack) {
		if(TW_STATUS != nack) //lost in addressing
			return -3; //error return
		else //recieve NACK
			return -2; //error return
	}
	
	return 0; //addressing success
}

char twi_reg_pointer(unsigned char pointer) {
	TWDR = pointer; //writing to buffer reg. pointer

	TWCR = (1<<TWINT) | (1<<TWEN); //accept to send pointer packet
	while(!(TWCR & (1<<TWINT))); //waiting
	if(TW_STATUS != TW_MT_DATA_ACK) {
		if(TW_STATUS != TW_MT_DATA_NACK)
			return -3;
		else
			return -4; //recieve pointer NACK
	}

	return 0; //reg. pointer sending success
}

//////////////////UART//////////////////

void uart_trans(char data) {
	while(!(UCSR0A & (1<<UDRE))); //waiting TX buffer
	UDR0 = data; //writing at TX buffer
}

void put_string(char *str) {
	while(*str)
		uart_trans(*str++); //printing string
}

void uart_adc(int data) {
	char higher; //high 8bits of weight data
	unsigned char lower; //low 8bits of weight data

	higher = (data>>8);
	lower = data;

	uart_trans(higher); //send higher to PC
	uart_trans(lower); //send lower to PC
}


/////////////comparator///////////////

char adc_comparator_L(int Lo_thresh) {
	char x;
	char higher;
	unsigned char lower;

	x = twi_addressing(sla_w,TW_MT_SLA_ACK,TW_MT_SLA_NACK);
	if(x < 0)
		return x;

	x = twi_reg_pointer(0x02);
	if(x < 0)
		return x;

	higher = (Lo_thresh>>8); //high 8bits of Lo_thresh
	TWDR = higher; //writing to buffer

	TWCR = (1<<TWINT) | (1<<TWEN); //accept to send higher Lo_thresh packet
	while(!(TWCR & (1<<TWINT))); //waiting
	if(TW_STATUS != TW_MT_DATA_ACK) {
		if(TW_STATUS != TW_MT_DATA_NACK)
			return -3;
		else
			return -9; //recieve higher Lo_thresh packet NACK
	}

	lower = Lo_thresh; //LSB of Lo_thresh.
	TWDR = lower; //writing to buffer LSB of stat

	TWCR = (1<<TWINT) | (1<<TWEN); //accept to send lower Lo_thresh packet
	while(!(TWCR & (1<<TWINT))); //waiting
	if(TW_STATUS != TW_MT_DATA_ACK) {
		if(TW_STATUS != TW_MT_DATA_NACK)
			return -3;
		else
			return -10; //recieve lower Lo_thresh packet NACK
	}
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); //send stop cond.
	while(TWCR & (1<<TWSTO));
	cbi(TWCR,TWSTA);

	return 0; //TWI transmitting success
}

char adc_comparator_H(int Hi_thresh) {
	char x;
	char higher;
	unsigned char lower;

	x = twi_addressing(sla_w,TW_MT_SLA_ACK,TW_MT_SLA_NACK);
	if(x < 0)
		return x;

	x = twi_reg_pointer(0x03);
	if(x < 0)
		return x;

	higher = (Hi_thresh>>8); //high 8bits of Hi_thresh
	TWDR = higher; //writing to buffer

	TWCR = (1<<TWINT) | (1<<TWEN); //accept to send higher Hi_thresh packet
	while(!(TWCR & (1<<TWINT))); //waiting
	if(TW_STATUS != TW_MT_DATA_ACK) {
		if(TW_STATUS != TW_MT_DATA_NACK)
			return -3;
		else
			return -11; //recieve higher Lo_thresh packet NACK
	}

	lower = Hi_thresh; //LSB of Lo_thresh.
	TWDR = lower; //writing to buffer LSB of stat

	TWCR = (1<<TWINT) | (1<<TWEN); //accept to send lower Lo_thresh packet
	while(!(TWCR & (1<<TWINT))); //waiting
	if(TW_STATUS != TW_MT_DATA_ACK) {
		if(TW_STATUS != TW_MT_DATA_NACK)
			return -3;
		else
			return -12; //recieve lower Lo_thresh packet NACK
	}
	
	TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWSTO); //send stop cond.
	while(TWCR & (1<<TWSTO));
	cbi(TWCR,TWSTA);

	return 0; //TWI transmitting success
}

ISR(INT4_vect) {
	cbi(TIMSK,TOIE0); //unable overflow interrupt
	TCCR0 &= ~(0x07); //stop timer/counter
	sbi(TIFR,TOV0); //clear overflow flag
	TCNT0 &= 0x00; //initiate counting reg.
	count_1sec = 0; //initiate 1 sec counter
	count_10sec = 0; //initiate 10 sec counter

	err = adc_change_to_con();
	if(err<0)
		reti(); //exit ISR if error is occurred
	
	current_data = adc_conversion(); //recieve threshold

	if(current_data > past_data)
		err = adc_comparator_H((current_data+1));

	else if(current_data < past_data)
		err = adc_comparator_L((current_data-1));

	else
		err = 0;

	sbi(TIMSK,TOIE0); //enable overflow interrupt
	TCCR0 |= 0x07; //start timer/counter
	reti(); //exit ISR
}

ISR(TIMER0_OVF_vect) {
	count_1sec++; //1 overflow count
	reti();
}
