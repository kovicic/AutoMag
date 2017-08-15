#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <lcd.h>

#define LCD_RS  29               //Register select pin
#define LCD_E   25               //Enable Pin
#define LCD_D0  24//29               //Data pin D0
#define LCD_D1  23//28               //Data pin D1
#define LCD_D2  22//27               //Data pin D2
#define LCD_D3  21//26               //Data pin D3
#define LCD_D4  0//23               //Data pin D4
#define LCD_D5  0//22               //Data pin D5
#define LCD_D6  0//21               //Data pin D6
#define LCD_D7  0//14               //Data pin D7

const unsigned char Buf[] = "---SUNFOUNDER---";
const unsigned char myBuf[] = "  sunfounder.com";

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
	lcdPuts(fd, "aasdasd");
	printf("fd is %d\n",fd);
	lcdPosition(fd, 0, 1); 
	lcdPuts(fd, "asdasd");

	sleep(2);
	lcdClear(fd);
	
	while(1){
		for(i=0;i<sizeof(Buf)-1;i++){
			lcdPosition(fd, i, 1);
			lcdPutchar(fd, *(Buf+i));
			printf("kovacic\n");
			delay(200);
		}
		lcdPosition(fd, 0, 1); 
		lcdClear(fd);
		sleep(0.5);
		for(i=0; i<16; i++){
			lcdPosition(fd, i, 0);
			lcdPutchar(fd, *(myBuf+i));
			delay(100);
		}
	}
	
	return 0;
}/*
#include <stdio.h>
#include <stdlib.h>
#include <wiringPi.h>
#include <lcd.h>

#define LCD_RS  29               //Register select pin
#define LCD_E   25               //Enable Pin
#define LCD_D0  24//29               //Data pin D0
#define LCD_D1  23//28               //Data pin D1
#define LCD_D2  22//27               //Data pin D2
#define LCD_D3  21//26               //Data pin D3
#define LCD_D4  0//23               //Data pin D4
#define LCD_D5  0//22               //Data pin D5
#define LCD_D6  0//21               //Data pin D6
#define LCD_D7  0//14               //Data pin D7
 
int main()
{
    int lcd;
    wiringPiSetup();
    lcd = lcdInit (2, 16, 8, LCD_RS, LCD_E, LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
	lcdClear(0);
    lcdPuts(lcd, "Hello, world!");
	
	
}*/
