/***********************************************
SNMP chat
Group 28
by: Jussi Pollari, Teemu Autio and Antti Seppälä
************************************************/

#define RPORT   161

#define TIMEOUT 6000

#define MYMAC   02, 01, 05, 03, 01, 250
#define BROADCAST_IP "10.10.255.255"
#define BUFFERSIZE  128

#include <string.h>
#include <io.h>
#include <ctype.h>

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
#include "SNMPchat.h"
#include "speechGeneration.h"

static char my_nick[20];
static char my_community[20];
static char MYIP[16];
static char MYMASK[16];
static char control[]="control";
static char inbuf2[BUFFERSIZE];
static char savemessage[BUFFERSIZE];
unsigned char RID_data[8];

int quit = 0;
int debug = 0;
int synthesize = 0;
int RID_length = 0;

u_long addr;

UDPSOCKET *sock;
FILE *uart;

 
/********************************************************************************/
/* This method reads the received SNMP message and interprets the content of it */
/********************************************************************************/

int SNMP_parser(unsigned char received_message[])
{
	int count;
	int idx;
	int PDU = 0;
	int OID = 0;
	int OID_length = 0;
	int error_length = 0;
	int community_length = 0;
	int no_errors=0;
	int community_OK =0;
	int control_OK =0;
	int OID_OK =0;
	int nickreceived = 0;
	
	char temp[4];
	char community_string[20];
		
	char *message;
	message = malloc(BUFFERSIZE);
	
	count=0;
	
	int tcount = 0, scount = 0, icnt=0;
	char tempword[10];
	char onewordy[10];
/* SNMP Message starts     */
/***************************/
	
	itoa(received_message[count], temp, 16);
	received_message[count] = strtol(temp, 0, 16 );
			
	if (received_message[count] == 48) /*Check the SNMP_HEADER */
	{
		count++;
		
		itoa(received_message[count], temp, 16); /* Check the length of the SNMP message */
		received_message[count] = strtol(temp, 0, 16 );
		
		count++;
		
		itoa(received_message[count], temp, 16); /* Check the type of SNMP_VER */
		received_message[count] = strtol(temp, 0, 16 );
		
		count++;
		itoa(received_message[count], temp, 16); /* Check the length of SNMP_VER */
		received_message[count] = strtol(temp, 0, 16 );
			
		count++;
		itoa(received_message[count], temp, 16); /* Check the SNMP VERSION */
		received_message[count] = strtol(temp, 0, 16 );
		if (received_message[count] == 0)
		{					

/* SNMP Community String   */
/***************************/
			count++;
			itoa(received_message[count], temp, 16); /* Check the type of SNMP_Community */
			received_message[count] = strtol(temp, 0, 16 );
			if (received_message[count] == 4)
			{
				
				count++;
				itoa(received_message[count], temp, 16); /* Check the length of SNMP_Community */
				received_message[count] = strtol(temp, 0, 16 );
				community_length = received_message[count];
				
				count++;
							
				for (idx=0; idx<community_length; idx++) /* Read the SNMP_Community String */
				{
					community_string[idx]=received_message[count]; 
					count++;
				}
				community_string[community_length] = '\0';
				
				/* Compare the community strings */
			
				if (strcmp(my_community, community_string)==0)
				{
					
					community_OK=1;
										
					if (debug == 1)
					{
						printf("\nSNMP_parser: Community String Type: Octet String");
						printf("\nSNMP_parser: Length of Community String: %d", community_length);
						printf("\nSNMP_parser: Community string: %s", community_string);
					}
				}

				else if (strcmp(control, community_string)==0)
				{
					
					control_OK=1;
					community_OK=control_OK;

					if (debug == 1)
					{
						printf("\nSNMP_parser: Community String Type: Octet String");
						printf("\nSNMP_parser: Length of Community String: %d", community_length);
						printf("\nSNMP_parser: Community string: %s", community_string);
					}
					
				}
							
					
				else
				{
					community_OK=0;
					control_OK =0;

					
				}

				if (community_OK==1)
				{
					if (debug == 1)
					printf("\nSNMP_parser: Correct Community String");
					
				}
			
				
			} // community string
			
			else
			{
				printf("\nSNMP_parser: Wrong Community String");
				return 0;
			}	
			

/* SNMP PDU                */
/***************************/
			
			itoa(received_message[count], temp, 16); /* Check the type of SNMP_PDU */
			received_message[count] = strtol(temp, 0, 16 );
			if (received_message[count] == 163)
			{
				PDU=1;
				if (debug == 1)
				printf("\nSNMP_parser: SNMP PDU Header: Set Request");
			}
			else if (received_message[count] == 162)
			{
				PDU=2;
				if (debug == 1)
				printf("\nSNMP_parser: SNMP PDU Header: Get Response");
			}
			else if (received_message[count] == 160)
			{
				PDU=3;
				if (debug == 1)
				printf("\nSNMP_parser: SNMP PDU Header: Get Request");
			}
			else if (received_message[count] == 164)
			{
				PDU=4;
				if (debug == 1)
				printf("\nSNMP_parser: SNMP PDU Header: TRAP");
			}
			else
				printf("\nSNMP_parser: Invalid PDU Header");
			
			
			count++;
			itoa(received_message[count], temp, 16); /* Check the length of SNMP_PDU */
			received_message[count] = strtol(temp, 0, 16 );
			
			if (debug == 1)
			printf("\nSNMP_parser: Length of SNMP_PDU: %d", received_message[count]);
						
			if (PDU == 4)
			{
				no_errors=1;
				OID_OK = 1;
				OID=4;
			}

			else {
					
	/* SNMP Request ID         */
	/***************************/				
			
				count++;
				itoa(received_message[count], temp, 16); /* Check the type of RequestID */
				received_message[count] = strtol(temp, 0, 16 );
			
				if (debug == 1)
				printf("\nSNMP_parser: SNMP RequestID type: Integer");
			
				count++;
				itoa(received_message[count], temp, 16); /* Check the length of RequestID */
				received_message[count] = strtol(temp, 0, 16 );
				RID_length = received_message[count];
			
				if (debug == 1)
				printf("\nSNMP_parser: SNMP RequestID length: %d", RID_length);
			
			
				count++;
			
				for (idx =0; idx < RID_length; idx++)
				{
				
					itoa(received_message[count], temp, 16); /* Check the data of RequestID */
					received_message[count] = strtol(temp, 0, 16 );
					RID_data[idx]=received_message[count];
				
					if (debug == 1)
					printf("\nSNMP_parser: SNMP RequestID data: %d", received_message[count]);
				
					count++;
				}
					
					

	/* SNMP Error              */
	/***************************/
			
				itoa(received_message[count], temp, 16); /* Check the Error type */
				received_message[count] = strtol(temp, 0, 16 );
			
				count++;
				itoa(received_message[count], temp, 16); /* Check the Error length */
				received_message[count] = strtol(temp, 0, 16 );
				error_length = received_message[count];
			
				if (debug == 1)
				printf("\nSNMP_parser: Error length: %i", error_length);		

			
				for (idx =0; idx < error_length; idx++)
				{
					count++;
					itoa(received_message[count], temp, 16); /* Check the Error value */
					received_message[count] = strtol(temp, 0, 16 );
							

					if(received_message[count]==0)
					{
						if (debug == 1)
						printf("\nSNMP_parser: No error occurred");
					
						no_errors=1;
					}
					else if(received_message[count]==1)
					{
						if (debug == 1)
						printf("\nSNMP_parser: Response message too large to transport");
					}
					else if(received_message[count]==2)
					{
						if (debug == 1)
						printf("\nSNMP_parser: Name of the requested object wasn't found");
					}
					else if(received_message[count]==3)
					{
						if (debug == 1)
						printf("\nSNMP_parser: Data type of the requested didn't match the data type in the SNMP agent");
					}
					else if(received_message[count]==4)
					{
						if (debug == 1)
						printf("\nSNMP_parser: SNMP manager wanted to set read-only parameter");
					}
					else if(received_message[count]==5)
					{
						if (debug == 1)
						printf("\nSNMP_parser: Something else went wrong");
					}
					else
					if (debug == 1){printf("\nSNMP_parser: Invalid error value");}
					
				
				}
			
			

	/* SNMP Error Index        */
	/***************************/	
			
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
						{
							if (debug == 1)
							printf("\nSNMP_parser: Error index no errors");
						}
						else
						if (debug == 1) {printf("\nSNMP_parser: Error in index %d", received_message[count]);}
					}
				}
				else
				if (debug == 1) {printf("\nSNMP_parser: Invalid Error Index Type");}
			


	/* Varbind List            */
	/***************************/

				count++;
				itoa(received_message[count], temp, 16); /* Check the Varbind list type */
				received_message[count] = strtol(temp, 0, 16 );
				if(received_message[count]==48)
				{
					count++;
					itoa(received_message[count], temp, 16); /* Check the Varbind list length */
					received_message[count] = strtol(temp, 0, 16 );
				
					if (debug == 1)
					printf("\nSNMP_parser: Length of Varbind list: %d", received_message[count]);
				}
				else
				if (debug == 1) {printf("\nSNMP_parser: Invalid Varbind List Type");}



	/* Varbind                 */
	/***************************/
			
				count++;
				itoa(received_message[count], temp, 16); /* Check the Varbind type */
				received_message[count] = strtol(temp, 0, 16 );
				if(received_message[count]==48)
				{
					count++;
					itoa(received_message[count], temp, 16); /* Check the Varbind length */
					received_message[count] = strtol(temp, 0, 16 );
				
					if (debug == 1)
					printf("\nSNMP_parser: Length of Varbind: %d", received_message[count]);
				}
				else
				if (debug == 1) {printf("\nSNMP_parser: Invalid Varbind Type");}
			
			

	/* OID                     */
	/***************************/

				count++;
				itoa(received_message[count], temp, 16); /* Check the OID type */
				received_message[count] = strtol(temp, 0, 16 );
				if(received_message[count]==6)
				{
					count++;
					itoa(received_message[count], temp, 16); /* Check the OID length */
					received_message[count] = strtol(temp, 0, 16 );
				
					if (debug == 1)
					printf("\nSNMP_parser: Length of OID: %d", received_message[count]);



	/* OID prefix              */
	/***************************/	
			
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
											OID_OK =1;
											if (debug == 1)
											printf("\nSNMP_parser: Correct OID prefix");
								
										}
										else
										if (debug == 1){printf("\nSNMP_parser: Wrong OID prefix");}
								
									}
									else
									if (debug == 1){printf("\nSNMP_parser: Wrong OID prefix");}
								
								}
								else
								if (debug == 1){printf("\nSNMP_parser: Wrong OID prefix");}
							}
							else
							if (debug == 1){printf("\nSNMP_parser: Wrong OID prefix");}
						}
						else
						if (debug == 1){printf("\nSNMP_parser: Wrong OID prefix");}
					}
					else
					if (debug == 1){printf("\nSNMP_parser: Wrong OID prefix");}
				
					count++;
					itoa(received_message[count], temp, 16); /* Check OID Identifier Value */
					received_message[count] = strtol(temp, 0, 16 );
					if(received_message[count]==1)
					{
						OID = 1;
						if (debug == 1)
						printf("\nSNMP_parser: OID Identifier: SEND Message");
					}
					else if(received_message[count]==2)
					{
						OID = 2;
						if (debug == 1)
						printf("\nSNMP_parser: OID Identifier: REQUEST Name");
					}
					else if(received_message[count]==3)
					{
						OID = 3;
						if (debug == 1)
						printf("\nSNMP_parser: OID Identifier: QUERY Channels");
					}
					else if(received_message[count]==4)
					{
						OID = 4;
						if (debug == 1)
						printf("\nSNMP_parser: OID Identifier: LEAVE Channel");
					}
					else
					{
					OID_OK=0;
					if (debug == 1){printf("\nSNMP_parser: Invalid OID identifier");}
					}



	/* OID Value               */
	/***************************/
				
					count++;
					itoa(received_message[count], temp, 16); /* Check the OID Value type */
					received_message[count] = strtol(temp, 0, 16 );
					if(received_message[count]==4)
					{
						count++;
						itoa(received_message[count], temp, 16); /* Check the OID Value length */
						received_message[count] = strtol(temp, 0, 16 );
						OID_length=received_message[count];
					
						if (debug == 1)
						printf("\nSNMP_parser: Length of OID Message: %d", OID_length);
					
						count++;
														
						if (debug == 1)
						printf("\nSNMP_parser: OID Message: ");
					
						for (idx=0; idx<OID_length; idx++) /* Read the OID Value String */
						{
							message[idx]=received_message[count]; 
							if (debug == 1)
							printf("%c", message[idx]);
							count++;
						
						}
						if (debug == 1)
						printf("\n");
						message[OID_length] = '\0';
					}
				
				}
				else
				if (debug == 1) {printf("\nSNMP_parser: Invalid OID Type\n");}
		
		} /*Not a TRAP-message else*/

		}
		else
		{
		if (debug==1)
		printf("\nSNMP_parser: Unsupported SNMP version");
		}
	}
	else
	{
	if (debug==1)
	printf("\nSNMP_parser: Not an SNMP message");
	}
	
