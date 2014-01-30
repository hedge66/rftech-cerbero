/*
 * RF_Tranceiver.c
 *
 * Created: 2012-08-10 15:24:35
 *  Author: Kalle
 *  Atmega88
 */ 

#include <avr/io.h>
#include <stdio.h>
#define F_CPU 8000000UL  // 8 MHz
#include <util/delay.h>
#include <avr/interrupt.h>

#include "nRF24L01.h"

#define CSn_HIGH PORTB|=_BV(2)
#define CSn_LOW PORTB&=~_BV(2)
#define CE_HIGH PORTB|=_BV(1)
#define CE_LOW PORTB&=~_BV(1)
#define LED_ON PORTB|=_BV(0)
#define LED_OFF PORTB&=~_BV(0)

#define dataLen 3  //lunghezza dei pacchetti di dati inviati / ricevuti
#define W 0  //
#define R 1  //
uint8_t *data;
uint8_t *arr;


/*****************ändrar klockan till 8MHz ist för 1MHz*****************************/
void clockprescale(void)	
{
	CLKPR = 0b10000000;	//Prepare the chip for a change of clock prescale (CLKPCE=1 and the rest zeros)
	CLKPR = 0b00000000;	//Wanted clock prescale (CLKPCE=0 and the four first bits CLKPS0-3 sets division factor = 1)
	//See page 38 in datasheet
}
////////////////////////////////////////////////////




/*****************SPI*****************************/  //Skickar data mellan chip och nrf'ens chip
//initiering
void InitSPI(void)
{
	//Set SCK (PB5), MOSI (PB3) , CSN (SS & PB2) & C  as outport 
	//NOTA! Deve essere impostato prima dell'abilitazione dell'SPI
	DDRB |= (1<<DDB5) | (1<<DDB3) | (1<<DDB2) |(1<<DDB1);
	
	/* Enable SPI, Master, set clock rate fck/16 .. */
	SPCR |= (1<<SPE)|(1<<MSTR);// |(1<<SPR0) |(1<<SPR1);
	
	CSn_HIGH;	//CSN IR_High to start with, noi non inviare nulla a nrf'en ancora!
	CE_LOW;	//CE low to start with, nrf'en non inviare / ricevere ancora nulla!
}

//Skickar kommando till nrf'en å får då tillbaka en byte
char WriteByteSPI(unsigned char cData)
{
	//Load byte to Data register
	SPDR = cData;	
		
	/* Wait for transmission complete */
	while(!(SPSR & (1<<SPIF)));
	
	//Returnera det som sänts tillbaka av nrf'en (första gången efter csn-låg kommer Statusregistert)
	return SPDR;
}
////////////////////////////////////////////////////


/*****************in/out***************************/  //ställ in t.ex. LED
//sätter alla I/0 portar för t.ex. LED
void ioinit(void)			
{
	DDRB |= (1<<DDB0); //led
}
////////////////////////////////////////////////////


/*****************interrupt***************************/ //orsaken till att köra med interrupt är att de avbryter koden var den än är och kör detta som är viktigast!
//när data tas emot/skickas så går interr uptet INT0 näst längst ner igång
void INT0_interrupt_init(void)	
{
	DDRD &= ~(1<<DDD2);	//Extern interrupt su INT0
	PORTD |= (1<<PD2);	//Abilita pullup su INT0
	
	EICRA |=  (1<<ISC01);// INT0 falling edge	PD2
	EICRA  &=  ~(1<<ISC00);// INT0 falling edge	PD2

	EIMSK |=  (1<<INT0);	//enablar int0
  	//sei();              // Enable global interrupts görs sen
} 

//när chipets RX (usart) får ett meddelande fårn datorn går interruptet USART_RX igång längst ner.

//////////////////////////////////////////////////////

//funktion för att hämta nåt av nrf's register
uint8_t GetReg(uint8_t reg)
{	
	//andvändning: USART_Transmit(GetReg(STATUS)); //där status är registret du vill kolla
	_delay_us(10);
	CSn_LOW;	//CSN low
	_delay_us(10);
	WriteByteSPI(R_REGISTER + reg);	//Vilket register vill du läsa (nu med R_Register för att inget ska skrivas till registret)
	_delay_us(10);
	reg = WriteByteSPI(NOP);	//Skicka NOP antalet byte som du vill hämta (oftast 1gång, men t.ex addr är 5 byte!) och spara isf inte i "reg" utan en array med en loop
	_delay_us(10);
	CSn_HIGH;	//CSN IR_High
	return reg;	// Returnerar registret förhoppningsvis med bit5=1 (tx_ds=lyckad sändning)
}


