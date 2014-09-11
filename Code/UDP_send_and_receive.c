#define RPORT   161

#define TIMEOUT 6000

#define MYMAC   02, 01, 02, 00, 00, 250
#define MYIP    "10.10.28.69"
#define MYMASK  "255.255.0.0"
#define DST_IP "10.10.255.255"

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

static char my_nick[]="Tap";
static char my_community[]="sot";
static char control[]="CONTROL";
static char inbuf[128];
int flag=0;
UDPSOCKET *sock;
UDPSOCKET *sock2;
FILE *uart;
u_long addr;
  

int SNMP_parser(unsigned char received_message[])
{
	int count;
	int apucount;
	int idx;
	int control_value=0;
	int PDU;
	int OID;
	int OID_length;
	int RID_length;
	int error_length;
	int community_length;
	int no_errors=0;
	
	char temp[4];
	char community_string[20];
	char message[100];
	
	count=0;
	
	itoa(received_message[count], temp, 16);
	received_message[count] = strtol(temp, 0, 16 );
			
	if (received_message[count] == 48) /*Check the SNMP_HEADER */
	{
		printf("SNMP\n");
		count++;
		
		itoa(received_message[count], temp, 16); /* Check the length of the SNMPmessage */
		received_message[count] = strtol(temp, 0, 16 );
		printf("Length of the SNMP-message: %d\n", received_message[count]);
		count++;
		
		itoa(received_message[count], temp, 16); /* Check the type of SNMP_VER */
		received_message[count] = strtol(temp, 0, 16 );
		if (received_message[count] == 2)
			printf("Type of SNMP message: Integer\n");
		else
			printf("Not an SNMP messager");
		
		count++;
		itoa(received_message[count], temp, 16); /* Check the length of SNMP_VER */
		received_message[count] = strtol(temp, 0, 16 );
		printf("Length of VER: %d\n", received_message[count]);
		
		count++;
		itoa(received_message[count], temp, 16); /* Check the SNMP VERSION */
		received_message[count] = strtol(temp, 0, 16 );
		if (received_message[count] == 0)
			printf("SNMP Version 1\n");
		else
			printf("Unsupported SNMP version");
		
		count++;
		itoa(received_message[count], temp, 16); /* Check the type of SNMP_Community */
		received_message[count] = strtol(temp, 0, 16 );
		if (received_message[count] == 4)
		{
			printf("Community String Type: Octet String\n");
			count++;
			itoa(received_message[count], temp, 16); /* Check the length of SNMP_Community */
			received_message[count] = strtol(temp, 0, 16 );
			community_length = received_message[count];
			printf("Length of Community String: %d\n", community_length);
			
			count++;
			idx=0;
			apucount=count;
			
			for (; count<community_length+apucount; count++) /* Read the SNMP_Community String */
			{
				community_string[idx]=received_message[count]; 
				idx++;
			}
			printf("Community string: %s\n", community_string);
			
			if (strcmp(community_string, my_community) == 0) /* Check that the SNMP_Community String is correct */
				printf("Correct Community String\n");
			else if (strcmp(community_string, control) == 0)
			{
				printf("Community string is CONTROL\n");
				control_value=1;
			}
			else
			printf("Wrong Community, discard message");
		}
		
		else
			printf("Wrong Community String Type\n");
			
		itoa(received_message[count], temp, 16); /* Check the type of SNMP_PDU */
		received_message[count] = strtol(temp, 0, 16 );
		if (received_message[count] == 163)
		{
			PDU=1;
			printf("SNMP PDU Header: Set Request\n");
		}
		else if (received_message[count] == 162)
		{
			PDU=2;
			printf("SNMP PDU Header: Get Response\n");
		}
		else if (received_message[count] == 160)
		{
			PDU=3;
			printf("SNMP PDU Header: Get Request\n");
		}
		else if (received_message[count] == 164)
		{
			PDU=4;
			printf("SNMP PDU Header: TRAP\n");
		}
		else
			printf("Invalid PDU Header\n");
		
		
		count++;
		itoa(received_message[count], temp, 16); /* Check the length of SNMP_PDU */
		received_message[count] = strtol(temp, 0, 16 );
		printf("Length of SNMP_PDU: %d\n", received_message[count]);
		
		count++;
		itoa(received_message[count], temp, 16); /* Check the type of RequestID */
		received_message[count] = strtol(temp, 0, 16 );
		printf("SNMP RequestID type: Integer\n");
		
		count++;
		itoa(received_message[count], temp, 16); /* Check the length of RequestID */
		received_message[count] = strtol(temp, 0, 16 );
		printf("SNMP RequestID length: %d\n", received_message[count]);
		RID_length = received_message[count];
		
		for (idx =0; idx < RID_length; idx++)
		{
			count++;
			itoa(received_message[count], temp, 16); /* Check the data of RequestID */
			received_message[count] = strtol(temp, 0, 16 );
			printf("SNMP RequestID data: %d\n", received_message[count]);
		}
				
		count++;
		itoa(received_message[count], temp, 16); /* Check the Error type */
		received_message[count] = strtol(temp, 0, 16 );
		
		count++;
		itoa(received_message[count], temp, 16); /* Check the Error length */
		received_message[count] = strtol(temp, 0, 16 );
		error_length = received_message[count];
		
		for (idx =0; idx < error_length; idx++)
		{
			count++;
			itoa(received_message[count], temp, 16); /* Check the Error value */
			received_message[count] = strtol(temp, 0, 16 );
			
			if(received_message[count]==0)
			{
				printf("No error occurred\n");
				no_errors=1;
			}
			else if(received_message[count]==1)
			{
				printf("Response message too large to transport\n");
			}
			else if(received_message[count]==2)
			{
				printf("Name of the requested object wasn't found\n");
			}
			else if(received_message[count]==3)
			{
				printf("Data type of the requested didn't match the data type in the SNMP agent\n");
			}
			else if(received_message[count]==4)
			{
				printf("SNMP manager wanted to set read-only parameter\n");
			}
			else if(received_message[count]==5)
			{
				printf("Something else went wrong\n");
			}
			else
			printf("Invalid error value");
				
			
		}
		
			
		count++;
		itoa(received_message[count], temp, 16); /* Check the Error index type */
		received_message[count] = strtol(temp, 0, 16 );
		if(received_message[count]==2)
		{
			count++;
			itoa(received_message[count], temp, 16); /* Check the Error index length */
			received_message[count] = strtol(temp, 0, 16 );
			error_length=received_message[count];
			for (idx =0; idx < error_length; idx++)
			{
				count++;
				itoa(received_message[count], temp, 16); /* Check the Error index value */
				received_message[count] = strtol(temp, 0, 16 );
				if(received_message[count]==0)
					printf("No errors\n");
				else
				printf("Error in index %d\n", received_message[count]);
			}
		}
		else
		printf("Invalid Error Index Type\n");
		
		count++;
		itoa(received_message[count], temp, 16); /* Check the Varbind list type */
		received_message[count] = strtol(temp, 0, 16 );
		if(received_message[count]==48)
		{
			count++;
			itoa(received_message[count], temp, 16); /* Check the Varbind list length */
			received_message[count] = strtol(temp, 0, 16 );
			printf("Length of Varbind list: %d\n", received_message[count]);
		}
		else
		printf("Invalid Varbind List Type\n");
		
		count++;
		itoa(received_message[count], temp, 16); /* Check the Varbind type */
		received_message[count] = strtol(temp, 0, 16 );
		if(received_message[count]==48)
		{
			count++;
			itoa(received_message[count], temp, 16); /* Check the Varbind length */
			received_message[count] = strtol(temp, 0, 16 );
			printf("Length of Varbind: %d\n", received_message[count]);
		}
		else
		printf("Invalid Varbind Type\n");
		
		count++;
		itoa(received_message[count], temp, 16); /* Check the OID type */
		received_message[count] = strtol(temp, 0, 16 );
		if(received_message[count]==6)
		{
			count++;
			itoa(received_message[count], temp, 16); /* Check the OID length */
			received_message[count] = strtol(temp, 0, 16 );
			printf("Length of OID: %d\n", received_message[count]);
			
			count++;
			itoa(received_message[count], temp, 16); /* Check that the OID prefix is correct */
			received_message[count] = strtol(temp, 0, 16 );
			if(received_message[count]==43)
			{
				count++;
				itoa(received_message[count], temp, 16); /* Check that the OID prefix is correct */
				received_message[count] = strtol(temp, 0, 16 );
				if(received_message[count]==6)
				{
					count++;
					itoa(received_message[count], temp, 16); /* Check that the OID prefix is correct */
					received_message[count] = strtol(temp, 0, 16 );
					if(received_message[count]==1)
					{
						count++;
						itoa(received_message[count], temp, 16); /* Check that the OID prefix is correct */
						received_message[count] = strtol(temp, 0, 16 );
						if(received_message[count]==3)
						{
							count++;
							itoa(received_message[count], temp, 16); /* Check that the OID prefix is correct */
							received_message[count] = strtol(temp, 0, 16 );
							if(received_message[count]==55)
							{
								count++;
								itoa(received_message[count], temp, 16); /* Check that the OID prefix is correct */
								received_message[count] = strtol(temp, 0, 16 );
								if(received_message[count]==0)
								{
									printf("Correct OID prefix\n");
							
								}
								else
								printf("Wrong OID prefix\n");
							
							}
							else
							printf("Wrong OID prefix\n");
							
						}
						else
						printf("Wrong OID prefix\n");
					}
					else
					printf("Wrong OID prefix\n");
				}
				else
				printf("Wrong OID prefix\n");
			}
			else
			printf("Wrong OID prefix\n");
			
			count++;
			itoa(received_message[count], temp, 16); /* Check OID Identifier Value */
			received_message[count] = strtol(temp, 0, 16 );
			if(received_message[count]==1)
			{
				OID = 1;
				printf("OID Identifier: SEND Message\n");
			}
			else if(received_message[count]==2)
			{
				OID = 2;
				printf("OID Identifier: REQUEST Name\n");
			}
			else if(received_message[count]==3)
			{
				OID = 3;
				printf("OID Identifier: QUERY Channels\n");
			}
			else if(received_message[count]==4)
			{
				OID = 4;
				printf("OID Identifier: LEAVE Channel\n");
			}
			else
			printf("Invalid OID identifier\n");
			
			count++;
			itoa(received_message[count], temp, 16); /* Check the OID Value type */
			received_message[count] = strtol(temp, 0, 16 );
			if(received_message[count]==4)
			{
				count++;
				itoa(received_message[count], temp, 16); /* Check the OID Value length */
				received_message[count] = strtol(temp, 0, 16 );
				OID_length=received_message[count];
				printf("Length of OID Message: %d\n", OID_length);
				
				count++;
				idx=0;
				apucount=count;
				printf("OID Message: ");
				for (; count<OID_length+apucount; count++) /* Read the OID Value String */
				{
					message[idx]=received_message[count]; 
					printf("%c", message[idx]);
					idx++;
					
				}
				printf("\n");
				printf("%s", message);
			}
			
		}
		else
		printf("Invalid OID Type\n");
		
	}
	else
	printf("Not an SNMP message");
	
	if (PDU == 1 && OID == 1) /* Message received, send GET-RESPONSE and request name if not known and print message */
	{
		Send_message("0", 2);
	}
	
	else if (PDU == 2 && OID == 1) /* A Client has received the sent message and has replied with GET-RESPONSE */
	{
		printf("Client replied\n");
	}
	
	else if (PDU == 2 && OID == 2) /* A Client has sent us their nickname */
	{
		printf("Nickname received: %s\n", message);
	}
	
	else if (PDU == 3 && OID == 2) /* A Client wants to know our nickname so lets response */
	{
		Send_message(my_nick, 4);
	}
	
	else if (PDU == 3 && OID == 3) /* A Client wants to know what channel we are on so lets response */
	{
		Send_message(my_community, 6);
	}
	
	else if (PDU == 2 && OID == 3) /* A Client has sent us which channel they are on */
	{
		printf("User is on channel: %s\n", message);
	}
	
	else if (PDU == 4 && OID == 4) /* A Client has sent us which channel they are on */
	{
		printf("User has left the channel");
	}
	
	else
	printf("Not a valid message");
	
	return 1;
}