/* SNMP message ends        */
/****************************/	
	


/* Checking the PDU header and OID value to choose the right action */ 
/********************************************************************/
	
	if (community_OK==1 && no_errors==1 && OID_OK == 1) /* If the message has wrong community string, the message is discarded */
	{
		if (PDU == 1 && OID == 1 && control_OK == 0) /* Message received, send GET-RESPONSE and request name if not known and print message */
		{
			strcpy(savemessage, message);
			savemessage[OID_length] = '\0';
			
			SendMessage("", 2);
			
			SendMessage("", 3);
			
			if (debug==1)			
			printf("\nSNMP_parser: IP: %s: ", inet_ntoa(addr));
			
			
		}
		
		else if (PDU == 2 && OID == 1) /* A Client has received the sent message and has replied with GET-RESPONSE */
		{
			if (debug==1)	
			printf("\n* Client replied *\n");
		}
		
		else if (PDU == 2 && OID == 2) /* A Client has sent us their nickname */
		{
			nickreceived=1;
			
			if (debug==1)
			printf("\nSNMP_parser: Nickname received: %s\n", message);

			if (synthesize==1)
			{
				
				for (icnt = 0; i <= strlen(savemessage); icnt++) {
					if (savemessage[icnt] == ' ' || i == strlen(savemessage)) {
					onewordy[lettercntr] = '\0';
					
					
						if (strlen(onewordy) > 9) {
						for(scount = 0; scount <= strlen(onewordy); scount++) {
							
							tempword[tcount] = onewordy[scount];
							tcount++;
							if (tcount == 8) {
								tempword[tcount] = '\0';
								speakSentence(tempword,debug);
								memset(tempword, 0, sizeof(tempword));
								tcount = 0;
								printf("\nword sent tcount == 8\n");
							}
							if (scount >= 8 && strlen(onewordy)-scount < 9) {
									for(tcount = 0; tcount <= strlen(onewordy)-scount; tcount++) {
										tempword[tcount] = onewordy[scount];
										scount++;
									}
									tcount++;
									tempword[tcount] = '\0';
									speakSentence(tempword,debug);
									//NutSleep(100);
									memset(tempword, 0, sizeof(tempword));
									memset(onewordy, 0, sizeof(onewordy));
									lettercntr = 0;
									break;
								}
							}
						}
						else speakSentence(onewordy,debug);
						//NutSleep(100);
					
					
					}
					else {
					onewordy[lettercntr] = savemessage[i];
					lettercntr++;
					}
				}
				
				
				
				speakSentence(savemessage,debug);
				//NutSleep(100);
				//speakSentence("sanoo\0", debug);
				/*
				printf("\nlause: %s\n", savemessage);
				if (strlen(savemessage) > 9) {
					for(scount = 0; scount <= strlen(savemessage); scount++) {
						printf("\ntcount: %d, scount: %d\n", tcount, scount);
						tempword[tcount] = savemessage[scount];
						tcount++;
						if (tcount == 8) {
							tempword[tcount] = '\0';
							speakSentence(tempword,debug);
							memset(tempword, 0, sizeof(tempword));
							tcount = 0;
							printf("\nword sent tcount == 8\n");
						}
						if (scount >= 8 && strlen(savemessage)-scount < 9) {
								for(tcount = 0; tcount <= strlen(savemessage)-scount; tcount++) {
									tempword[tcount] = savemessage[scount];
									scount++;
								}
								tcount++;
								tempword[tcount] = '\0';
								speakSentence(tempword,debug);
								memset(tempword, 0, sizeof(tempword));
								printf("\nword sent tcount > 9\n");
								break;
						}
					}
				}
				else speakSentence(savemessage,debug);
				//NutSleep(100);
				*/
			}		

			printf("\n%s: %s\n", message, savemessage);
			
		}
		
		else if (PDU == 3 && OID == 2) /* A Client wants to know our nickname so lets response */
		{
			SendMessage(my_nick, 4);
		}
		
		else if (PDU == 3 && OID == 3 && control_OK==1) /* A Client wants to know what channel we are on so lets response */
		{
			SendMessage(my_community, 6);
		}
		
		else if (PDU == 2 && OID == 3) /* A Client has sent us which channel they are on */
		{
			printf("\n-- %s is on channel: %s --\n",inet_ntoa(addr), message);
		}
		
		else if (PDU == 4 && OID == 4) /* A Client has sent us the TRAP message */
		{
			printf("** %s has left the channel %s\n", inet_ntoa(addr), my_community);
		}
		
		else
		{
		if (debug ==1)
		printf("\nSNMP_parser: Not a valid message\n");
		}
	}
	
	free(message);
	
	
	return 1;
}