/*****************nrf-setup***************************/ //Ställer in nrf'en genoma att först skicka vilket register, sen värdet på registret.
uint8_t *WriteToNrf(uint8_t ReadWrite, uint8_t reg, uint8_t *val, uint8_t antVal)	//tar in "ReadWrite" (W el R), "reg" (ett register), "*val" (en array) & "antVal" (antal integer i variabeln)
{
	cli();	//disable global interrupt
	
	if (ReadWrite == W)	//W=vill skriva till nrf-en (R=läsa av den, R_REGISTER (0x00) ,så skiter i en else funktion)
	{
		reg = W_REGISTER + reg;	//ex: reg = EN_AA: 0b0010 0000 + 0b0000 0001 = 0b0010 0001  
	}
	
	//Static uint8_t för att det ska gå att returnera en array (lägg märke till "*" uppe på funktionen!!!)
	static uint8_t ret[dataLen];	//antar att det längsta man vill läsa ut när man kallar på "R" är dataleng-långt, dvs använder man bara 1byte datalengd å vill läsa ut 5byte RF_Adress så skriv 5 här ist!!!	
	
	_delay_us(10);		//alla delay är så att nrfen ska hinna med! (microsekunder)
	CSn_LOW;	//CSN low = nrf-chippet börjar lyssna
	_delay_us(10);		
	WriteByteSPI(reg);	//första SPI-kommandot efter CSN-låg berättar för nrf'en vilket av dess register som ska redigeras ex: 0b0010 0001 write to registry EN_AA	
	_delay_us(10); 		
	
	int i;
	for(i=0; i<antVal; i++)
	{
		if (ReadWrite == R && reg != W_TX_PAYLOAD)
		{
			ret[i]=WriteByteSPI(NOP);	//Andra och resten av SPI kommandot säger åt nrfen vilka värden som i det här fallet ska läsas
			_delay_us(10);			
		}
		else 
		{
			WriteByteSPI(val[i]);	//Andra och resten av SPI kommandot säger åt nrfen vilka värden som i det här fallet ska skrivas till
			_delay_us(10);
		}		
	}
	CSn_HIGH;	//CSN IR_High = nrf-chippet slutar lyssna
	
	sei(); //enable global interrupt
	
	return ret;	//returnerar en array
}

