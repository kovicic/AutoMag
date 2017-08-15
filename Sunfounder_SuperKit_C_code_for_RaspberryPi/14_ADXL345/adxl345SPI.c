
#include <wiringPiI2C.h>
#include <wiringPiSPI.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#define  DevAddr  0x53  //device address

struct acc_dat{
	int x;
	int y;
	int z;
};

void adxl345_init(int fd)
{
	unsigned char buff[2];
	buff[0] = 0x31;
	buff[1] = 0x0b;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x2d;
	buff[1] = 0x08;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x2e;
	buff[1] = 0x00;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x1e;
	buff[1] = 0x00;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x1f;
	buff[1] = 0x00;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x20;
	buff[1] = 0x00;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x21;
	buff[1] = 0x00;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x22;
	buff[1] = 0x00;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x23;
	buff[1] = 0x00;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x24;
	buff[1] = 0x01;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x25;
	buff[1] = 0x0f;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x26;
	buff[1] = 0x2b;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x27;
	buff[1] = 0x00;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x28;
	buff[1] = 0x09;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x29;
	buff[1] = 0xff;
	wiringPiSPIDataRW(fd, buff, 2);

	buff[0] = 0x2a;
	buff[1] = 0x80;
	wiringPiSPIDataRW(fd, buff, 2); 

}

struct acc_dat adxl345_read_xyz(int fd)
{
	char x0, y0, z0, x1, y1, z1;
	struct acc_dat acc_xyz;
	unsigned char buffer[7];
	
	memset(buffer,'0',7);	
	
	buffer[0] = 0x32 | 0xC0;	
	wiringPiSPIDataRW(fd,buffer,7);
	

	x0 = 0xff - buffer[1];
	x1 = 0xff - buffer[2];
	y0 = 0xff - buffer[3];
	y1 = 0xff - buffer[4];
	z0 = 0xff - buffer[5];
	z1 = 0xff - buffer[6];

	acc_xyz.x = (int)(x1 << 8) + (int)x0;
	acc_xyz.y = (int)(y1 << 8) + (int)y0;
	acc_xyz.z = (int)(z1 << 8) + (int)z0;

	return acc_xyz;
}

int main(void)
{
	int fd;
	int channel = 0;
	struct acc_dat acc_xyz;

	
	fd = wiringPiSPISetupMode(channel,1000000,3); 
	
	if(-1 == fd){
		perror("SPI device setup error");	
	}

	adxl345_init(channel);

	while(1){
		acc_xyz = adxl345_read_xyz(channel);
		printf("x: %05d  y: %05d  z: %05d\n", acc_xyz.x, acc_xyz.y, acc_xyz.z);
		
		delay(1000);
	}
	
	return 0;
}



