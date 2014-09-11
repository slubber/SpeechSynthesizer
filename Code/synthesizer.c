/* * Synthesizer greatest hits vol. 2
 *
 * The ULTIMATE speech synthesizer
 *
 * * SOP Group 28
 *   Teemu Autio, Jussi Pollari, Antti Seppälä
 * 
 */

//#include <cfg/crt.h>	/* Floating point configuration. */
#include <cfg/os.h>

//#include <dev/debugonoff.h>
#include <dev/board.h>
#include <dev/urom.h>
//#include <dev/irqreg.h>
//#include <dev/uartavr.h>

#include <sys/device.h>
#include <sys/timer.h>
//#include <sys/thread.h>
//#include <sys/version.h>
//#include <sys/event.h>
//#include <sys/heap.h>
//#include <sys/confnet.h>
//#include <sys/socket.h>

//#include <arch/avr.h>

//#include <arpa/inet.h>

//#include <pro/httpd.h>
//#include <pro/dhcp.h>
//#include <pro/ssi.h>
//#include <pro/asp.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
//#include <fcntl.h>

#include "synthesizer.h"
#include "hd44780.h"
#include "speechGeneration.h"
#include "SNMPchat.h"
//#include "utils.h"

/* Global variables */
static char *banner = "\nWelcome to Synthesizer Greatest Hits vol. 2\n";
const prog_char presskey_P[] = "Press any key...";
static char myip[20] = "10.10.28.12";
static char mysubnetmask[20] = "255.255.0.0";
static char mynick[20] = "IIO";
static char mychannel[20] = "oma";
static int synthonoff = 0;
static int debugonoff = 0;

static char inbuf[128];

FILE *uart;
/*
 * main() of synthesizer
 */
int main(void)
{
	int got;
	int userchoice;
	
	init_devices();
	if (debugonoff) {
		printf("\nsynthesizer: Devices initialized!\n");
		NutSleep(100);
	}	
	/*
	* Stream devices can use low level read and write functions.
	* Writing program space data is supported too.
	*/
	_write(_fileno(uart), banner, strlen(banner));
	{
		lcdPrint(banner);
		
		_write_P(_fileno(uart), presskey_P, sizeof(presskey_P));
	}

	/*
	* Stream devices do buffered I/O. That means, nothing will be 
	* passed to the hardware device until either the output buffer 
	* is full or we do a flush. With stream I/O we typically use
	* fflush(), but low level writing a null pointer will also flush 
	* the output buffer.
	*/
	_write(_fileno(uart), 0, 0);

	/*
	* The low level function read() will grab all available bytes 
	* from the input buffer. If the buffer is empty, the call will
	* block until something is available for reading.
	*/
	got = _read(_fileno(uart), inbuf, sizeof(inbuf));
	_write(_fileno(uart), inbuf, got);
	
	/*
	* Nut/OS never expects a thread to return. So we enter an 
	* endless loop here.
	*/
	do {
		fflush(uart);
		lcdPrint("Main Menu");
		userchoice = 0;

		print_main_menu();
		readLine(inbuf, sizeof(inbuf));			
		/*
		* Streams support formatted output as well as printing strings 
		* from program space.
		*/
		if (inbuf[0]) {
			puts("\n\n");
			userchoice = atoi(inbuf);
			light_led(userchoice);
			switch (userchoice)
			{
				case 1: speechSynthesize();
						break;
				case 2: runSNMP();
						break;
				case 3: openSettings();
						break;
				case 4: showHelp();
						break;
				case 5: getDiphones();
						break;
				default: puts("Invalid option selected\n");
			}
		}
	}
	while (1);
}

