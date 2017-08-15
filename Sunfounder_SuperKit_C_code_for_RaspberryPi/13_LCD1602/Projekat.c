#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <lcd.h>

#define LCD_RS  29               //Register select pin
#define LCD_E   25               //Enable Pin
#define LCD_D0  24 //29               //Data pin D0
#define LCD_D1  23 //28               //Data pin D1
#define LCD_D2  22 //27               //Data pin D2
#define LCD_D3  21 //26               //Data pin D3
#define LCD_D4  0//23               //Data pin D4
#define LCD_D5  0//22               //Data pin D5
#define LCD_D6  0//21               //Data pin D6
#define LCD_D7  0//14               //Data pin D7

#define   SDI   28   //serial data input
#define   RCLK  27   //memory clock input(STCP)
#define   SRCLK 26   //shift register clock input(SHCP)
						//LEVEL 1-8
unsigned char code_H[3] = {0x09,0xff,0xc0};//{0xc0, 0xe0,0xf0, 0xf8, 0xfc, 0xfe, 0xff}
unsigned char code_L[3] = {0xf0,0xfe,0x3f};//{uvek 0x3f    }

//unsigned char code_L[8] = {0x00,0x00,0x3c,0x42,0x42,0x3c,0x00,0x00};
//unsigned char code_H[8] = {0xff,0xe7,0xdb,0xdb,0xdb,0xdb,0xe7,0xff};

//unsigned char code_L[8] = {0xff,0xff,0xc3,0xbd,0xbd,0xc3,0xff,0xff};
//unsigned char code_H[8] = {0x00,0x18,0x24,0x24,0x24,0x24,0x18,0x00};

void init(void)
{
	pinMode(SDI, OUTPUT); //make P0 output
	pinMode(RCLK, OUTPUT); //make P0 output
	pinMode(SRCLK, OUTPUT); //make P0 output

	digitalWrite(SDI, 0);
	digitalWrite(RCLK, 0);
	digitalWrite(SRCLK, 0);
}

void hc595_in(unsigned char dat)
{
	int i;

	for(i=0;i<8;i++){
		digitalWrite(SDI, 0x80 & (dat << i));
		digitalWrite(SRCLK, 1);
	//	delay(1);
		digitalWrite(SRCLK, 0);
	}
}

void hc595_out()
{
	digitalWrite(RCLK, 1);
	//delay(1);
	digitalWrite(RCLK, 0);
}

int main(void)
{
	int fd;
	int i;
	
	setenv("WIRINGPI_GPIOMEM", "1", 1);
	if(wiringPiSetup() == -1){
		exit(1);
	}
	//extern int  lcdInit (const int rows, const int cols, const int bits,
	//const int rs, const int strb,
	//const int d0, const int d1, const int d2, const int d3, const int d4,
	//const int d5, const int d6, const int d7) ;
	fd = lcdInit(2, 16, 4, LCD_RS, LCD_E, LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5, LCD_D6, LCD_D7); //see /usr/local/include/lcd.h
	if (fd == -1){
		printf("lcdInit 1 failed\n") ;
		return 1;
	}
	sleep(1);

	lcdClear(fd);
	lcdPosition(fd, 0, 0); 
	lcdPuts(fd, "kova i kole projekat");
	//printf("fd is %d\n",fd);
	lcdPosition(fd, 0, 1); 
	lcdPuts(fd, "mile i tesic ");

	
	while(1){
		for(i=0;i<sizeof(code_H);i++){
			hc595_in(code_L[i]);
			hc595_in(code_H[i]);
			hc595_out();
		//	delay(100);
		}
		
		for(i=sizeof(code_H);i>=0;i--){
			hc595_in(code_L[i]);
			hc595_in(code_H[i]);
			hc595_out();
		//	delay(100);
		}
	}

	return 0;
}


