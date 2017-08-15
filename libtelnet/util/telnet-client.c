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

int main(int argc, char **argv) {
	char buffer[512];
	char buffer1[512];
	char buffer2[512];
	char buffer3[512];
	int boudrate = 10000;
	char boudrateString[20];
	int rs;
	int rs1;
	int sock;
	struct sockaddr_in addr;
	struct pollfd pfd[2];
	struct addrinfo *ai;
	struct addrinfo hints;
	struct termios tios;

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

	
	printf("Enter new boudrate\n");
	char str1[30];
	char c;
	/*
	do {
		if(!fgets(str1,30,stdin))
		{
			break;
		}
		printf("%s", str1);
		size_t len = strlen(str1);
	}while(str1[0] != '\n' || str1[len-1] !='\n');
	*/
	while(c = getchar())
	{
		if(c== '\n')
			break;
		else
			printf("string is : %c\n",c);
	}


	printf(" New boudrate is %s\n",str1);




	/*
	while (poll(pfd, 2, -1) != -1) 
	{
		// read from stdin 
		if (pfd[0].revents & POLLIN) 
		{	
			fgets(buffer1, sizeof(buffer1), STDIN_FILENO) ;
			printf("buffer value is:%s\n",buffer1);
				size_t len = strlen(buffer1);
				if(len>0 && buffer[0] == '\n')
				{
					printf("pressed Enter");
					

				}

		

				
			/*	
			while ((rs1 = read(STDIN_FILENO, buffer1, sizeof(buffer1))) > 0) 
			{

				size_t len = strlen(buffer1);
				if(len>0 && buffer[len-1] == '\0')
				{
					printf("pressed Enter");
					

				}
				
				if (rs1<4)
				{
					//bzero(buffer,512);
					switch(buffer1[0])
					{
						case 'q':
         						exit(1);
							break;
						default:
						//	boudrate = buffer1[0] - '0';				
						//	printf("New boudrate is:%d\n", boudrate);
							printf("LEN is:%d\n", len);
							break;
					}

				}
				
			
					      
		      	    else
				{
					 _input(buffer1, rs1);
				}
			      
			 }     	
			//_input(buffer1, rs1);
			     	
		//	} else if (rs1 == 0) {
		//			break;
		//	} else 
		//	{
		//		fprintf(stderr, "recv(server) failed: %s\n",
		//				strerror(errno));
		//		exit(1);
		//	}		
		}
	}
	*/

	printf("Choose l for LIN options\n\n");
	printf("Choose q for EXIT\n\n");
	/* loop while both connections are open */
	while (poll(pfd, 2, -1) != -1) {
		/* read from stdin */
		if (pfd[0].revents & POLLIN) {
			if ((rs = read(STDIN_FILENO, buffer, sizeof(buffer))) > 0) {
			    if(rs < 2){	 
			   
					switch(buffer[0])
					{
						case 'l':
							bzero(buffer,512);
							printf("For lin open master1x press 1\r\n");
							printf("For lin open master2x press 2\r\n");
							printf("For lin open slave1x press 3\r\n");
							printf("For lin open slave2x press 4\r\n");
							printf("For lin open free press 5\r\n");
							printf("For lin close press 6\r\n");
							printf("For set boudrate press 7\r\n");
							rs = read(STDIN_FILENO, buffer, sizeof(buffer));
							switch(buffer[0])
							{	
								case '1':							 
									sprintf(buffer, "LIN OPEN MASTER1X %d\r\n", boudrate);
									
									_input(buffer,strlen(buffer));
									bzero(buffer,512);
									break;
								case '2':							 
									sprintf(buffer, "LIN OPEN MASTER2X %d\r\n", boudrate);	
									
									_input(buffer,strlen(buffer));
									bzero(buffer,512);
									break;
								case '3':							 
									sprintf(buffer, "LIN OPEN SLAVE1X %d\r\n", boudrate);	
									
									_input(buffer,strlen(buffer));
									bzero(buffer,512);
									break;
								case '4':							 
									sprintf(buffer, "LIN OPEN SLAVE2X %d\r\n", boudrate);	
									
									_input(buffer,strlen(buffer));
									bzero(buffer,512);
									break;
								case '5':							 
									sprintf(buffer, "LIN OPEN FREE %d\r\n", boudrate);	
									
									_input(buffer,strlen(buffer));
									bzero(buffer,512);
									break;
								case '6':						
									_input("LIN CLOSE\r\n", 10);
									bzero(buffer,512);
									break;
								case '7':
									printf("Enter new boudrate\n");
									//call function to set boudrate
									/*while(rs1 = read(STDIN_FILENO, buffer1, sizeof(buffer1))<5)
									{
										
										if(rs1 = read(STDIN_FILENO, buffer1, sizeof(buffer1))>3)
										{
											break;
										}
										printf("RS is%d\n", rs1); 
										_input(buffer1, rs1);
									}*/
									boudrate = buffer1[0] - '0';
									printf("boudrate set to %c\n", buffer1[0]);
									break;
								default:
									printf("There is no option for %c\n", buffer1[0]);
								
							}		
							break;
						case 'q':
							exit(1);
							break;
							default:
							printf("There is no option for %c\n",buffer[0]);
							
					}
			    }      
			    else
				{
				 _input(buffer, rs);
				}
			      
			      		
			//	_input(buffer, rs);
			      	
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
			if ((rs = recv(sock, buffer, sizeof(buffer), 0)) > 0) {
				telnet_recv(telnet, buffer, rs);
			} else if (rs == 0) {
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