int Send_message(char message[100], int action)
{

	/*List of actions:
	1: send message to the channel
	2: message received, send GET-RESPONSE
	3: request name
	4: reply to name request
	5: query for channels
	6: reply to channel query
	7: leave the channel
	*/
	
	u_short  size;
	int count;
	int total_length;
	int PDU_length;
	int varbind_length;
	int varbind2_length;
	int idx;
	int apu;

	char total_length_hex[4];
	char community_length_hex[4];
	char PDU_length_hex[4];
	char varbind_length_hex[4];
	char varbind2_length_hex[4];
	char OID_length_hex[4];
	unsigned char snmp_message[31 + strlen(message) + strlen(my_community)];

	total_length = 31 + strlen(message) + strlen(my_community);

	PDU_length = total_length - (7 + strlen(my_community));
	varbind_length = PDU_length - 11;
	varbind2_length = varbind_length - 2;

	sprintf(total_length_hex, "%x", total_length);
	sprintf(PDU_length_hex, "%x", PDU_length);
	sprintf(varbind_length_hex, "%x", varbind_length);
	sprintf(varbind2_length_hex, "%x", varbind2_length);
	sprintf(community_length_hex, "%x", strlen(my_community)); /* length of the community string in hex */
	sprintf(OID_length_hex, "%x", strlen(message)); /* length of the message in hex */


	count = 0;
	snmp_message[count] = 0x30; /* SNMP_HEADER type=Sequence */
	count++;
	snmp_message[count] = total_length; /* SNMP_HEADER Length of message */
	count++;
	snmp_message[count] = 0x02; /* SNMP_VER type=Integer */
	count++;
	snmp_message[count] = 0x01; /* SNMP_VER Length */
	count++;
	snmp_message[count] = 0x00; /* SNMP_VER Version */
	count++;
	snmp_message[count] = 0x04; /* SNMP COMMUNITY type=OctetString */
	count++;
	snmp_message[count] = strlen(my_community); /* SNMP COMMUNITY length */
	count++;
	idx=0;
	apu=count;

	for (; count<strlen(my_community)+apu; count++)
	{
		snmp_message[count]=my_community[idx]; /* SNMP COMMUNITY string */
		idx++;
		
	}	

	if (action == 1)
	snmp_message[count] = 0xA3; /* SNMP_PDU_HEADER SetRequest */
	else if (action == 2 || action == 4 || action == 6)
	snmp_message[count] = 0xA2; /* SNMP_PDU_HEADER GetResponse */
	else if (action == 3 || action == 5)
	snmp_message[count] = 0xA0; /* SNMP_PDU_HEADER GetRequest */
	else
	snmp_message[count] = 0xA4; /* SNMP_PDU_HEADER TRAP */

	count++;
	snmp_message[count] = PDU_length; /* SNMP_PDU Length */
	count++;
	snmp_message[count] = 0x02; /* SNMP_RID type=Integer */
	count++;
	snmp_message[count] = 0x01; /* SNMP_RID Length */
	count++;
	snmp_message[count] = 0x01; /* SNMP_RID data */
	count++;
	snmp_message[count] = 0x02; /* SNMP_ERR */
	count++;
	snmp_message[count] = 0x01; /* SNMP_ERR */
	count++;
	snmp_message[count] = 0x00; /* SNMP_ERR */
	count++;
	snmp_message[count] = 0x02; /* SNMP_ERR_INDEX */
	count++;
	snmp_message[count] = 0x01; /* SNMP_ERR_INDEX */
	count++;
	snmp_message[count] = 0x00; /* SNMP_ERR_INDEX */
	count++;
	snmp_message[count] = 0x30; /* Varbind List Header type=Sequence */
	count++;
	snmp_message[count] = varbind_length; /* Varbind List Length */
	count++;
	snmp_message[count] = 0x30; /* First Varbind header type=Sequence */
	count++;
	snmp_message[count] = varbind2_length; /* First Varbind Length */
	count++;
	snmp_message[count] = 0x06; /* Object ID type=Object Identifier */
	count++;
	snmp_message[count] = 0x07; /* Object ID Length */
	count++;
	snmp_message[count] = 0x2B; /* Object ID Prefix */
	count++;
	snmp_message[count] = 0x06; /* Object ID Prefix */
	count++;
	snmp_message[count] = 0x01; /* Object ID Prefix */
	count++;
	snmp_message[count] = 0x03; /* Object ID Prefix */
	count++;
	snmp_message[count] = 0x37; /* Object ID Prefix */
	count++;
	snmp_message[count] = 0x00; /* Object ID Prefix */
	count++;

	if (action == 1 || action == 2)
	snmp_message[count] = 0x01; /* OID Identifier: SEND message */
	else if (action == 3 || action == 4)
	snmp_message[count] = 0x02; /* OID Identifier: REQUEST name */
	else if (action == 5 || action == 6)
	snmp_message[count] = 0x03; /* OID Identifier: QUERY channels */
	else
	snmp_message[count] = 0x04; /* OID Identifier: LEAVE channel */

	count++;
	snmp_message[count] = 0x04; /* OID Value type=OctetString */
	count++;
	snmp_message[count] = strlen(message); /* OID Value Length */
	count++;
	
	idx=0;
	apu = count;
	
	for (; count<strlen(message)+apu; count++)
	{
		snmp_message[count]=message[idx]; /* OID Value Data = message */
		idx++;
		
	}	
	
		
	SNMP_parser(snmp_message);
   
   
   /* Send the message */
  
	if (action == 2 || action == 3 || action == 4 || action == 6)
	{
		flag=1;
	   size = NutUdpSendTo(sock,addr,RPORT,snmp_message,sizeof(snmp_message));
	   flag=0;
		  if (size==0) {
		fprintf(uart,"Sending OK\n");
		
		  }
		  else {
		fprintf(uart,"ERROR: NutUdpSendTo=%d\n",size);

		  }
	}
	else
	{
	   flag=1;
	   size = NutUdpSendTo(sock,inet_addr(DST_IP),RPORT,snmp_message,sizeof(snmp_message));
	   flag=0;
		  if (size==0) {
		fprintf(uart,"Sending OK\n");
		
		  }
		  else {
		fprintf(uart,"ERROR: NutUdpSendTo=%d\n",size);

		  }
	}
        
	return 1;
}


 
void receive()
{

	
    u_short port;
    u_char data[200];
    u_short  size;

    
    
    fprintf(uart,"%d\n",RPORT);
    
    while (1) {
      if (flag == 1)
	  NutSleep(1);
	  
	  size = NutUdpReceiveFrom(sock,&addr,&port,data,sizeof(data),100);
      if (size > 0) 
	  {
	SNMP_parser(data);
   /*fprintf(uart,"Got %d bytes from %s:%d\n",size,inet_ntoa(addr),port);
	fprintf(uart,"Message: %s\n",data);
   */
	
      }
    }
    

	
}