int SendMessage(char message2[], int action)
{

	/*List of actions:
	1: send message to the channel
	2: message received, send GET-RESPONSE
	3: request name
	4: reply to name request
	5: query for channels
	6: reply to channel query
	*/
	
	
	int count2;
	int total_length = 0;
	int community_length2 = 0;
	int PDU_length =0;
	int varbind_length = 0;
	int varbind2_length = 0;
	int OID_message_length = 0;
	int idx2 = 0;
	char community[20];
	u_short  size2;
	
		
	if (action == 5 || action  == 6) /* The channel query must be sent to control channel */
	{
	strncpy(community, control, strlen(control));
	community[strlen(control)]='\0';
	}
	
	else
	{
	strncpy(community, my_community, strlen(my_community));
	community[strlen(my_community)]='\0';
	}

	if (action == 1 || action==3 || action==5)
	RID_length=1;
	
	unsigned char snmp_message[32 + strlen(message2) + strlen(community) + RID_length];
	
	total_length = 30 + strlen(message2) + strlen(community) + RID_length;
	PDU_length = total_length - (6 + strlen(community)+RID_length);
	varbind_length = PDU_length - 11;
	varbind2_length = varbind_length - 2;
	
	count2 = 0;
	
	

/* SNMP Message starts     */
/***************************/

	snmp_message[count2] = 0x30; /* SNMP_HEADER type=Sequence */
	count2++;
	snmp_message[count2] = total_length; /* SNMP_HEADER Length of message */
	
	count2++;
	snmp_message[count2] = 0x02; /* SNMP_VER type=Integer */
	count2++;
	snmp_message[count2] = 0x01; /* SNMP_VER Length */
	count2++;
	snmp_message[count2] = 0x00; /* SNMP_VER Version */
	count2++;
	
	snmp_message[count2] = 0x04; /* SNMP COMMUNITY type=OctetString */
	count2++;
	snmp_message[count2] = strlen(community); /* SNMP COMMUNITY length */
	community_length2 = snmp_message[count2];
	count2++;
	

	for (idx2 = 0; idx2<community_length2; idx2++)
	{
		snmp_message[count2]=community[idx2]; /* SNMP COMMUNITY string */
		count2++;
		
	}	



/* SNMP PDU                */
/***************************/

	if (action == 1)
	snmp_message[count2] = 0xA3; /* SNMP_PDU_HEADER SetRequest */
	else if (action == 2 || action == 4 || action == 6)
	snmp_message[count2] = 0xA2; /* SNMP_PDU_HEADER GetResponse */
	else if (action == 3 || action == 5)
	snmp_message[count2] = 0xA0; /* SNMP_PDU_HEADER GetRequest */
	else
	snmp_message[count2] = 0xA4; /* SNMP_PDU_HEADER TRAP */

	count2++;
	snmp_message[count2] = PDU_length; /* SNMP_PDU Length */
	
	
		
/* SNMP Request ID         */
/***************************/	
	
	count2++;
	snmp_message[count2] = 0x02; /* SNMP_RID type=Integer */

	if (action ==2 || action == 4 || action == 6)
	{
		count2++;
		snmp_message[count2] = RID_length; /* SNMP_RID Length */
		count2++;

		for (idx2 =0; idx2 < RID_length; idx2++)
				{
				
					snmp_message[count2] = RID_data[idx2];				
					if (debug == 1)
					printf("SNMP RequestID data: %d\n", snmp_message[count2]);
				
					count2++;
				}
		
	}

	else
	{	
	
		
		count2++;
		snmp_message[count2] = 0x01; /* SNMP_RID Length */
		count2++;
		snmp_message[count2] = rand() % 100 + 1; /* SNMP_RID data */
		count2++;
	}
	
	

/* SNMP Error              */
/***************************/
	
	
	snmp_message[count2] = 0x02; /* SNMP_ERR */
	count2++;
	snmp_message[count2] = 0x01; /* SNMP_ERR */
	count2++;
	snmp_message[count2] = 0x00; /* SNMP_ERR */



/* SNMP Error Index        */
/***************************/	
	
	count2++;
	snmp_message[count2] = 0x02; /* SNMP_ERR_INDEX */
	count2++;
	snmp_message[count2] = 0x01; /* SNMP_ERR_INDEX */
	count2++;
	snmp_message[count2] = 0x00; /* SNMP_ERR_INDEX */
	


/* Varbind List            */
/***************************/

	count2++;
	snmp_message[count2] = 0x30; /* Varbind List Header type=Sequence */
	count2++;
	snmp_message[count2] = varbind_length; /* Varbind List Length */
	


/* Varbind                 */
/***************************/

	count2++;
	snmp_message[count2] = 0x30; /* First Varbind header type=Sequence */
	count2++;
	snmp_message[count2] = varbind2_length; /* First Varbind Length */


/* OID                     */
/***************************/
	
	count2++;
	snmp_message[count2] = 0x06; /* Object ID type=Object Identifier */
	count2++;
	snmp_message[count2] = 0x07; /* Object ID Length */
	count2++;



/* OID prefix              */
/***************************/

	snmp_message[count2] = 0x2B; /* Object ID Prefix */
	count2++;
	snmp_message[count2] = 0x06; /* Object ID Prefix */
	count2++;
	snmp_message[count2] = 0x01; /* Object ID Prefix */
	count2++;
	snmp_message[count2] = 0x03; /* Object ID Prefix */
	count2++;
	snmp_message[count2] = 0x37; /* Object ID Prefix */
	count2++;
	snmp_message[count2] = 0x00; /* Object ID Prefix */
	count2++;
	
	

/* OID identifier          */
/***************************/

	if (action == 1 || action == 2)
	snmp_message[count2] = 0x01; /* OID Identifier: SEND message */
	else if (action == 3 || action == 4)
	snmp_message[count2] = 0x02; /* OID Identifier: REQUEST name */
	else if (action == 5 || action == 6)
	snmp_message[count2] = 0x03; /* OID Identifier: QUERY channels */
	else
	snmp_message[count2] = 0x04; /* OID Identifier: LEAVE channel */

	count2++;
	snmp_message[count2] = 0x04; /* OID Value type=OctetString */
	count2++;
	snmp_message[count2] = strlen(message2); /* OID Value Length */
	OID_message_length = snmp_message[count2];
	count2++;
	
	

/* OID value               */
/***************************/

	for (idx2=0; idx2<OID_message_length; idx2++)
	{
		snmp_message[count2]=message2[idx2]; /* OID Value Data = message */
		count2++;
		
	}	
	

/* End of SNMP message */		
/***********************/   
   
  
	if (debug == 1)
	{
	printf("\nSendMessage: SNMP COMMUNITY string: %s", community);
	
	if (action == 1)
	printf("\nSendMessage: SNMP PDU HEADER: SetRequest");
	else if (action == 2 || action == 4 || action == 6)
	printf("\nSendMessage: SNMP PDU HEADER: GetResponse");
	else if (action == 3 || action == 5)
	printf("\nSendMessage: SNMP PDU HEADER: GetRequest");
	else
	printf("\nSendMessage: SNMP PDU HEADER: TRAP");

	if (action == 1 || action == 2)
	printf("\nSendMessage: OID Identifier: SEND message");
	else if (action == 3 || action == 4)
	printf("\nSendMessage: OID Identifier: REQUEST name");
	else if (action == 5 || action == 6)
	printf("\nSendMessage: OID Identifier: QUERY channels");
	else
	printf("\nSendMessage: OID Identifier: LEAVE channel");
	
	printf("\nSendMessage: OID Value Data: %s",message2);
	
	}
 

/* Send the message according the chosen action */
/***********************************************/  

  
	if (action == 2 || action == 3 || action == 4 || action == 6)
	{
		
	   size2 = NutUdpSendTo(sock,addr,RPORT,snmp_message,sizeof(snmp_message));
	   
		  if (size2==0) {
				
				if (debug ==1)
				printf("\n* Sending to address: %s Port: %i *\n", inet_ntoa(addr), RPORT);
						
		  }
		  else {

				if (debug ==1)
				printf("\n* Sending to address: %s Port: %i *\n", inet_ntoa(addr), RPORT);

				printf("* ERROR: NutUdpSendTo=%d *\n",size2);
		  }
	}
	else
	{
	   
	   size2 = NutUdpSendTo(sock,inet_addr(BROADCAST_IP),RPORT,snmp_message,sizeof(snmp_message));
	   
		  if (size2==0) {
				
				if (debug ==1)
				printf("\n* Sending to address: %s Port: %i *\n", BROADCAST_IP, RPORT);
				
				printf("* Message sent *\n");
		
		  }
		  
			else {
				if (debug ==1)
				printf("\n* Sending to address: %s Port: %i *\n", BROADCAST_IP, RPORT);
				
				printf("* ERROR: NutUdpSendTo=%d *\n",size2);
			}
	}
    
    memset(community,0x0,sizeof(community));
	return 1;
}