static void speechSynthesize(void) {
	int taajuus;
	int userchoice = 0;
	char speechSentence[128];

	lcdPrint("Generate speech");
	if (debugonoff) {
		printf("\nsynthesizer: Printing Generate speech-menu!\n");
		NutSleep(100);
	}
	do {
		fflush(uart);
		puts("\n\n\n\n\n");
		printf("\n*******************");
		printf("\n* Generate speech *");
		printf("\n*******************\n\n");
		printf("\n1. Text to speech\n");
		printf("\n2. Return to Main Menu\n");
		printf("\n*3. Buzzer*\n\n");
		printf("\nEnter your choice: ");
		readLine(inbuf, sizeof(inbuf));
	
		if (inbuf[0]) {
			puts("\n\n");
			userchoice = atoi(inbuf);
			
			switch (userchoice)
			{
				case 1: printf("\nEnter sentence you want to hear: ");
						readLine(speechSentence, sizeof(speechSentence));
						speakSentence(speechSentence, debugonoff);
						//changeChar(speechSentence, strlen(speechSentence));
						printf("\nCan you hear: %s\n", speechSentence);
						break;
				case 2: puts("\nReturning to Main Menu\n");
						puts("\n\n\n\n\n");
						break;
				case 3: printf("\nInsert normalized frequency value: ");
						readLine(speechSentence, sizeof(speechSentence));
						taajuus=atoi(speechSentence);
						printf("\nBuzzing at %s.\n", speechSentence);
						buzzer(taajuus);
						break;
				default: puts("\nInvalid option selected\n");
			}
		}
	}
	while (userchoice != 2);
}

static void runSNMP(void) {
	int userchoice = 0;

	lcdPrint("SNMP Chat");
	light_led(2);
	if (debugonoff) {
		printf("\nsynthesizer: Printing SNMP Chat-menu!\n");
		NutSleep(100);
	}
	do {
		fflush(uart);
		puts("\n\n\n\n\n");
		printf("\n*************");
		printf("\n* SNMP Chat *");
		printf("\n*************\n\n");
		printf("\n1. Nickname: %s\n", mynick);
		printf("\n2. Channel: %s\n", mychannel);
		printf("\n3. Start chatting\n");
		printf("\n4. Return to Main Menu\n\n");
		printf("\nEnter your choice: ");

		readLine(inbuf, sizeof(inbuf));
	
		if (inbuf[0]) {
			puts("\n\n");
			userchoice = atoi(inbuf);
			switch (userchoice)
			{
				case 1: printf("\nEnter your nickname: ");
						readLine(mynick, sizeof(mynick));
						break;
				case 2: printf("\nEnter channel  : ");
						readLine(mychannel, sizeof(mychannel));
						break;
				case 3: puts("\nStart Chatting!\n");
						SNMP(mynick, mychannel, myip, mysubnetmask, debugonoff, synthonoff);
						break;
				case 4: puts("\nReturning to Main Menu\n");
						puts("\n\n\n\n\n");
						break;
				default: puts("\nInvalid option selected\n");
			}
		}
	}
	while (userchoice != 4);
}

static void openSettings(void) {
	int userchoice = 0;

	lcdPrint("Settings");
	if (debugonoff) {
		printf("\nsynthesizer: Printing Settings-menu!\n");
		NutSleep(100);
	}
	do {
		fflush(uart);
		puts("\n\n\n\n\n");
		printf("\n************");
		printf("\n* Settings *");
		printf("\n************\n\n");
		printf("\n1. Network settings\n");
		if (synthonoff == 0) printf("\n2. Synthesizer: OFF\n");
		else printf("\n2. Synthesizer: ON\n");
		if (debugonoff == 0) printf("\n3. Debug: OFF\n");
		else printf("\n3. Debug: ON\n");
		printf("\n4. Return to Main Menu\n\n");
		printf("\nEnter your choice: ");
		readLine(inbuf, sizeof(inbuf));
	
		if (inbuf[0]) {
			puts("\n\n");
			userchoice = atoi(inbuf);
			switch (userchoice)
			{
				case 1: openNetworkSettings();
						break;
				case 2: synthonoff = abs(1 - synthonoff);
						break;
				case 3: debugonoff = abs(1 - debugonoff);
						break;
				case 4: puts("\nReturning to Main Menu\n");
						puts("\n\n\n\n\n");
						break;
				default: puts("\nInvalid option selected\n");
			}
		}
	}
	while (userchoice != 4);
}

