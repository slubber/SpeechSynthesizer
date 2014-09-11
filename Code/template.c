/*!
 * Copyright (C) 2001-2003 by egnite Software GmbH. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor the names of
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY EGNITE SOFTWARE GMBH AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL EGNITE
 * SOFTWARE GMBH OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * For additional information see http://www.ethernut.de/
 */

/*!
 * $Log: uart.c,v $
 * Revision 1.4  2005/11/22 09:14:13  haraldkipp
 * Replaced specific device names by generalized macros.
 *
 * Revision 1.3  2004/11/24 16:35:56  haraldkipp
 * Configurable floating point support
 *
 * Revision 1.2  2004/09/10 10:33:28  haraldkipp
 * Temporarly removed non-configurable FP support
 *
 * Revision 1.1  2003/08/05 18:59:52  haraldkipp
 * Release 3.3 update
 *
 * Revision 1.3  2003/02/04 18:19:41  harald
 * Version 3 released
 *
 * Revision 1.2  2003/02/04 16:24:38  harald
 * Adapted to version 3
 *
 * Revision 1.1  2002/08/09 12:44:10  harald
 * Renamed for make rules
 *
 * Revision 1.5  2002/06/12 11:00:10  harald
 * *** empty log message ***
 *
 * Revision 1.4  2002/06/04 19:13:21  harald
 * *** empty log message ***
 *
 * Revision 1.3  2002/05/08 16:02:34  harald
 * First Imagecraft compilation
 *
 * Revision 1.2  2001/08/10 18:20:41  harald
 * GCC version 3 update
 *
 * Revision 1.1  2001/06/28 18:43:13  harald
 * Preview release
 *
 */

/*!
 * \example uart/uart.c
 *
 * This sample demonstrates the usage of the ATmega on-chip UART.
 * Note, that we don't do any error checking, because without this
 * UART we can't tell the user our problem.
 *
 * We use floating points. Make sure to link with nutlibcrtf.
 */

#include <cfg/crt.h>    /* Floating point configuration. */
#include <cfg/os.h>

#include <dev/debug.h>
#include <dev/board.h>
#include <dev/urom.h>
#include <dev/irqreg.h>
#include <dev/uartavr.h>

#include <sys/device.h>
#include <sys/timer.h>
#include <sys/thread.h>
#include <sys/version.h>
#include <sys/event.h>
#include <sys/heap.h>
#include <sys/confnet.h>
#include <sys/socket.h>

#include <arch/avr.h>

#include <arpa/inet.h>

#include <pro/httpd.h>
#include <pro/dhcp.h>
#include <pro/ssi.h>
#include <pro/asp.h>

//#include <fs/uromfs.h>
//#include <fs/fs.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <io.h>
#include <fcntl.h>

#include "hd44780.h"
#include "speechGeneration.h"
#include "SNMPchat.h"

/* Global variables */
static char *banner = "\nWelcome to Synthesizer Greatest Hits vol. 2\n";
static prog_char presskey_P[] = "Press any key...";
static char myip[20] = "10.10.28.69";
static char mysubnetmask[20] = "255.255.0.0";
static int synthonoff = 0;
static int debugonoff = 0;

static char inbuf[128];

static void speechSynthesize(FILE *uart);
static void runIRC(FILE *uart);
static void openSettings(FILE *uart);
static void openNetworkSettings(FILE *uart);
static void showHelp(FILE *uart);
void readLine(char *str,uint8_t strLen);
static void lcdPrint(char *line);
static void buzzer (int taajuus);
static void playWAV(FILE *uart);

FILE *uart;
/*
 * UART sample.
 *
 * Some functions do not work with ICCAVR.
 */
