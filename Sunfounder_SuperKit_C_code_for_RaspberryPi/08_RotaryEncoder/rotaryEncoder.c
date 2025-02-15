/**********************************************************************
* Filename    : rotaryEncoder.c
* Description : make a rotaryEncoder work.
* Author      : Robot
* E-mail      : support@sunfounder.com
* website     : www.sunfounder.com
* Date        : 2014/08/27
**********************************************************************/
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <wiringPi.h>

#define  RoAPin    21
#define  RoBPin    22
#define  RoSPin    23

static volatile int globalCounter = 0 ;

unsigned char flag;
unsigned char Last_RoB_Status;
unsigned char Current_RoB_Status;

void rotaryDeal(void)
{
	Last_RoB_Status = digitalRead(RoBPin);
//	printf("read");
	while(!digitalRead(RoAPin)){
		Current_RoB_Status = digitalRead(RoBPin);
		flag = 1;
	}

	if(flag == 1){
		flag = 0;
		if((Last_RoB_Status == 0)&&(Current_RoB_Status == 1)){
			globalCounter ++;
			printf("globalCounter : %d\n",globalCounter);
		}
		if((Last_RoB_Status == 1)&&(Current_RoB_Status == 0)){
			globalCounter --;
			printf("globalCounter : %d\n",globalCounter);
		}

	}
}

void rotaryClear(void)
{
	if(digitalRead(RoSPin) == 0)
	{
		globalCounter = 0;
		printf("globalCounter : %d\n",globalCounter);
		delay(1000);
	}
}

int main(void)
{
	printf("main");
	if(wiringPiSetup() < 0){
		fprintf(stderr, "Unable to setup wiringPi:%s\n",strerror(errno));
		return 1;
	}
	printf("setting inputs");
	pinMode(RoAPin, INPUT);
	pinMode(RoBPin, INPUT);
	pinMode(RoSPin, INPUT);
	printf("Inputs setted");
	
	pullUpDnControl(RoSPin, PUD_UP);
	printf("setting pullup");
	while(1){
		//printf("Working");
		rotaryDeal();
		rotaryClear();
	}

	return 0;
}