void readLine(char *str,uint8_t strLen) {
  char ch;
  uint8_t pos=0;
  do {
    if (_read(_fileno(uart), &ch, sizeof(ch)) > 0) {
      _write(_fileno(uart), &ch, sizeof(ch));
      if (ch=='\n' || pos>=strLen-1) {
	ch = 0x00;
      }
      str[pos]=ch;
      pos++;
    }
  } while (ch != 0x00);
}



void send(void)
{
   
    

    while (1) {
      fputs("\nEnter message: ", uart);
      readLine(inbuf,sizeof(inbuf));
      Send_message(inbuf,1);
     
      
   
    }
    
  
}

THREAD(Thread1, arg)
{
	NutThreadSetPriority(62);
      
    /*
     * Create socket for sending
     */
			
	while(1)	{
	
	send();
	
	}
}

THREAD(Thread2, arg)
{
	NutThreadSetPriority(32);
      
    /*
     * Create socket for sending
     */
			
	while(1)	{
	
	receive();
	
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
    
    freopen("uart0", "w", stdout);
    _ioctl(_fileno(stdout), UART_SETSPEED, &baud);

    fprintf(uart,"ready\n");
   
  
   sock = NutUdpCreateSocket(RPORT);
   
	NutThreadCreate("t1", Thread1, 0, 4096);
	NutThreadCreate("t2", Thread2, 0, 4096);
   
   /*
   int userchoice;
   fputs("\nChoose mode 1 or 2: ", uart);
	readLine(inbuf, uart);
	fputs("\n", uart);
	userchoice = atoi(inbuf);
	if (userchoice==1)
	send();
	else
	receive();
	*/
   readLine(inbuf,sizeof(inbuf));
   NutUdpDestroySocket(sock); /* Useless, after an endless loop */
	
	return 0;
	
  }