int TRAPmessage(void)
{
				
	int count3 = 0;
	int idx3;
	int community_length3;
	int trap_length;
	int trap_PDU_length;
	u_short  size3;
		
	community_length3 = strlen(my_community);
	trap_length = 55 + community_length3;
	trap_PDU_length = trap_length - 5 -community_length3;

	unsigned char TRAP_message[trap_length+2];


/* SNMP Message starts     */
/***************************/

	TRAP_message[count3] = 0x30; /* SNMP_HEADER type=Sequence */
	count3++;
	TRAP_message[count3] = trap_length; /* SNMP_HEADER Length of message */
	
	count3++;
	TRAP_message[count3] = 0x02; /* SNMP_VER type=Integer */
	count3++;
	TRAP_message[count3] = 0x01; /* SNMP_VER Length */
	count3++;
	TRAP_message[count3] = 0x00; /* SNMP_VER Version */
	count3++;
	
	TRAP_message[count3] = 0x04; /* SNMP COMMUNITY type=OctetString */
	count3++;
	TRAP_message[count3] = community_length3; /* SNMP COMMUNITY length */
	count3++;
	

	for (idx3 = 0; idx3<community_length3; idx3++)
	{
		TRAP_message[count3]=my_community[idx3]; /* SNMP COMMUNITY string */
		count3++;
		
	}	


/* SNMP PDU                */
/***************************/

	TRAP_message[count3] = 0xA4; /* SNMP_PDU_HEADER TRAP */
	count3++;
	TRAP_message[count3] = trap_PDU_length; /* SNMP_PDU Length */
	count3++;


/* Enterprise              */
/***************************/
	
	TRAP_message[count3] = 0x06;
	count3++;
	TRAP_message[count3] = 0x08;
	count3++;
	TRAP_message[count3] = 0x2b;
	count3++;
	TRAP_message[count3] = 0x06;
	count3++;
	TRAP_message[count3] = 0x01;
	count3++;
	TRAP_message[count3] = 0x04;
	count3++;
	TRAP_message[count3] = 0x01;
	count3++;
	TRAP_message[count3] = 0x4d;
	count3++;
	TRAP_message[count3] = 0x01;
	count3++;
	TRAP_message[count3] = 0x01;
	count3++;


/* Agent-addr              */
/***************************/

	TRAP_message[count3] = 0x40;
	count3++;
	TRAP_message[count3] = 0x04;
	count3++;
	TRAP_message[count3] = 0x0a;
	count3++;
	TRAP_message[count3] = 0x0a;
	count3++;
	TRAP_message[count3] = 0x1c;
	count3++;
	TRAP_message[count3] = 0x45;
	count3++;


/* Generic-trap            */
/***************************/
	
	TRAP_message[count3] = 0x02;
	count3++;
	TRAP_message[count3] = 0x01;
	count3++;
	TRAP_message[count3] = 0x02;
	count3++;


/* Specific-trap           */
/***************************/
	
	TRAP_message[count3] = 0x02;
	count3++;
	TRAP_message[count3] = 0x01;
	count3++;
	TRAP_message[count3] = 0x00;
	count3++;


/* Time-stamp              */
/***************************/

	TRAP_message[count3] = 0x43;
	count3++;
	TRAP_message[count3] = 0x04;
	count3++;
	TRAP_message[count3] = 0x00;
	count3++;
	TRAP_message[count3] = 0x00;
	count3++;
	TRAP_message[count3] = 0x00;
	count3++;
	TRAP_message[count3] = 0x00;
	count3++;


/* OID                     */
/***************************/
	
	TRAP_message[count3] = 0x30;
	count3++;
	TRAP_message[count3] = 0x12;
	count3++;
	TRAP_message[count3] = 0x30;
	count3++;
	TRAP_message[count3] = 0x10;
	count3++;
	TRAP_message[count3] = 0x06;
	count3++;
	TRAP_message[count3] = 0x07;
	count3++;
	TRAP_message[count3] = 0x2b;
	count3++;
	TRAP_message[count3] = 0x06;
	count3++;
	TRAP_message[count3] = 0x01;
	count3++;
	TRAP_message[count3] = 0x03;
	count3++;
	TRAP_message[count3] = 0x37;
	count3++;
	TRAP_message[count3] = 0x00;
	count3++;
	TRAP_message[count3] = 0x04;
	count3++;


/* OID Value               */
/***************************/
	
	TRAP_message[count3] = 0x04;
	count3++;
	TRAP_message[count3] = 0x05;
	count3++;
	TRAP_message[count3] = 0x5c;
	count3++;
	TRAP_message[count3] = 0x51;
	count3++;
	TRAP_message[count3] = 0x55;
	count3++;
	TRAP_message[count3] = 0x49;
	count3++;
	TRAP_message[count3] = 0x54;	


	size3 = NutUdpSendTo(sock,inet_addr(BROADCAST_IP),RPORT,TRAP_message,sizeof(TRAP_message));
	   
	if (size3==0) {
		if (debug ==1)
				printf("\n* Sending to address: %s Port: %i *\n", BROADCAST_IP, RPORT);
		
		printf("* Message sent *\n");
		
		}

	else {

		if (debug ==1)
				printf("\n* Sending to address: %s Port: %i *\n", BROADCAST_IP, RPORT);

		printf("* ERROR: NutUdpSendTo=%d *\n",size3);
		  }


return 1;
}




