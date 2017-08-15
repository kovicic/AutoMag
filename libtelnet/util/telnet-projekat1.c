/*
 * Sean Middleditch
 * sean@sourcemud.org
 *
 * The author or authors of this code dedicate any and all copyright interest
 * in this code to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and successors. We
 * intend this dedication to be an overt act of relinquishment in perpetuity of
 * all present and future rights to this code under copyright law. 
 */

#if !defined(_POSIX_SOURCE)
#	define _POSIX_SOURCE
#endif
#if !defined(_BSD_SOURCE)
#	define _BSD_SOURCE
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <termios.h>
#include <unistd.h>
#include <wiringPi.h>
#include <lcd.h>
#include <ctype.h>
#include <limits.h>

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

		
static int fd;

#ifdef HAVE_ZLIB
#include "zlib.h"
#endif

#include "libtelnet.h"

static struct termios orig_tios;
static telnet_t *telnet;
static int do_echo;


static const telnet_telopt_t telopts[] = {
	{ TELNET_TELOPT_ECHO,		TELNET_WONT, TELNET_DO   },
	{ TELNET_TELOPT_TTYPE,		TELNET_WILL, TELNET_DONT },
	{ TELNET_TELOPT_COMPRESS2,	TELNET_WONT, TELNET_DO   },
	{ TELNET_TELOPT_MSSP,		TELNET_WONT, TELNET_DO   },
	{ -1, 0, 0 }
};

static void _cleanup(void) {
	tcsetattr(STDOUT_FILENO, TCSADRAIN, &orig_tios);
}



static void _input(char *buffer, int size) {
	static char crlf[] = { '\r', '\n' };
	int i;

	for (i = 0; i != size; ++i) {
		/* if we got a CR or LF, replace with CRLF
		 * NOTE that usually you'd get a CR in UNIX, but in raw
		 * mode we get LF instead (not sure why)
		 */
		if (buffer[i] == '\r' || buffer[i] == '\n') {
			if (do_echo)
				printf("\r\n");
			telnet_send(telnet, crlf, 2);
		} else {
			if (do_echo)
				putchar(buffer[i]);
			telnet_send(telnet, buffer + i, 1);
		}
	}
	fflush(stdout);
}

static void _send(int sock, const char *buffer, size_t size) {
	int rs;

	/* send data */
	while (size > 0) {
		if ((rs = send(sock, buffer, size, 0)) == -1) {
			fprintf(stderr, "send() failed: %s\n", strerror(errno));
			exit(1);
		} else if (rs == 0) {
			fprintf(stderr, "send() unexpectedly returned 0\n");
			exit(1);
		}

		/* update pointer and size to see if we've got more to send */
		buffer += rs;
		size -= rs;
	}
}

static void _event_handler(telnet_t *telnet, telnet_event_t *ev,
		void *user_data) {
	int sock = *(int*)user_data;

	switch (ev->type) {
	/* data received */
	case TELNET_EV_DATA:
		printf("%.*s", (int)ev->data.size, ev->data.buffer);
		fflush(stdout);
		break;
	/* data must be sent */
	case TELNET_EV_SEND:
		_send(sock, ev->data.buffer, ev->data.size);
		break;
	/* request to enable remote feature (or receipt) */
	case TELNET_EV_WILL:
		/* we'll agree to turn off our echo if server wants us to stop */
		if (ev->neg.telopt == TELNET_TELOPT_ECHO)
			do_echo = 0;
		break;
	/* notification of disabling remote feature (or receipt) */
	case TELNET_EV_WONT:
		if (ev->neg.telopt == TELNET_TELOPT_ECHO)
			do_echo = 1;
		break;
	/* request to enable local feature (or receipt) */
	case TELNET_EV_DO:
		break;
	/* demand to disable local feature (or receipt) */
	case TELNET_EV_DONT:
		break;
	/* respond to TTYPE commands */
	case TELNET_EV_TTYPE:
		/* respond with our terminal type, if requested */
		if (ev->ttype.cmd == TELNET_TTYPE_SEND) {
			telnet_ttype_is(telnet, getenv("TERM"));
		}
		break;
	/* respond to particular subnegotiations */
	case TELNET_EV_SUBNEGOTIATION:
		break;
	/* error */
	case TELNET_EV_ERROR:
		fprintf(stderr, "ERROR: %s\n", ev->error.msg);
		exit(1);
	default:
		/* ignore */
		break;
	}
}

void lcdScreen(int speed,int fuel,int temperature)
{	
	char buff[128]="";
	char val1 = 0xff;
	char val2 = 0xff;
	char val3 = 0xff;
	char val4 = 0xff;
	
	sleep(0.5);
	//setting fuel level indicators
	if(fuel == 6 || fuel == 5) 
	{
		val4 = '-';
	}
	else if(fuel == 4 || fuel == 3)
	{
		val3 = '-';
		val4 = '-';
	}
	else if(fuel == 2 || fuel == 1)
	{
		val2 = '-';
		val3 = '-';
		val4 = '-';
		
	}
	
	//printing on screen
	sprintf(buff,"Speed : %d km/h",speed);	
	lcdPosition(fd, 0, 0); 
	lcdPuts(fd, buff);
	
	sprintf(buff,"F:%c%c%c%c T:%d C%c  ",val1,val2,val3,val4,temperature,'\'');	
	lcdPosition(fd, 0, 1); 
	lcdPuts(fd, buff);
	
}