int main(void)
{
    int got;
    int userchoice;
    char *cp;
    u_long baud = 115200;

#ifdef STDIO_FLOATING_POINT
    float dval = 0.0;
#endif

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

    /* Initialize the LCd */
    NutRegisterDevice(&devLcd, 0, 0);
    /* Initialize UROM */
    NutRegisterDevice(&devUrom, 0, 0);
    NutRegisterDevice(&devDebug0, 0, 0);

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
        /*
         * A bit more advanced input routine is able to read a string 
         * up to and including the first newline character or until a
         * specified maximum number of characters, whichever comes first.
         */
		puts("\n*************");
		puts("\n* Main Menu *");
		puts("\n*************\n");
		puts("\n1. Generate speech\n");
		puts("\n2. IRC\n");
		puts("\n3. Settings\n");
		puts("\n4. Help\n");
		puts("\n5. Play wav\n");
      puts("\nEnter your choice: ");
		readLine(inbuf, sizeof(inbuf));

        /*
         * Streams support formatted output as well as printing strings 
         * from program space.
         */
        if (inbuf[0]) {
		   puts("\n\n");
			userchoice = atoi(inbuf);
			switch (userchoice)
    		{
    		case 1: speechSynthesize(uart);
        		break;
    		case 2: runIRC(uart);
        		break;
    		case 3: openSettings(uart);
        		break;
    		case 4: showHelp(uart);        		
        		break;
    		case 5: playWAV(uart);
    		    break;
    		default: puts("Invalid option selected\n");
    		}
		}

        /*
         * Just to demonstrate formatted floating point output.
         * In order to use this, we need to link the application
         * with nutcrtf instead of nutcrt for pure integer.
         */
#ifdef STDIO_FLOATING_POINT
        dval += 1.0125;
        fprintf(uart, "FP %f\n", dval);
#endif
    }
	while (1);
}