//initierar nrf'en (obs nrfen måste vala i vila när detta sker CE-låg)
void nrf24L01_init(void)
{
	_delay_ms(100);	//allow radio to reach power down if shut down
	
	uint8_t val[5];	//en array av integers som skickar värden till WriteToNrf-funktionen

	//EN_AA - (auto-acknowledgements) - Transmittern får svar av recivern att packetet kommit fram, grymt!!! (behöver endast vara enablad på Transmittern!)
	//Kräver att Transmittern även har satt SAMMA RF_Adress på sin mottagarkanal nedan ex: RX_ADDR_P0 = TX_ADDR
	val[0]=0x01;	//ger första integern i arrayen "val" ett värde: 0x01=EN_AA på pipe P0. 
	WriteToNrf(W, EN_AA, val, 1);	//W=ska skriva/ändra nåt i nrfen, EN_AA=vilket register ska ändras, val=en array med 1 till 32 värden  som ska skrivas till registret, 1=antal värden som ska läsas ur "val" arrayen.
	
	//SETUP_RETR (the setup for "EN_AA")
	val[0]=0x2F;	//0b0010 00011 "2" sets it up to 750uS delay between every retry (at least 500us at 250kbps and if payload >5bytes in 1Mbps, and if payload >15byte in 2Mbps) "F" is number of retries (1-15, now 15)
	WriteToNrf(W, SETUP_RETR, val, 1);
	
	//Väljer vilken/vilka datapipes (0-5) som ska vara igång.
	val[0]=0x01;
	WriteToNrf(W, EN_RXADDR, val, 1); //enable data pipe 0

	//RF_Adress width setup (hur många byte ska RF_Adressen bestå av? 1-5 bytes) (5bytes säkrare då det finns störningar men långsammare dataöverföring) 5addr-32data-5addr-32data....
	val[0]=0x03;
	WriteToNrf(W, SETUP_AW, val, 1); //0b0000 00011 motsvarar 5byte RF_Adress

	//RF channel setup - väljer frekvens 2,400-2,527GHz 1MHz/steg
	val[0]=0x01;
	WriteToNrf(W, RF_CH, val, 1); //RF channel registry 0b0000 0001 = 2,401GHz (samma på TX å RX)

	//RF setup	- väljer effekt och överföringshastighet 
	val[0]=0x07;
	WriteToNrf(W, RF_SETUP, val, 1); //00000111 bit 3="0" ger lägre överföringshastighet 1Mbps=Längre räckvidd, bit 2-1 ger effektläge hög (-0dB) ("11"=(-18dB) ger lägre effekt =strömsnålare men lägre range)

	//RX RF_Adress setup 5 byte - väljer RF_Adressen på Recivern (Måste ges samma RF_Adress om Transmittern har EN_AA påslaget!!!)
	int i;
	for(i=0; i<5; i++)	
	{
		val[i]=0x12;	//RF channel registry 0b10101011 x 5 - skriver samma RF_Adress 5ggr för att få en lätt och säker RF_Adress (samma på transmitterns chip!!!)
	}
	WriteToNrf(W, RX_ADDR_P0, val, 5); //0b0010 1010 write registry - eftersom vi valde pipe 0 i "EN_RXADDR" ovan, ger vi RF_Adressen till denna pipe. (kan ge olika RF_Adresser till olika pipes och därmed lyssna på olika transmittrar) 	
	
	//TX RF_Adress setup 5 byte -  väljer RF_Adressen på Transmittern (kan kommenteras bort på en "ren" Reciver)
	//int i; //återanvänder föregående i...
	for(i=0; i<5; i++)	
	{
		val[i]=0x12;	//RF channel registry 0b10111100 x 5 - skriver samma RF_Adress 5ggr för att få en lätt och säker RF_Adress (samma på Reciverns chip och på RX-RF_Adressen ovan om EN_AA enablats!!!)
	}
	WriteToNrf(W, TX_ADDR, val, 5); 

	// payload width setup - Hur många byte ska skickas per sändning? 1-32byte 
	val[0]=dataLen;		//"0b0000 0001"=1 byte per 5byte RF_Adress  (kan välja upp till "0b00100000"=32byte/5byte RF_Adress) (definierat högst uppe i global variabel!)
	WriteToNrf(W, RX_PW_P0, val, 1);
	
	//CONFIG reg setup - Nu är allt inställt, boota upp nrf'en och gör den antingen Transmitter lr Reciver
	val[0]=0x1E;  //0b0000 1110 config registry	bit "1":1=power up,  bit "0":0=transmitter (bit "0":1=Reciver) (bit "4":1=>mask_Max_RT,dvs IRQ-vektorn reagerar inte om sändningen misslyckades. 
	WriteToNrf(W, CONFIG, val, 1);

//device need 1.5ms to reach standby mode
	_delay_ms(100);	

	//sei();	
}

void ChangeAddress(uint8_t adress)
{
	_delay_ms(100);
	uint8_t val[5];
	//RX RF_Adress setup 5 byte - väljer RF_Adressen på Recivern (Måste ges samma RF_Adress om Transmittern har EN_AA påslaget!!!)
	int i;
	for(i=0; i<5; i++)
	{
		val[i]=adress;	//RF channel registry 0b10101011 x 5 - skriver samma RF_Adress 5ggr för att få en lätt och säker RF_Adress (samma på transmitterns chip!!!)
	}
	WriteToNrf(W, RX_ADDR_P0, val, 5); //0b0010 1010 write registry - eftersom vi valde pipe 0 i "EN_RXADDR" ovan, ger vi RF_Adressen till denna pipe. (kan ge olika RF_Adresser till olika pipes och därmed lyssna på olika transmittrar)
	
	//TX RF_Adress setup 5 byte -  väljer RF_Adressen på Transmittern (kan kommenteras bort på en "ren" Reciver)
	//int i; //återanvänder föregående i...
	for(i=0; i<5; i++)
	{
		val[i]=adress;	//RF channel registry 0b10111100 x 5 - skriver samma RF_Adress 5ggr för att få en lätt och säker RF_Adress (samma på Reciverns chip och på RX-RF_Adressen ovan om EN_AA enablats!!!)
	}
	WriteToNrf(W, TX_ADDR, val, 5);
	_delay_ms(100);
}
/////////////////////////////////////////////////////