int htoi(char c)
{
	 int ret = 0;
	 
	 if('a'<= c && c <= 'f')
	 {
		ret = c - 'a' + 10;
	 }
	 else if('A' <= c && c <= 'F')
	 {
		ret = c - 'A' + 10;
	 }
	 else if('0' <= c && c <= '9')
	 {
		ret = c - '0';
	 }
	 
	 return ret;
}
void initLcd()
{
	
	wiringPiSetup();
	fd = lcdInit(2, 16, 4, LCD_RS, LCD_E, LCD_D0, LCD_D1, LCD_D2, LCD_D3, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
	lcdClear(fd);
	lcdPosition(fd, 0, 0); 
	lcdPuts(fd, "Speed : ");
	lcdPosition(fd, 0, 1); 
	lcdPuts(fd,"F:---- T:");
}
   
int main(int argc, char **argv) {
	
	char buffer[512];
	char recieveBuffer[512];
	char setBuffer[512];
	char dataBuffer[512];
	int br = 10000;
	int rs;
	int sock;
	struct sockaddr_in addr;
	struct pollfd pfd[2];
	struct addrinfo *ai;
	struct addrinfo hints;
	struct termios tios;
	
	int i;
	int j;
	int speed;
	int temperature;
	int greenLedState = 0;
	int redLedState = 0;
	int fuelLevel = 0;
	
	const char checkMsg[5] ="RX2";
	char *ret;

	//lcd init
	initLcd();

	/* check usage */
	if (argc != 3) {
		fprintf(stderr, "Usage:\n ./telnet-client <host> <port>\n");
		return 1;
	}

	/* look up server host */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	if ((rs = getaddrinfo(argv[1], argv[2], &hints, &ai)) != 0) {
		fprintf(stderr, "getaddrinfo() failed for %s: %s\n", argv[1],
				gai_strerror(rs));
		return 1;
	}
	
	/* create server socket */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		fprintf(stderr, "socket() failed: %s\n", strerror(errno));
		return 1;
	}

	/* bind server socket */
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		fprintf(stderr, "bind() failed: %s\n", strerror(errno));
		return 1;
	}

	/* connect */
	if (connect(sock, ai->ai_addr, ai->ai_addrlen) == -1) {
		fprintf(stderr, "connect() failed: %s\n", strerror(errno));
		return 1;
	}

	/* free address lookup info */
	freeaddrinfo(ai);

	/* get current terminal settings, set raw mode, make sure we
	 * register atexit handler to restore terminal settings
	 */
	tcgetattr(STDOUT_FILENO, &orig_tios);
	atexit(_cleanup);
	tios = orig_tios;
	cfmakeraw(&tios);
	tcsetattr(STDOUT_FILENO, TCSADRAIN, &tios);

	/* set input echoing on by default */
	do_echo = 1;

	/* initialize telnet box */
	telnet = telnet_init(telopts, _event_handler, 0, &sock);

	/* initialize poll descriptors */
	memset(pfd, 0, sizeof(pfd));
	pfd[0].fd = STDIN_FILENO;
	pfd[0].events = POLLIN;
	pfd[1].fd = sock;
	pfd[1].events = POLLIN;

	
	/* loop while both connections are open */
	while (poll(pfd, 2, -1) != -1) {

		/* read from stdin */
		if (pfd[0].revents & POLLIN) {

			if(rs = read(STDIN_FILENO, buffer, sizeof(buffer)) >0){
					
					if(strlen(buffer) >= 1)
					{
						printf("Choose s for CAN setup\r\n");
						printf("Choose l for LIN options\r\n");
						printf("Choose q for EXIT\r\n");
					switch(buffer[0])
					{
						if(strlen(buffer) >= 1){
						case 'l':

							printf("For lin open master1x press 1\r\n");
							printf("For lin open master2x press 2\r\n");
							printf("For lin open slave1x press 3\r\n");
							printf("For lin open slave2x press 4\r\n");
							printf("For lin open free mode press 5\r\n");
							printf("For lin close press 6\r\n");
							rs = read(STDIN_FILENO, buffer, sizeof(buffer));
							switch(buffer[0])
							{	
								case '1':							 
									printf("Please enter boudrate:");
									scanf("%d",&br);

									sprintf(buffer, "LIN OPEN MASTER1X %d\r\n", br);	
									_input(buffer,strlen(buffer));
									break;
								case '2':							 
									printf("Please enter boudrate:");
									scanf("%d",&br);
									sprintf(buffer, "LIN OPEN MASTER2X %d\r\n", br);	
									_input(buffer,strlen(buffer));
									break;
								case '3':							 
									printf("Please enter boudrate:");
									scanf("%d",&br);
									sprintf(buffer, "LIN OPEN SLAVE1X  %d\r\n", br);	
									_input(buffer,strlen(buffer));
									break;
								case '4':							 
									printf("Please enter boudrate:");
									scanf("%d",&br);
									sprintf(buffer, "LIN OPEN SLAVE2X %d\r\n", br);	
									_input(buffer,strlen(buffer));
									break;
								case '5':						
									printf("Please enter boudrate:");
									scanf("%d",&br);
									sprintf(buffer, "LIN OPEN FREE %d\r\n", br);	
									_input(buffer,strlen(buffer));
									break;
								case '6':						
									_input("LIN CLOSE\r\n", 10);
									bzero(buffer,512);
									break;

								default:
									//_input(buffer,rs);
									printf("There is no option for %c\n", buffer[0]);
									break;
							}		
							break;
						case 's':
							sprintf(setBuffer,"AT\r\n");
							_input(setBuffer,strlen(setBuffer));
							sprintf(setBuffer,"GPLED LED1 CLEAR\r\n");					
							_input(setBuffer,strlen(setBuffer));
							sprintf(setBuffer,"GPLED LED2 CLEAR\r\n");					
							_input(setBuffer,strlen(setBuffer));
							sprintf(setBuffer,"CAN USER ALIGN RIGHT\r\n");
							_input(setBuffer,strlen(setBuffer));
							sprintf(setBuffer,"CAN USER OPEN CH2 83k3\r\n");
							_input(setBuffer,strlen(setBuffer));
							sprintf(setBuffer,"CAN USER FILTER CH2 0 0010\r\n");
							_input(setBuffer,strlen(setBuffer));
							sprintf(setBuffer,"CAN USER MASK CH2 0010\r\n");
							_input(setBuffer,strlen(setBuffer));							
							
							break;
						case 'q':
							exit(1);
							break;
						default:
							_input(buffer,rs);
							printf("There is no option for %c please choose again\r\n",buffer[0]);
							break;
					}
				}
			}
			else {
			      	_input(buffer,rs);
			      }
			      _input(buffer,rs);
			} else if (rs == 0) {
				break;
			} else {
				fprintf(stderr, "recv(server) failed: %s\n",
						strerror(errno));
				exit(1);
			}
		}
		
		
		/* read from client */
		if (pfd[1].revents & POLLIN) {
			if ((rs = recv(sock, recieveBuffer, sizeof(recieveBuffer), 0)) > 0) {
				telnet_recv(telnet, recieveBuffer, rs);
				ret = strstr(recieveBuffer,checkMsg);	
				if(ret != NULL)
				{
					//write first part of data from telnet to dataBuffer
					for(i = 0;i < 8;i++)
					{							
						dataBuffer[i] = ret[i];						
					}	

					if((rs = recv(sock,recieveBuffer,sizeof(recieveBuffer),0))>0)
					{
						//write second part of data from telnet to dataBuffer
						for(i = 8;i< 16;i++)
						{
							dataBuffer[i] = recieveBuffer[i-8];	
						}
						if((rs = recv(sock,recieveBuffer,sizeof(recieveBuffer),0))>0)
					    {
							//write third part of data from telnet to dataBuffer
							for(i = 16 ;i< 24; i++)
							{
								dataBuffer[i] = recieveBuffer[i-16];	
							}
							//check part of ID
							if(dataBuffer[6] == 0x31 && dataBuffer[7] == 0x30)
							{	
								//fuel level
								fuelLevel = htoi(dataBuffer[18]);
								
								//temperature					    
								temperature = htoi(dataBuffer[22])+(htoi(dataBuffer[21])<<4);

								//speed
								speed = htoi(dataBuffer[10])+(htoi(dataBuffer[9])<<4);					    
								
								lcdScreen(speed,fuelLevel,temperature);
		

								if(speed <= 60 && greenLedState != 1)
								{								
									sprintf(setBuffer,"\r\n GPLED LED1 SET\n");				
									_input(setBuffer,strlen(setBuffer));
									greenLedState = 1;

									sprintf(setBuffer,"GPLED LED2 CLEAR\n");				
									_input(setBuffer,strlen(setBuffer));
									redLedState = 0;
								}
								else if(speed >60 && redLedState != 1)
								{
									sprintf(setBuffer,"GPLED LED2 SET\n");			
									_input(setBuffer,strlen(setBuffer));
									redLedState = 1;
		
									sprintf(setBuffer,"GPLED LED1 CLEAR\n");				
									_input(setBuffer,strlen(setBuffer));
									greenLedState = 0;
								}	
							}
						}
					}
				}
				
			} else if (rs == 0 ) {
				break;
			} else {
				
				fprintf(stderr, "recv(client) failed: %s\n",
						strerror(errno));
				exit(1);
			}
		}
	}

	/* clean up */
	telnet_free(telnet);
	close(sock);

	return 0;
}