/***********************************/ 
/* Function for reading user input */
/***********************************/

void userInput(char *str,uint8_t strLen) 
{
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





/***************************************/
/* Thread for listening the user input */
/***************************************/

THREAD(Thread1, arg)
{
	NutThreadSetPriority(62);
      
	int cnt;

	while(1)	
	{
		
		/*fputs("\nEnter message: ", uart);*/
		userInput(inbuf2,sizeof(inbuf2));
		
		if (strcmp(inbuf2, "/quit")==0)
		{
			TRAPmessage();
			quit=1;
		}
		
		else if (strcmp(inbuf2, "/query")==0)
		{	
			SendMessage("\\QUERY",5);
		}

		else if (strcmp(inbuf2, "/help")==0)
		{	
			printf("\n\nSNMP Chat Help\n");
			printf("**************\n");
			printf("Command\t\t\tExplanation\n\n");
			printf("/help\t\t\tThis help text\n");
			printf("/query\t\t\tTells what channels the other users are on\n");
			printf("/quit\t\t\tQuits chat client\n");
			printf("/nick <name>\t\tChanges the user's nickname\n");
			printf("/channel <name>\t\tChanges the channel that is listened\n");
			printf("/debug\t\t\tTurns debug ON/OFF\n");
			printf("/synthesize\t\tTurns speech synthesizer ON/OFF\n\n\n");
		
		}

		else if (strcmp(inbuf2, "/debug")==0)
		{	
			debug = 1-debug;
			printf("\n** Debug is now ");
			if (debug==0)
			printf("OFF\n");
			else
			printf("ON\n");
			
		}

		else if (strcmp(inbuf2, "/synthesize")==0)
		{	
			synthesize = 1-synthesize;
			printf("\n** Speech synthesizer is now ");
			if (synthesize==0)
			printf("OFF\n");
			else
			printf("ON\n");
			
		}

		else if (strncmp(inbuf2, "/channel", 8)==0)
		{	
			memset(my_community,0x0,sizeof(my_community));

			for (cnt=0; cnt<(strlen(inbuf2)-9); cnt++)
			{
				my_community[cnt]=inbuf2[cnt+9];
				my_community[cnt]=tolower(my_community[cnt]);
			}
			
			my_community[strlen(my_community)]='\0';
			printf("\n\nNow talking in channel: %s\n\n", my_community);
			
		}

		else if (strncmp(inbuf2, "/nick", 5)==0)
		{	
			memset(my_nick,0x0,sizeof(my_nick));
			
			for (cnt=0; cnt<(strlen(inbuf2)-6); cnt++)
			{
				my_nick[cnt]=inbuf2[cnt+6];
			}
			
			my_nick[strlen(my_nick)]='\0';
			printf("\n\nYour nickname is now: %s\n\n", my_nick);
			
		}
		
		else
		{
			if (inbuf2[0]!='/')
			SendMessage(inbuf2,1);  
		}
		
		if (quit==1)
		NutThreadExit();
		
		
	}
	
}



/***************************************/
/* Thread for listening the UDP socket */
/***************************************/

THREAD(Thread2, arg)
{
	NutThreadSetPriority(32);

	u_short port;
    u_char data[200];
    u_short  size;
	
      	
	while(1)	
	{
		size = NutUdpReceiveFrom(sock,&addr,&port,data,sizeof(data),100);
		if (size > 0) 
		{
			//SNMP_buffer(data, addr);
			SNMP_parser(data);
		}

		if (quit==1)
		NutThreadExit();
		
	}
	
	
}




/**************************************************/
/* The function that starts the SNMP chat program */
/**************************************************/

int SNMP(char nick[], char channel[], char ip[], char mask[], int d, int s)
  
  {
  
    quit=0;
	 strcpy(my_nick,nick);
	 strcpy(my_community,channel);
	 strcpy(MYIP,ip);	
	 strcpy(MYMASK,mask);
	 debug=d;
	 synthesize=s;		
    
    /*
     * Initialize the uart device.
     */
		 /*
	u_long baud = 115200;
   NutRegisterDevice(&devUart0, 0, 0);
   uart = fopen("uart0", "r+");
   _ioctl(_fileno(uart), UART_SETSPEED, &baud);
      */
	 /*
     * LAN configuration using fixed values.
     */
    u_char mac[] = { MYMAC };
    u_long ip_addr = inet_addr(MYIP);
    u_long ip_mask = inet_addr(MYMASK);
    NutNetIfConfig("eth0", mac, ip_addr, ip_mask);
    
    /*
		freopen("uart0", "w", stdout);
    _ioctl(_fileno(stdout), UART_SETSPEED, &baud);
	*/
	
	sock = NutUdpCreateSocket(RPORT);
	
	NutSleep(10);
    
	printf("\n\n\n\n\n\n\n\n\n\nWelcome to chat!\n\n");
	printf("Talking on channel: %s\n\n\n\n\n", my_community);
	
	NutThreadCreate("t1", Thread1, 0, 4096);
	NutThreadCreate("t2", Thread2, 0, 4096);
	
	NutSleep(10);
	
	
	while (quit==0){
		NutSleep(0);
	}
	NutSleep(10);
	NutUdpDestroySocket(sock); /* Lets destroy the socket after we are done with chatting */
	sock = 0;
	printf("\n\nBye bye!\n\n\n\n");
	
	
	return 1;
	
  }
