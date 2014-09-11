#define LPORT   1234
#define RPORT   1234

#define TIMEOUT 6000

#define MYMAC   02, 01, 02, 00, 00, 250
#define MYIP    "10.10.28.69"
#define MYMASK  "255.255.255.0"
#define DST_IP "10.10.28.255"

#include <string.h>
#include <io.h>

#include <dev/uartavr.h>

#include <sys/version.h>
#include <sys/thread.h>
#include <sys/timer.h>
#include <sys/heap.h>
#include <sys/confnet.h>
#include <sys/socket.h>

#include <arpa/inet.h>

#include <pro/httpd.h>
#include <pro/dhcp.h>

#ifdef NUTDEBUG
#include <sys/osdebug.h>
#include <net/netdebug.h>
#endif

#ifdef ETHERNUT2
#include <dev/lanc111.h>
#else
#include <dev/nicrtl.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <io.h>
#include <math.h>

static int running = 1;
static char inbuf[128];
UDPSOCKET *sock;
UDPSOCKET *sock2;
FILE *uart;
  
 
void receive()
{

	u_long addr;
    u_short port;
    u_char data[200];
    u_short  size;

    
    _write_P(_fileno(stdout), LP, sizeof(LP));
    
    
    
      size = NutUdpReceiveFrom(sock,&addr,&port,data,sizeof(data),100);
      if (size > 0) 
	  {
	printf("Got %d bytes from %s:%d\n",size,inet_ntoa(addr),port);
	printf("Message: %s\n",data);
	size = NutUdpSendTo(sock,addr,port,data,size);
      }
    
   

	
}


static char *readLine(char *buffer, FILE *uart){
   char *cp;
	fflush(uart);
	fgets(inbuf, sizeof(inbuf), uart);

    /*
     * Chop off trailing linefeed.
     */
    cp = strchr(inbuf, '\n');
    if (cp)
        *cp = 0;

	strncpy(buffer, inbuf, sizeof(inbuf));
	return buffer;
}


void send(void)
{
   
    u_short  size;
    

    
      fputs("\nEnter message: ", uart);
      readLine(inbuf,uart);
      fprintf(uart,"Sending to %s:%d\n---\n",DST_IP,RPORT);
     
      size = NutUdpSendTo(sock,inet_addr(DST_IP),RPORT,inbuf,strlen(data));
      if (size==0) {
	fprintf(uart,"Sending OK\n");
	
      }
      else {
	fprintf(uart,"ERROR: NutUdpSendTo=%d\n",size);

      }
   
    
    
  
}

THREAD(Thread1, arg)
{
	NutThreadSetPriority(32);
    /*
     * Create socket for sending
     */
	if ((sock = NutUdpCreateSocket(LPORT)) == 0) {
			printf("Cannot create udp socket!\n");
	}
		
	for(;;)	{
	if (running==1);
	send();	
	else
	NutSleep(1);
	}
}

THREAD(Thread2, arg)
{
	NutThreadSetPriority(33);
    /*
     * Create socket for receiving.
     */
	
	if ((sock2 = NutUdpCreateSocket(RPORT)) == 0) {
			printf("Cannot create udp socket!\n");
	}
	
	for(;;)	{
	if (running==1);
	receive();	
	else
	NutSleep(1);
	}
}


int main()
  
  {
  
     	
    
    /*
     * Initialize the uart device.
     */
	u_long baud = 115200;
   NutRegisterDevice(&devUart0, 0, 0);
   uart = fopen("uart0", "r+");
   _ioctl(_fileno(uart), UART_SETSPEED, &baud);

	/*
     * Register Ethernet controller.
     */
    if (NutRegisterDevice(&DEV_ETHER, 0x8300, 5))
      fprintf(uart,"Registering failed!");
	
      
	 /*
     * LAN configuration using fixed values.
     */
    u_char mac[] = { MYMAC };
    u_long ip_addr = inet_addr(MYIP);
    u_long ip_mask = inet_addr(MYMASK);
    NutNetIfConfig("eth0", mac, ip_addr, ip_mask);
    
    fprintf(uart,"ready\n");
   
	NutThreadCreate("t1", Thread1, 0, 6666);
	NutThreadCreate("t2", Thread2, 0, 6666); 
   
   
	fputs("\nWelcome\n\n", uart);
	send();
	
	
   
	
	return 0;
	
  }