static void speechSynthesize(FILE *uart) {
	int taajuus;
	int userchoice = 0;
	char speechSentence[128];
	DDRB= 0x0f;
	PORTB=0x0f;
	PORTB = 14;	
	lcdPrint("Generate speech");

	do {
	    fflush(uart);
		puts("\n\n\n\n\n");
		puts("\n********************");
		puts("\n* Generate speech *");
		puts("\n********************\n");
		puts("\n1. Text to speech\n");
		puts("\n2. Return to Main Menu\n");
		puts("\n*3. Buzzer*\n");
		puts("\nEnter your choice: ");
	   readLine(inbuf, sizeof(inbuf));
	
		if (inbuf[0]) {
	         puts("\n\n");
				userchoice = atoi(inbuf);
			
				switch (userchoice)
	    		{
	    		case 1: puts("\nEnter sentence you want to hear: ");
					readLine(speechSentence, sizeof(speechSentence));
					printf("\nCan you hear: %s\n", speechSentence);
	        		break;
	    		case 2: puts("\nReturning to Main Menu\n");
						puts("\n\n\n\n\n");
	        		break;
				case 3: puts("\nInsert normalized frequency value: ");
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

static void runIRC(FILE *uart) {
	char mynick[10] = "ISMO";
	char mychannel[20] = "SOP";
	int userchoice = 0;
	DDRB= 0x0f;
	PORTB=0x0f;
	PORTB = 13;
	lcdPrint("SNMP Chat");

	do {
	   fflush(uart);
		puts("\n\n\n\n\n");
		puts("\n*************");
		puts("\n* SNMP Chat *");
		puts("\n*************\n");
		printf("\n1. Nickname: %s\n", mynick);
		printf("\n2. Channel: %s\n", mychannel);
		puts("\n3. Start chatting\n");
		puts("\n4. Return to Main Menu\n");
		puts("\nEnter your choice: ");
	   
	   readLine(inbuf, sizeof(inbuf));
	
		if (inbuf[0]) {
	         puts("\n\n");
				userchoice = atoi(inbuf);
			
				switch (userchoice)
	    		{
	    		case 1: puts("\nEnter your nickname: ");
					readLine(mynick, sizeof(mynick));
	        		break;
	    		case 2: puts("\nEnter server address: ");
					readLine(mychannel, sizeof(mychannel));
	        		break;
				case 3: puts("\nStart Chatting!\n");
					SNMP(mynick, mychannel, myip, mysubnetmask);
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

static void openSettings(FILE *uart) {
    int userchoice = 0;
	DDRB= 0x0f;
	PORTB=0x0f;
	PORTB = 11;
	lcdPrint("Settings");
	do {
	   fflush(uart);
		puts("\n\n\n\n\n");
		puts("\n************");
		puts("\n* Settings *");
		puts("\n************\n");
		puts("\n1. Network settings\n");
		if (synthonoff == 0) puts("\n2. Synthesizer: OFF\n");
		else puts("\n2. Synthesizer: ON\n");
		if (debugonoff == 0) puts("\n2. Debug: OFF\n");
		else puts("\n2. Debug: ON\n");
		puts("\n4. Return to Main Menu\n");
		puts("\nEnter your choice: ");
	   readLine(inbuf, sizeof(inbuf));
	
		if (inbuf[0]) {
	         puts("\n\n");
				userchoice = atoi(inbuf);
			
				switch (userchoice)
	    		{
	    		case 1: openNetworkSettings(uart);
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

static void showHelp(FILE *uart) {
	DDRB= 0x0f;
	PORTB=0x0f;
	PORTB = 7;
	lcdPrint("Help");
	puts("\n\n\n\n\n");
	puts("\n********");
	puts("\n* Help *");
	puts("\n********\n");
	puts("\nBeatles - Help! Lyrics\n\nHelp, I need somebody,\nHelp, not just anybody,\nHelp, you know I need someone, help.\n\nWhen I was younger, so much younger than today,\nI never needed anybody's help in any way.\nBut now these days are gone, I'm not so self assured,\nNow I find I've changed my mind and opened up the doors.\n\nHelp me if you can, I'm feeling down\nAnd I do appreciate you being round.\nHelp me, get my feet back on the ground,\nWon't you please, please help me.\n");
	puts("\nPress any key to continue...\n");
	fflush(uart);
	fgetc(uart);
}

static void openNetworkSettings(FILE *uart) {
   int userchoice = 0;
	lcdPrint("Network settings");
	do {
	   fflush(uart);
		puts("\n\n\n\n\n");
		puts("\n********************");
		puts("\n* Network settings *");
		puts("\n********************\n");
		printf("\n1. IP: %s\n", myip);
		printf("\n2. Subnet mask: %s\n", mysubnetmask);
		puts("\n3. Return to Settings Menu\n");
		puts("\nEnter your choice: ");
		readLine(inbuf, sizeof(inbuf));
	
		if (inbuf[0]) {
	            fputs("\n\n", uart);
				userchoice = atoi(inbuf);
			
				switch (userchoice)
	    		{
	    		case 1: puts("\nEnter new IP address: \n");
					    readLine(myip, uart);
	        		break;
	    		case 2: puts("\nEnter new subnet mask: \n");
					    readLine(mysubnetmask, uart);
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

static void buzzer (int taajuus){

// Initialize PORTB. Leds and PWM Output are here.
    
    DDRB= _BV(DDB7);//0xff;
    //PORTB= 0x0f;	// 0xff makes software unstable..dunno why TODO: PORTB|=0xff;
    TIMSK &= ~_BV(TOIE1);	// Disable timer1 interrupt(TOV)
  
    setup_pwm(taajuus);		//Setting the frequency
	NutSleep(3000);			//Buzzing 3 sec
	TIMSK &= ~(_BV(TOIE1));	// Disable timer1 interrupt(TOV)


}


/*Method for printing characters on the LCD screen*/

static void lcdPrint(char *line){
	int i;
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

static void playWAV(FILE *uart) {
    printf("\n\nTest wav decoding with UROM\n");
    NutSleep(100);
    //init_localaudio(8000, "UROM:testi.wav", 1);
    playWaveFile();
}