/*****************Funktioner***************************/ //Funktioner som används i main
//Resetta nrf per la nuova comunicazione
void reset(void)
{
	_delay_us(10);
	CSn_LOW;	//CSN low
	_delay_us(10);
	WriteByteSPI(W_REGISTER + STATUS);	//
	_delay_us(10);
	WriteByteSPI(0b01110000);	//radedrar alla irq i statusregistret (för att kunna lyssna igen)
	_delay_us(10);
	CSn_HIGH;	//CSN IR_High
}

//Reciverfunktioner
/*********************Reciverfunktioner********************************/
//Il ricevitore si apre e "Ascolto" in 1s
void receive_payload(void)
{
	sei();		//Enable global interrupt
	
	CE_HIGH;	//CE IR_High = "ascolto" 
	_delay_ms(1000);	//ascolto 1s e ha ricevuto salta al vettore di interrupt INT0
	CE_LOW; //CE IR_llow = "stop ascolto" 
	
	cli();	//Disable global interrupt
}

//Sänd data
void transmit_payload(uint8_t * W_buff)
{
	WriteToNrf(R, FLUSH_TX, W_buff, 0); //Cancella eventuali dati nel registro di TX dell'nrf
	
	WriteToNrf(R, W_TX_PAYLOAD, W_buff, dataLen);	//skickar datan i W_buff till nrf-en (obs går ej att läsa w_tx_payload-registret!!!)
	
	sei();	//enable global interrupt- redan på!
	//USART_Transmit(GetReg(STATUS));

	_delay_ms(10);		//bisogno davvero essere ms, non noi? Yeees! altrimenti non funzionerà!
	CE_HIGH;	//CE hög=sänd data	INT0 interruptet körs när sändningen lyckats och om EN_AA är på, också svaret från recivern är mottagen
	_delay_us(20);		//minst 10us!
	CE_LOW;	//CE låg
	_delay_ms(10);		//behöver det verkligen vara ms å inte us??? JAAAAAA! annars funkar det inte!!!

	//cli();	//Disable global interrupt... ajabaja, då stängs USART_RX-lyssningen av!

}


/////////////////////////////////////////////////////

int main(void)
{
	uint8_t W_buffer[dataLen];	//Creates a buffer to receive data with specified length (ex. dataLen = 5 bytes)
	static uint8_t tmp;

	clockprescale();
	InitSPI();
    	ioinit();
	INT0_interrupt_init();

	nrf24L01_init();

	LED_ON;		//per controllare che il lde funzioni!
	_delay_ms(1000);
	LED_OFF;


	
	tmp = GetReg(7);
	ChangeAddress(0x12);	//change address to send to different receiver
	W_buffer[0] = 0x30;
	W_buffer[1] = 0x31;
	W_buffer[2] = 0x32;
	transmit_payload(W_buffer);	//Sänder datan

	while(1)
	{
		LED_ON;		//per controllare che il lde funzioni!
		_delay_ms(500);
		LED_OFF;
		_delay_ms(500);
	}
	return 0;
}




ISR(INT0_vect)	//vettore che viene attivato quando transmit_payload riuscito a inviare o quando receive_payload i dati ricevuti NOTA: quando Mask_Max_rt è impostato nel registro di configurazione in modo che non si spegnerà quando MAX_RT è stato raggiunto sulle nmisslyckats lodge mailing!
{
	cli();	//Disable global interrupt
	CE_LOW;		//ce låg igen -sluta lyssna/sända
	
	LED_ON; //led on
	_delay_ms(500);
	LED_OFF; //led off
	
	//Receiver function to print out on usart:
	//data=WriteToNrf(R, R_RX_PAYLOAD, data, dataLen);	//läs ut mottagen data
	//reset();
//
	//for (int i=0;i<dataLen;i++)
	//{
		//USART_Transmit(data[i]);
	//}
	//
	sei();

}