static void showHelp(void) {
	lcdPrint("Help");
	if (debugonoff) {
		printf("\nsynthesizer: Printing HELP!\n");
		NutSleep(100);
	}
	puts("\n\n\n\n\n");
	printf("\n********");
	printf("\n* Help *");
	printf("\n********\n");
	printf("\nSpeech synthesizing\n");
	printf("\nThe speech synthesizer needs the user to input a word or a sentence. ");
	printf("\nAfter writing the word or the sentence press enter and listen to the speech.");
	printf("\nThere isn't any other functionalities in the speech synthesizing part yet.\n");
	printf("\n\nSNMP Chat\n\n");
	printf("Command\t\t\tExplanation\n\n");
	printf("/help\t\t\tThis help text\n");
	printf("/query\t\t\tTells what channels the other users are on\n");
	printf("/quit\t\t\tQuits chat client\n");
	printf("/nick <name>\t\tChanges the user's nickname\n");
	printf("/channel <name>\t\tChanges the channel that is listened\n");
	printf("/debug\t\t\tTurns debug ON/OFF\n");
	printf("/synthesize\t\tTurns speech synthesizer ON/OFF\n\n\n");
	printf("\nPress any key to continue...");
	fflush(uart);
	fgetc(uart);
}

static void openNetworkSettings(void) {
	int userchoice = 0;
	lcdPrint("Network settings");
	if (debugonoff) {
		printf("\nsynthesizer: Printing Network settings-menu!\n");
		NutSleep(100);
	}
	do {
		fflush(uart);
		puts("\n\n\n\n\n");
		printf("\n********************");
		printf("\n* Network settings *");
		printf("\n********************\n\n");
		printf("\n1. IP: %s\n", myip);
		printf("\n2. Subnet mask: %s\n", mysubnetmask);
		printf("\n3. Return to Settings Menu\n\n");
		printf("\nEnter your choice: ");
		readLine(inbuf, sizeof(inbuf));
	
		if (inbuf[0]) {
			fputs("\n\n", uart);
			userchoice = atoi(inbuf);
			switch (userchoice)
			{
				case 1: printf("\nEnter new IP address: ");
						readLine(myip, sizeof(myip));
						break;
				case 2: printf("\nEnter new subnet mask: ");
						readLine(mysubnetmask, sizeof(mysubnetmask));
						break;
				case 3: puts("\nReturning to Settings Menu\n");
						puts("\n\n\n\n\n");
						break;
				default: puts("\nInvalid option selected\n");
			}
		}
	}
	while (userchoice != 3);
}

void readLine(char *str,uint8_t strLen) {
	char ch;
	uint8_t pos=0;
	fflush(uart);

/*
	do {
		if (_read(_fileno(uart), &ch, sizeof(ch)) > 0)
		{
			_write(_fileno(uart), &ch, sizeof(ch));
			if (ch=='\n' || pos>=strLen-1) {
				ch = 0x00;
			}
			// Bubblegum fix for backspace 
			if (ch==0x08 && pos > 0) {
				ch=0x20;
				_write(_fileno(uart), &ch, sizeof(ch));
				ch=0x08;
				_write(_fileno(uart), &ch, sizeof(ch));
				pos--;
				str[pos]=0x00;
			}
			else {
				str[pos]=ch;
				pos++;
			}
		}
	}
*/
	do 
	{
		if (_read(_fileno(uart), &ch, sizeof(ch)) > 0)
		{
			/* Bubblegum fix for backspace */
			if (ch == 0x08) 
			{
				str[pos]=0x00;
				if (pos > 0) 
				{
					ch=0x08;
					_write(_fileno(uart), &ch, sizeof(ch));
					pos--;
					str[pos]=0x00;
				}
			}			
			else 
			{
				_write(_fileno(uart), &ch, sizeof(ch));
				if (ch=='\n' || pos>=strLen-1) 
				{
					ch = 0x00;
				}				
				str[pos]=ch;
				pos++;
			}
		}
	}
	while (ch != 0x00);
	if (debugonoff) {
		printf("\nsynthesizer: Done reading user input!\n");
		NutSleep(100);
	}
}

static void buzzer (int taajuus){
	if (debugonoff) {
		printf("synthesizer: Testing buzzer!\n");
		NutSleep(100);
	}
	disable_timer1();		//TIMSK &= ~_BV(TOIE1);	//Disable timer1 interrupt(TOV)
	setup_pwm(taajuus);		//Setting the frequency
	NutSleep(3000);			//Buzzing 3 sec
	disable_timer1();		//TIMSK &= ~(_BV(TOIE1));	// Disable timer1 interrupt(TOV)
}


/*Method for printing characters on the LCD screen*/

