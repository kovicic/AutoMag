/**********************************

$sudo ./l602
------------------
|  Raspberry Pi! |
|51.9C 215/477MB |
------------------

$sudo ./1602 QtSharp
------------------
|QtSharp         |
|51.9C 215/477MB |
------------------

$sudo ./1602 \ \ Hello\ World
------------------
|  hello world   |
|51.9C 215/477MB |
------------------

************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <wiringPi.h>
#include <lcd.h>

#define LCD_RS  25               //Register select pin
#define LCD_E   24               //Enable Pin
#define LCD_D0  23//29               //Data pin D0
#define LCD_D1  22//28               //Data pin D1
#define LCD_D2  21//27               //Data pin D2
#define LCD_D3  14//26               //Data pin D3
#define LCD_D4  0//23               //Data pin D4
#define LCD_D5  0//22               //Data pin D5
#define LCD_D6  0//21               //Data pin D6
#define LCD_D7  0//14               //Data pin D7


int main(int args, char *argv[])
{
	FILE *fp;
	int fd;
	char temp_char[15];
	char Total[20];
	char Free[20];
	float Temp;
	
	if(wiringPiSetup() == -1){
		exit(1);
	}
	
	
	
	
	
	
    //Initialise LCD(int rows, int cols, int bits, int rs, int enable, int d0, int d1, int d2, int d3, int d4, int d5, int d6, int d7)
	fd = lcdInit(2, 16, 4, LCD_RS, LCD_E, LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5, LCD_D6, LCD_D7); //see /usr/local/include/lcd.h
	if (fd == -1){
		printf ("lcdInit 1 failed\n") ;
		return 1;
	}
	sleep(1);

	lcdPosition(fd, 0, 0); lcdPuts (fd, "  Raspberry Pi!");
	sleep(1);

	if(argv[1]){
		lcdPosition(fd, 0, 0);
		lcdPuts(fd, "                ");
		lcdPosition(fd, 0, 0); lcdPuts(fd, argv[1]);
	} 

	while(1){
		fp=fopen("/sys/class/thermal/thermal_zone0/temp","r"); //read Rpi's tempture
		fgets(temp_char,9,fp);
		Temp = atof(temp_char)/1000;
		lcdPosition(fd, 0, 1); lcdPrintf(fd, "%3.1fC", Temp);
		fclose(fp);

		fp=fopen("/proc/meminfo","r"); //read RAM infomation
		fgets(Total,19,fp);
		fgets(Total,4,fp);
		fgets(Free,9,fp);
		fgets(Free,19,fp);
		fgets(Free,4,fp);
//		printf("%3d/%3dMB\n",atoi(Free),atoi(Total));
		lcdPosition(fd, 7, 1);
		lcdPrintf(fd, "%3d/%3dMB", atoi(Free), atoi(Total)) ;
		fclose(fp);

		sleep(1);
	}

	return 0;

}
