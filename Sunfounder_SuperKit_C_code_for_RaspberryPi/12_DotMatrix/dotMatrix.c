#include <wiringPi.h>
#include <stdio.h>

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
	int i;

	if(wiringPiSetup() == -1){ //when initialize wiring failed,print messageto screen
		printf("setup wiringPi failed !");
		return 1; 
	}

	init();

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