static void lcdPrint(char *line){
	int i;
	if (debugonoff) {
		printf("\nsynthesizer: Printing line on LCD!\n");
		NutSleep(100);
	}
	/*
	* Show welcome message in LCD-display
	*/
	LcdSetCursor(0);
	LcdClear();
	for (i = 0; i < strlen(line); i++) {
		LcdSetCursor(i);
		LcdWriteData(line[i]);
	}
}

/* Download files from ftp-server */
static void getDiphones(void) {
	printf("\ngetDiphones: Downloading diphones\n");
	if (debugonoff) {
		printf("\ngetDiphones: calling function ftpGetFiles(debugonoff)\n");
		NutSleep(100);
	}	
	NutSleep(100);
	ftpGetFiles(debugonoff);
}

/* Initializing devices */
void init_devices(void) {
	u_long baud = 115200;

	/*
	* Each device must be registered. We do this by referencing the 
	* device structure of the driver. The advantage is, that only 
	* those device drivers are included in our flash code, which we 
	* really need.
	*
	* The uart0 device is the first one on the ATmega chip. So it 
	* has no configurable base address or interrupt and we set both 
	* parameters to zero.
	*/
	NutRegisterDevice(&DEV_UART, 0, 0);

	/*
	* Now, as the device is registered, we can open it. The fopen()
	* function returns a pointer to a FILE structure, which we use 
	* for subsequent reading and writing.
	*/
	uart = fopen(DEV_UART_NAME, "r+");
	freopen("uart0", "w", stdout);
	_ioctl(_fileno(stdout), UART_SETSPEED, &baud);
	NutSleep(100);

	/*
	* Before doing the first read or write, we set the baudrate.
	* This low level function doesn't know about FILE structures
	* and we use _fileno() to get the low level file descriptor
	* of the stream.
	*
	* The short sleep allows the UART to settle after the baudrate
	* change.
	*/
	_ioctl(_fileno(uart), UART_SETSPEED, &baud);
	NutSleep(100);

	if (debugonoff) {
		printf("\ninit_devices: Starting synthesizer!\n");
		NutSleep(100);
	}
	/* Initialize the LCd */
	if (debugonoff) {
		printf("\ninit_devices: Initializing LCD!\n");
		NutSleep(100);
	}
	NutRegisterDevice(&devLcd, 0, 0);
	/* Initialize UROM */
	if (debugonoff) {
		printf("\ninit_devices: Initialize UROM!\n");
		NutSleep(100);
	}
	
	
    /* Register Ethernet controller */
 	if (debugonoff) {
		printf("\ninit_devices: Initializing Ethernet!\n");
		NutSleep(100);
	}      
	if (NutRegisterDevice(&DEV_ETHER, 0x8300, 5))
      printf("Registering failed!");
	
	//NutRegisterDevice(&devUrom, 0, 0);
	/* Initialize devdebugonoff0 (dunno why) */
	/*
	if (debugonoff) {
		printf("\ninit_devices: Initializing devdebug!\n");
		NutSleep(100);
	}
	NutRegisterDevice(&devdebug0, 0, 0);
	*/	
}

/* Print main menu */
void print_main_menu(void) {
	if (debugonoff) {
		printf("\nprint_main_menu: Printing Main Menu!\n");
		NutSleep(100);
	}
	/*
	* A bit more advanced input routine is able to read a string 
	* up to and including the first newline character or until a
	* specified maximum number of characters, whichever comes first.
	*/
	printf("\n*************");
	printf("\n* Main Menu *");
	printf("\n*************\n\n");
	printf("\n1. Generate speech\n");
	printf("\n2. SNMP chat\n");
	printf("\n3. Settings\n");
	printf("\n4. Help\n");
	printf("\n5. Download diphones\n\n");
	printf("\nEnter your choice: ");
}

void light_led(int lednro) {
	if (debugonoff) {
		printf("\nlight_led: Lighting LED 0x%x!\n", lednro);
		NutSleep(100);
	}
	
	DDRB= 0x0f;
	PORTB=0x0f;
	
	switch (lednro)
	{
		case 1: PORTB = 14;
				break;
		case 2:	PORTB = 13;
				break;
		case 3:	PORTB = 11;
				break;
		case 4:	PORTB = 7;
				break;
		default:  PORTB = 0;
	}
}
