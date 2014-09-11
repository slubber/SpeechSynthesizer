/* * Functions for speech generation
 * * SOP Group 28
 *   Teemu Autio, Jussi Pollari, Antti Seppälä
 *
/* @{ */

#include <sys/timer.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <string.h>

//#include <stdio.h>
//#include <ctype.h>
//#include <avr/sfr_defs.h>
//#include <dev/debug.h>
//#include <dev/urom.h>
//#include <sys/thread.h>
//#include <dev/irqreg.h>
//#include <arch/avr.h>
//#include <dev/uartavr.h>
#include <sys/heap.h>
#include <sys/bankmem.h>

#include <net/route.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pro/httpd.h>
#include <pro/ftpd.h>
#include <pro/dhcp.h>
#ifdef ETHERNUT2
#include <dev/lanc111.h>
#else
#include <dev/nicrtl.h>
#endif

#include <sys/confnet.h>
#include <math.h>
#include <time.h>
#include <sys/version.h>
#include <sys/thread.h>
#include <dev/uartavr.h>

#include "speechGeneration.h"
#include "spiflash.h"
//#include "utils.h"
#define COLCOUNT 5
#define FLASHPAGESIZE 264
//#define DIPHONECOUNT sizeof(ftpdifones)/3

#define FTP_MY_MAC   02, 01, 05, 03, 01, 250
//#define FTP_MY_MAC { 0x02, 0x01, 0x05, 0x03, 0x01, 0x03 }
#define FTP_MY_IPADDR "10.10.28.11"
#define FTP_MY_IPMASK "255.255.0.0"
#define FTP_MY_IPGATE "10.10.28.99"

#define TCPIP_MSS    1460
#define TCPIP_BUFSIZ 5840
#define FTPD_TIMEOUT 600000
#define TCP_MAXSEG   0x02


char ftpdifones[][3] = { "a", "b", "d", "e", "f", "g", "h", "i", "j", "k", 
                  "l", "m", "n", "o", "p", "r", "s", "t", "u", "v", 
				"x", "y","ä", "ö", "ad", "ar", "an", "as", "av", 
				"ba", "bä", "be", "bi", "bo", "bö", "bu", "by", "dä", "de", 
				"di", "do", "dö", "du", "dy", "ej", "et", "fa", "fä", "fi", 
				"fo", "fö", "fu", "fy", "gi","go", "gö", "gu", "gy", "ha", "hä", 
				"hi", "ho", "hö", "hu", "hy", "ij", "il", "im", "in", "ip", "ir", 
				"is", "it", "iv", "ja", "jä", "jo", "jö", "ju", "jy", "ka", "kä",
				"ko", "kö", "ku", "ky", "la", "lä", "lo", "lö", "lu", "ly",
				"ma", "ng", "no", "nä", "nö", "nu", "ny", "ot", "pa", "pä","pö",
                "pu", "py", "ra", "rä", "rö", "ru", "ry", "sä",
				"sö", "su", "sy", "ta", "tä", "tö", "tu", "ty", "uv", "va",
                "vä", "vö", "vy", "ak", "ap", "ek", "ik", "ke", "ki", "ok",
                "pe", "po", "se", "si", "so", "äk", "ök", "pi", "ep", "es",
				"op", "os", "sa", "uk", "up", "us", "yk", "yp", "ys", "äp", "äs", "ös", "öp"};


//char ftpdifones[DIPHONECOUNT][3] = { "ja", "ka", "la", "ma", "pa", "pi", "ra", "ta", "va" };
int diphonecounty = sizeof(ftpdifones)/3;
int buzzer = 0;
int pagecounter = 0;
int debugspeech = 0;
volatile unsigned char *wavbuffer;
struct diphonerecord diphonetable2[sizeof(ftpdifones)/3];
long wavbuffersize = 0;
long buffercounter = 0;
int diphonerow = 0;
int diphonebreak = 400;

/** Interrupt vector of TIMER1.
 * Handle PWM signal output
 * Output values from fifo to timer2 8bit comparator
 */
INTERRUPT(SIG_OVERFLOW1)
{
  /* Here the magic will happen: updating the value to be played */
  if (buzzer == 0)
  OCR2 = OCR2 + 1;
  else
  OCR2 = nextSample();
}

/** Setup Harware pwm output.
 * Use two hardare timers, Timer1 at 16bit, for samplerate timer.
 * Timer2 at 8bit for PWM sound output.
 * Using two counters allow better sound quality. 8bit counter is operating at 57 kHz
 * So we cannot hear the peep at saplerate.
 */
void setup_pwm(unsigned int sample_rate) {
	ICR1 = (SYSTEMCLOCK / sample_rate);			/* TOP = (Fclk / samplerate) */
	TCCR1A = _BV(WGM11);
	TCCR1B = _BV(WGM13) | _BV(WGM12);			/* MODE: Fast PWM*/
	TCCR1A |= _BV(COM1A1);						/* compare settings */

	OCR1A = 0x00;								/* reset register */

	DDRB= _BV(DDB7);							/* 0xff; */

	TCCR1B |= _BV(CS10);						/* Clock/1 */

	TCCR2 =  _BV(WGM21) | _BV(WGM20);			/* Timer2, mode fast PWM */
	TCCR2 |= _BV(COM21);						/* compare settings */

	TCCR2 |= _BV(CS20);							/* Clock/1 */

	enable_timer1();

}

/* Download diphones */
void ftpGetFiles(int debugenable) {
	int i;
	debugspeech = debugenable;
	printf("\nftpGetFiles: Downloading diphones %d\n", sizeof(ftpdifones)/3);
	if (debugspeech) {
		printf("\nftpGetFiles: Downloading diphones\n");
	}
	InitEthernetDevice();	
	NutSleep(6000);
		
	ftpGet();
	//reloadDiphones();

	if (debugspeech) {
		puts("\n\n[Diphonename],[Startpage],[Pagesize],[Pageoffset],[Diphonesize]");
		for (i = 0; i < diphonecounty; i++) {
			printf("\n[%s         ],[%d      ],[%d      ],[%d        ],[%d         ]",
				diphonetable2[i].diphonename, diphonetable2[i].startpage, diphonetable2[i].pagesize, 
				diphonetable2[i].pageoffset, diphonetable2[i].filesize);
		}
	}

	if (debugspeech) {
		printf("\nftpGetFiles: Done downloading diphones\n");
	}	
}

int nextSample(void)
{
	if (buffercounter > wavbuffersize) {
		disable_timer1();
		wavbuffersize = 0;
		buffercounter = 0;
		free((unsigned char*)wavbuffer);
		return 0;
	}
	else 
	return wavbuffer[buffercounter++];
}

void changeChar(char * lause, int length){
	int i=0;
	int j=0;
	int k=0;

	/* char lause[200] = {'\0'};		/*Syötetty lause (Mikä pannaan tälle pituusrajaksi?)*/
	char muunnettu[500] = {'\0'};		/*Muunnettu lause*/
	char *korvaaja;						/*Tähän tallennetaan erikoismerkin korvaava teksti*/

	if (debugspeech) {
		printf ("\nAlkuperäinen lause:%s,pituus:%d merkkiä\n",lause,strlen(lause));
		NutSleep(100);
	}

	for (i=0; i<length; i++){
		if (debugspeech) {
			printf("\nKäsitellään merkkiä: %d", i);
			NutSleep(100);
		}
		if (lause[i]=='\n'); 		/*En oo ihan varma että tarvisko tätä, mutta eipä tuo näy haittaavankaan*/
	
		else if ( ((lause[i]>64)&&(lause[i]<91))||((lause[i]>96)&&(lause[i]<123))){
			if (lause[i] == 'c') 
				muunnettu[j] = 'k';
			else if (lause[i] == 'q') 
				muunnettu[j] = 'k';
			else if (lause[i] == 'w') 
				muunnettu[j] = 'v';
			else if (lause[i] == 'z') 
				muunnettu[j] = 's';
			else muunnettu[j]=lause[i]; /*Normikirjaimet ja välilyönti*/
			j++;
		}
		
		else if ((lause[i]=='ä')||(lause[i]=='Ä')||(lause[i]=='ö')||(lause[i]=='Ö')||(lause[i]=='å')||(lause[i]=='Å')){
			if (lause[i] == 'å')
				muunnettu[j] = 'o';
			else if (lause[i] == 'Å')
				muunnettu[j] = 'O';
			else muunnettu[j]=lause[i]; /*Skandit*/
			j++;
		}
	
		else if ((lause[i]==' ')||(lause[i]==',')||(lause[i]=='.')){
			muunnettu[j]=lause[i]; /*Space, pilkku, piste*/
			j++;
		}
	
		/*Tässä alkaa erikoismerkkien käsittely*/
		else {
	
			k=0;
	
			if (lause[i]=='§')
				korvaaja="pykälä";
	
			else if (lause[i]=='½')
				korvaaja="puoli";
	
			else if (lause[i]=='0')
				korvaaja="nolla";
			
			else if (lause[i]=='1')
				korvaaja="yksi";
		
			else if (lause[i]=='2')
				korvaaja="kaksi";	
			
			else if (lause[i]=='3')
				korvaaja="kolme";
			
			else if (lause[i]=='4')
				korvaaja="neljä";
			
			else if (lause[i]=='5')
				korvaaja="viisi";
			
			else if (lause[i]=='6')
				korvaaja="kuusi";
			
			else if (lause[i]=='7')
				korvaaja="seitsemän";	
			
			else if (lause[i]=='8')
				korvaaja="kahdeksan";
			
			else if (lause[i]=='9')
				korvaaja="yhdeksän";
				
			else if (lause[i]=='+')
				korvaaja="plus";
		
			else if (lause[i]=='-')
				korvaaja="viiva";	
			
			else if (lause[i]=='*')
				korvaaja="tähti";
		
			else if (lause[i]=='/')
				korvaaja="kauttaviiva";
					
			else if (lause[i]=='@')
				korvaaja="at";
			
			else if (lause[i]=='£')
				korvaaja="puntamerkki";
			
			else if (lause[i]=='¤')
				korvaaja="aurinkomerkki";
			
			else if (lause[i]=='$')
				korvaaja="dollarimerkki";
			
			else if (lause[i]=='%')
				korvaaja="prosenttimerkki";
			
			else if (lause[i]=='&')
				korvaaja="ja";
			
			else if ((lause[i]=='{')||(lause[i]=='}'))
				korvaaja="kaarisulku";
			
			else if ((lause[i]=='(')||(lause[i]==')'))
				korvaaja="sulkumerkki";
			
			else if ((lause[i]=='[')||(lause[i]==']'))
				korvaaja="hakasulku";
			
			else if (lause[i]=='?')
				korvaaja="kysymysmerkki";
			
			else if (lause[i]=='~')
				korvaaja="tilde";
			
			else if (lause[i]=='<')
				korvaaja="on pienempi kuin";
			
			else if (lause[i]=='>')
				korvaaja="on suurempi kuin";
			
			else if (lause[i]=='|')
				korvaaja="putki";
			
			else if (lause[i]=='!')
				korvaaja="huutomerkki";
			
			else if (lause[i]=='#')
				korvaaja="risuaita";
			
			else if (lause[i]=='"')
				korvaaja="lainausmerkki";
			
			else if (lause[i]=='=')
				korvaaja="on yhtä kuin";
			
			else if ((lause[i]=='\'')||(lause[i]=='`')||(lause[i]=='´'))
				korvaaja="heittomerkki";
			
			else if (lause[i]=='^')
				korvaaja="hatten";
				
			else if (lause[i]=='¨')
				korvaaja="umlaut";
				
			else if (lause[i]=='\\')
				korvaaja="kenoviiva";
			
			else if (lause[i]=='_')
				korvaaja="alaviiva";
			
			else korvaaja="tuntematon merkki";
		
			while (korvaaja[k] != '\0') {
				muunnettu[j] = korvaaja[k];
				k++;
				j++;
			}
		}		/*Erikoismerkkien käsittely*/
	}			/*For*/
	if (debugspeech) {
		//Tulostetaan testauksen vuoksi muutettu lause
		printf("\nMuunnettu lause:%s, pituus:%d\n",muunnettu,j);
		NutSleep(100);
	}
	muunnettu[j] = '\0';
	strcpy(lause,muunnettu);
}

int sampleFromFlash2(char * samplefile, unsigned char * buffer, int reverse)
{
	int revcount, bytesize, error, samplesize, filesize, startpage, curpage, pagesize, pause, tempsize;
	unsigned char merkki[FLASHPAGESIZE];
	unsigned char revmerkki[FLASHPAGESIZE];
	error = findSample(samplefile);
	if (!error) {
		startpage  = diphonetable2[diphonerow].startpage;
		pagesize   = diphonetable2[diphonerow].pagesize;
		samplesize = diphonetable2[diphonerow].pageoffset;
		filesize   = diphonetable2[diphonerow].filesize;
		pause = 0;//      = diphonetable2[diphonerow].addbreak;

		if (debugspeech) {
			printf("\sampleFromFlash: File: %s, Startpage: %d, Pagesize: %d, Offset: %d, Filesize: %d\n", samplefile, startpage, pagesize, samplesize, filesize);
			NutSleep(100);
		}
		bytesize = 0;
		if(XflashInit()) {
			NutSleep(100);
			if (debugspeech) {
				printf("\sampleFromFlash: DataFlash initialized OK!\n");
				NutSleep(100);
			}				
			bytesize = sizeof(merkki);
			for (curpage = startpage; curpage < startpage+pagesize; curpage++) {
				if (curpage == (startpage+pagesize-1))
					bytesize = samplesize;
				if (debugspeech) {
					printf("\nsampleFromFlash: reading from page %d, bytesize %d\n", curpage, bytesize);
					NutSleep(100);				
				}
				error = PageRead(curpage, merkki, bytesize, 0);
				if (!error) {
					if (reverse) {
						for(revcount = 0; revcount < bytesize; revcount++) {
							revmerkki[revcount] = merkki[bytesize-revcount];
						}
						strncat(buffer, revmerkki, bytesize);
						printf("\nsampleFromFlash: reversed diphone, size:%d\n", bytesize);
					}
					else
						strncat(buffer, merkki, bytesize);
					if (debugspeech) {
						printf("\nsampleFromFlash: bytes stored to buffer\n");
						NutSleep(100);
					}
				}
				else {
					printf("\sampleFromFlash: Error reading from DataFlash\n");
					NutSleep(100);
					break;
				}
			}
			if (!error) {
				if(pause) {
					printf("\nsampleFromFlash: pausing for: %d\n", diphonebreak);
					tempsize = 0;
					memset(merkki, 0, FLASHPAGESIZE);					
					for(tempsize = diphonebreak; tempsize >= FLASHPAGESIZE; tempsize -= FLASHPAGESIZE) {
						strncat(buffer, merkki, FLASHPAGESIZE);
						printf("\nsampleFromFlash: zeores for: %d\n", tempsize);
					}
					if (tempsize > 0) {
						strncat(buffer, merkki, tempsize);
						printf("\nsampleFromFlash: zeores for: %d\n", tempsize);
					}
				}
				if (debugspeech) {
					printf("\sampleFromFlash: Done reading file %s\n", samplefile);
					NutSleep(100);
				}
			}
		}
		else {
			printf("\sampleFromFlash: DataFlash initialization FAILED!\n");
			NutSleep(100);
		}	
	}
	return(error);
}

int findSample(char * samplefile) {
	int result = -1;
	for (diphonerow = 0; diphonerow < diphonecounty; diphonerow++) {
		//printf("\nfindSample: comparing %s and %s\n", samplefile, diphonetable2[diphonerow].diphonename); 
		if(strcmp(samplefile, diphonetable2[diphonerow].diphonename) == 0) {
			if (debugspeech) {
				printf("\nfindSample: diphone %s found at row: %d\n", samplefile, diphonerow);
				NutSleep(100);
			}
			result = 0;
			break;
		}
		else {
			result = -1;
		}
	}
	if (result == -1) {
		if (debugspeech) {
			printf("\nfindSample: no diphones found\n");
			NutSleep(100);
		}
		diphonerow = 0;
	}
	return result;
}

void speakSentence(char * sentence, int debugenable) {
	debugspeech = debugenable;	
	buzzer = 1;	
	int wordlength= 25;
	char oneword[wordlength];
	int i, lettercounter = 0;
	if (debugspeech) {
		printf("\nspeakSentence: sentence: %s\n", sentence);
	}
	changeChar(sentence, strlen(sentence));
	for (i = 0; i <= strlen(sentence); i++) {
		if (sentence[i] == ' ' || i == strlen(sentence)) {
			oneword[lettercounter] = '\0';
			if (debugspeech) {
				printf("\nspeakSentence: Word (%d)%s\n", strlen(oneword), oneword);
				NutSleep(100);
			}
			speakWord(oneword);
			memset(oneword, 0, wordlength);
			lettercounter = 0;
		}
		else {
			oneword[lettercounter] = sentence[i];
			lettercounter++;
		}
	}
}

void speakWord(char * oneword) {	
	char * filename;
	int i, diphonesfound = 1;
	while (wavbuffersize != 0);
	buffercounter = 0;
	wavbuffersize = 0;
	int length = strlen(oneword);	
	char *diphones[length];
	length = AnttiFunktio(oneword, diphones);
	
	for (i = 0; i < length; i++) {
		diphonesfound &= 1;
		/*
		if(diphones[i][strlen(diphones[i])] == '1') {
			filename = (char*)malloc(strlen((char*)diphones[i]));
			strncpy(filename, (char*)diphones[i], strlen((char*)diphones[i]));
		}
		else {*/
			filename = (char*)malloc(strlen(diphones[i])+1);
			strncpy(filename, diphones[i], strlen(diphones[i]));
			filename[strlen(diphones[i])] = '\0';
		//}
		if (debugspeech) {
			printf("\nspeakWord: diphone (%d)%s\n", strlen(filename), filename);
			NutSleep(100);
		}
		if (!findSample(filename)) {
			wavbuffersize += diphonetable2[diphonerow].filesize;
			if (diphonetable2[diphonerow].addbreak)
				wavbuffersize += diphonebreak;
			diphonesfound = 1;
		}
		else 
			diphonesfound = 0;
		free(filename);
	}
	if (debugspeech) {
		printf("\nspeakWord: namecopy (%d)\n", length);
		NutSleep(100);
	}
	if (diphonesfound && wavbuffersize > 0) {
		wavbuffer = (unsigned char*)malloc(wavbuffersize+1);
		for (i = 0; i < length; i++) {
		   /*
			if(((char*)diphones[i][strlen((char*)diphones[i])]) == '1') {
				printf("\nspeakWord: reversed(%d) %s(%d)\n", length, (char*)diphones[i], strlen((char*)diphones[i]));
				NutSleep(100);
				filename = (char*)malloc(strlen((char*)diphones[i]));
				strncpy(filename, (char*)diphones[i], strlen((char*)diphones[i]));
				(char*)diphones[i][strlen((char*)diphones[i])] = '\0';
				sampleFromFlash(filename, (unsigned char*)wavbuffer, 1);
			}
			else {
			   printf("\nspeakWord: normal(%d) %s(%d)\n", length, (char*)diphones[i], strlen((char*)diphones[i]));
				NutSleep(100);
				filename = (char*)malloc(strlen((char*)diphones[i])+1);
				strncpy(filename, (char*)diphones[i], strlen((char*)diphones[i])+1);			
				sampleFromFlash(filename, (unsigned char*)wavbuffer, 0);
			}
			*/
				if (debugspeech) {
			   	printf("\nspeakWord: normal(%d) %s(%d)\n", length, diphones[i], strlen(diphones[i]));
					NutSleep(100);
				}
				filename = (char*)malloc(strlen(diphones[i])+1);
				strncpy(filename, diphones[i], strlen(diphones[i]));
				filename[strlen(diphones[i])] = '\0';
				//sampleFromFlash(filename, (unsigned char*)wavbuffer, 0);
				sampleFromFlash(filename, (unsigned char*)wavbuffer);
				free(filename);
		}		
		setup_pwm(8000);
		//NutSleep(wavbuffersize/8000);		
	}
	else 
		printf("\nspeakWord: Can't synthesize word %s\n", oneword);	
	if (debugspeech) {
		printf("\nspeakWord: freeing diphones (%d)\n", length);
		NutSleep(100);
	}
	for(i = 0; i < length; i++) {
		free((char*)diphones[i]);
	}
}

void ftpSetSock(TCPSOCKET *socket) {
	/*
	 * Set specified socket options. 
	 */
	#ifdef TCPIP_MSS
	      {
	          u_short mss = TCPIP_MSS;
	          NutTcpSetSockOpt(socket, TCP_MAXSEG, &mss, sizeof(mss));
	      }
	#endif
	#ifdef FTPD_TIMEOUT
	      {
	         u_long tmo = FTPD_TIMEOUT;
	         NutTcpSetSockOpt(socket, SO_RCVTIMEO, &tmo, sizeof(tmo));
	      }
	#endif
	#ifdef TCPIP_BUFSIZ
	      {
	          u_short siz = TCPIP_BUFSIZ;
	          NutTcpSetSockOpt(socket, SO_RCVBUF, &siz, sizeof(siz));
	      }
	#endif
}

void ftpReceiveFile(FILE *stream, char *inbuf, char *diphone, u_long ftp_addr) {
	int inbuffersize=128, outbuffersize, got=0;
	unsigned char *outbuf;
	char *message;
	char *filename;
	TCPSOCKET *sock2;
	int x;
	char filesizechar[4];
	u_short ftp_data_port = 1400;

	static prog_char dotwav_P[] = "%s.wav";			//Wav postfix
	static prog_char retrfile_P[] = "RETR %s\r\n";	//get file
	static prog_char pasv_P[] = "PASV\r\n";			//Passive mode
	static prog_char typei_P[] = "TYPE I\r\n";		//Binary mode
	static prog_char size_P[] = "SIZE %s\r\n";		//SIZE
				
	filename = malloc(10);
	message = malloc(inbuffersize);

	sprintf_P( (char*)filename, dotwav_P, (char*)diphone );
	if (debugspeech) {
		printf("\nftpReceiveFile: getfile : %s\n", (char*)filename);
		NutSleep(50);
	}

	if (debugspeech) {
		printf("\nftpReceiveFile: Sending TYPE I\n");	
		NutSleep(50);
	}
	fflush(stream);
	fprintf_P(stream, typei_P);


	fflush(stream);
	fgets(inbuf, inbuffersize, stream);
	got = strlen(inbuf);
	if (debugspeech) {
		printf("\nftpReceiveFile: received(%d): %s\n", got, inbuf);
		NutSleep(50);
	}
	memset(inbuf, 0, inbuffersize);

	sprintf_P( (char*)message, size_P, (char*)filename);
	if (debugspeech) {
		printf("\nftpReceiveFile: Message: %s\n", (char*)message);
		NutSleep(50);
	}

	if (debugspeech) {
		printf("\nftpReceiveFile: Sending %s", message);
		NutSleep(50);
	}
	fflush(stream);
	fprintf(stream, message);

	while(got != 213) {
		fflush(stream);
		fgets(inbuf, inbuffersize, stream);
		if (inbuf[3] == ' ')
			got = atoi(inbuf);
		else got = strlen(inbuf);
		if (got == 550)
			break;
		if (debugspeech) {
			printf("\nftpReceiveFile: received(%d): %s\n", got, inbuf);
			NutSleep(50);
		}
		for (x = 0; x < 4; x++) {
			filesizechar[x] = inbuf[4+x];
		}
		outbuffersize = atoi(filesizechar);
		if (debugspeech) {
			printf("\nftpReceiveFile: %s size: %d\n", (char*)diphone, outbuffersize);
			NutSleep(50);
		}
		memset(inbuf, 0, inbuffersize);
	}
	if (got == 213) {
		memset(message, 0, inbuffersize);
		outbuf = (unsigned char*)malloc(outbuffersize);
		//NutSleep(50);

		if (debugspeech) {
			printf("\nftpReceiveFile: Sending PASV\n");
			NutSleep(0);
		}
		fflush(stream);
		fprintf_P(stream, pasv_P);

		fflush(stream);
		fgets(inbuf, inbuffersize, stream);
		got = strlen(inbuf);
		if (debugspeech) {
			printf("\nftpReceiveFile: received(%d): %s\n", got, inbuf);
			NutSleep(50);
		}
		memset(inbuf, 0, inbuffersize);

		fflush(stream);
		if ((sock2 = NutTcpCreateSocket()) != 0) {
			NutSleep(0);

			fflush(stream);
			ftpSetSock(sock2);
			if (debugspeech) {
				printf("\nftpReceiveFile: FTP sock2 created!\n");
				NutSleep(50);
			}

			NutTcpConnect(sock2, ftp_addr, ftp_data_port);
			if (debugspeech) {
				printf("\nftpReceiveFile: Data connection created OK!\n");
				NutSleep(50);
			}
			
			fflush(stream);
			sprintf_P( (char*)message, retrfile_P, (char*)filename);
			if (debugspeech) {
				printf("\nftpReceiveFile: Message: %s\n", (char*)message);
				NutSleep(50);
			}

			if (debugspeech) {
				printf("\nftpReceiveFile: Sending %s\n", message);
				NutSleep(50);
			}
			fflush(stream);
			fprintf(stream, message);

			while(got != 226) {
				fflush(stream);
				fgets(inbuf, inbuffersize, stream);
				if (inbuf[3] == ' ')
					got = atoi(inbuf);
				else got = strlen(inbuf);
				if (debugspeech) {
					printf("\nftpReceiveFile: received(%d): %s\n", got, inbuf);
					NutSleep(50);
				}
				memset(inbuf, 0, inbuffersize);
				if (got == 426)
					break;
			}
			if (got == 226) {
				/* Receive a file. */
				while ((got = NutTcpReceive(sock2, outbuf, outbuffersize)) > 0) {				
					//memset(outbuf, 0, outbuffersize);
					if (debugspeech) {
						printf("\nftpReceiveFile: data(%d): %s\n", got, outbuf);	
						NutSleep(50);
					}
				}
				sampleToFlash(diphone, (unsigned char*)outbuf, outbuffersize);
				//NutSleep(0);
			}
			NutTcpCloseSocket(sock2);
			if (debugspeech) {
				printf("\nftpReceiveFile: FTP sock2 closed!\n");
				NutSleep(50);
			}
			sock2 = 0;
		}
	}
	else printf("\nftpReceiveFile: File not found!\n");
	free(outbuf);
	free(filename);
	free(message);
}

void ftpGet(void) {
	int inbuffersize = 128;
	TCPSOCKET *sock;
	//TCPSOCKET *listsock
	int got;
	char *inbuf;
	FILE *stream;
	u_short ftp_port = 21;
	//u_short ftp_data_port = 1400;
	int ftpcolcount = 0;

	char *diphone;

	static prog_char quit_P[] = "QUIT\r\n";  		  //Quit
	static prog_char user_P[] = "USER SOP\r\n";		//Username
	static prog_char passwd_P[] = "PASS SOP\r\n"; //Password
/*
	static prog_char list_P[] = "LIST -T\r\n";   //List
	static prog_char typea_P[] = "TYPE A\r\n";		//Binary mode
	static prog_char pasv_P[] = "PASV\r\n";			//Passive mode
*/
	diphonerow = 0;
	pagecounter = 0;

  u_long ftp_addr = inet_addr(FTP_MY_IPGATE);

	/*
	* Create a socket.
	*/
	if ((sock = NutTcpCreateSocket()) != 0) {
		ftpSetSock(sock);
		if (debugspeech) {
			printf("\nftpGet: TCP Socket created!\n");
			NutSleep(100);
		}

		if ((inbuf = malloc(inbuffersize)) != 0) {
	 		if (NutTcpConnect(sock, ftp_addr, ftp_port) == 0) {
				printf("\nftpGet: FTP Connection established\n");
				if ((stream = _fdopen((int) sock, "r+b")) != 0) {
					got = 0;
					while(got != 220) {
						fflush(stream);
						fgets(inbuf, inbuffersize, stream);
						if (inbuf[3] == ' ')
							got = atoi(inbuf);
						else got = strlen(inbuf);
						if (debugspeech) {
							printf("\nftpGet: received(%d): %s\n", got, inbuf);
							NutSleep(100);
						}
						memset(inbuf, 0, inbuffersize);
						//NutSleep(0);
					}

					fflush(stream);
					if (debugspeech) {
						printf("\nftpGet: Sending USER SOP\n");
						NutSleep(100);
					}
					fprintf_P(stream, user_P);
					//NutSleep(0);

					fflush(stream);
					fgets(inbuf, inbuffersize, stream);
					got = strlen(inbuf);
					if (debugspeech) {
	   				printf("\nftpGet: received(%d): %s\n", got, inbuf);
						NutSleep(100);
					}
          memset(inbuf, 0, inbuffersize);
					//NutSleep(0);
					
					fflush(stream);
					if (debugspeech) {
						printf("\nftpGet: Sending PASS SOP\n");
						NutSleep(100);
					}
					fprintf_P(stream, passwd_P);					
					//NutSleep(0);
					
					got = 0;
					while(got != 230) {
						fflush(stream);
						fgets(inbuf, inbuffersize, stream);
						if (inbuf[3] == ' ')
							got = atoi(inbuf);
						else got = strlen(inbuf);
						if (debugspeech) {
							printf("\nftpGet: received(%d): %s\n", got, inbuf);
							NutSleep(100);
						}
						memset(inbuf, 0, inbuffersize);
						//NutSleep(0);
					}
/*
					fflush(stream);
					printf("\nftpGet: Sending TYPE A\n");
					fprintf_P(stream, typea_P);					
					NutSleep(0);

					fflush(stream);
					fgets(inbuf, inbuffersize, stream);
					got = strlen(inbuf);
   				printf("\nftpGet: received(%d): %s\n", got, inbuf);
          memset(inbuf, 0, inbuffersize);
					NutSleep(0);

					fflush(stream);
					printf("\nftpGet: Sending PASV\n");
					fprintf_P(stream, pasv_P);					
					NutSleep(0);

					fflush(stream);
					fgets(inbuf, inbuffersize, stream);
					got = strlen(inbuf);
					printf("\nftpGet: received(%d): %s\n", got, inbuf);
					memset(inbuf, 0, inbuffersize);
					NutSleep(0);

					if ((listsock = NutTcpCreateSocket()) != 0) {
						NutSleep(0);

						ftpSetSock(listsock);
						printf("\nftpGet: FTP listsock created!\n");
						NutSleep(0);

						fflush(stream);
						NutTcpConnect(listsock, ftp_addr, ftp_data_port);
						printf("\nftpGet: Data connection created OK!\n");
						NutSleep(0);
					
						fflush(stream);
						printf("\nftpGet: Sending LIST\n");
						fprintf_P(stream, list_P);					
						NutSleep(0);

						got = 0;
						while(got != 150) {
							fflush(stream);
							fgets(inbuf, inbuffersize, stream);
							if (inbuf[3] == ' ')
								got = atoi(inbuf);
							else got = strlen(inbuf);
							printf("\nftpGet: received(%d): %s\n", got, inbuf);
							memset(inbuf, 0, inbuffersize);
							if (got == 426)
								break;
							NutSleep(0);
						}					

						if (got == 150) {
							while ((got = NutTcpReceive(listsock, inbuf, inbuffersize)) > 0) {
							 	printf("\nftpGet: data(%d): %s\n", got, inbuf);
								memset(inbuf, 0, inbuffersize);
								NutSleep(50);
							}
						}					
						NutTcpCloseSocket(listsock);
						printf("\nftpGet: FTP listsock closed!\n");
						NutSleep(50);
						listsock = 0;
					}
*/
					diphone = (char*)malloc(5);
					printf("\n");
					for(ftpcolcount = 0;ftpcolcount < diphonecounty; ftpcolcount++) {
						printf(".");
						strncpy(diphone, ftpdifones[ftpcolcount], strlen(ftpdifones[ftpcolcount])+1);
						ftpReceiveFile(stream, inbuf, diphone, ftp_addr);
						//NutSleep(100);
						memset(diphone, 0, 5);
					}
					free(diphone);

					fflush(stream);
					if (debugspeech) {
						printf("\nftpGet: Sending QUIT!\n");
						NutSleep(100);
					}
					fprintf_P(stream, quit_P);
					//NutSleep(0);

					got = 0;
					while(got != 221) {
						fflush(stream);
						fgets(inbuf, inbuffersize, stream);
						if (inbuf[3] == ' ')
							got = atoi(inbuf);
						else got = strlen(inbuf);
						if (debugspeech) {
							printf("\nftpGet: received(%d): %s\n", got, inbuf);
							NutSleep(100);
						}
						memset(inbuf, 0, inbuffersize);
						//NutSleep(0);
					}
					fflush(stream);
					fclose(stream);
				}
			}
			free(inbuf);

	   	NutTcpCloseSocket(sock);
	   	if (debugspeech) {
				printf("\nftpGet: Ftp connection closed!\n");
				NutSleep(100);
			}
	   	sock = 0;
		}
	}
}

int sampleToFlash(char * samplefile, unsigned char* storebuf, int samplefilesize)
{	
	int error, samplesize, startpage, readbytes;
	error = -1;
	startpage = pagecounter;
	int receivecounter = 44, x;
	unsigned char merkki[FLASHPAGESIZE];

	if(XflashInit()) {
		NutSleep(100);
		if (debugspeech) {
			printf("\nsampleToFlash: DataFlash initialized OK!\n");
			NutSleep(100);
			
			printf("\nsampleToFlash: Storing %s to flash!\n", samplefile);
			NutSleep(100);
		}
		for (;;) {
			memset(merkki, 0, FLASHPAGESIZE);
			    	 
			for(x=0;x < FLASHPAGESIZE; x++) {
				merkki[x] = storebuf[receivecounter];
				receivecounter++;
				if (receivecounter == samplefilesize) {
					readbytes = x;
					break;
				}
			}
			error = PageWrite (pagecounter, merkki, FLASHPAGESIZE, 0);
			if (!error) {
				pagecounter++;
				if (pagecounter > 2048)
					return(-1);
				samplesize = readbytes;
			}
			else {
				printf("\nsampleToFlash: error while writing to flash - retrying\n");
				NutSleep(100);
				error = PageWrite (pagecounter, merkki, FLASHPAGESIZE, 0);
				if (!error) {
					pagecounter++;
					if (pagecounter > 2048)
						return(-1);
					samplesize = readbytes;
				}
				else {
					printf("\nsampleToFlash: error again!\n");
					NutSleep(100);
					error = -1;
					break;
				}
			}
			if (receivecounter == samplefilesize) {
				if (debugspeech) {
					printf("\nsampleToFlash: Done copying file %s\n", samplefile);
					NutSleep(100);
				}
				error = 0;
				break;
			}
		}
		//printf("\nsampleToFlash: File: %s, Startpage: %d, Currentpage: %d, Offset: %d, Filesize: %d\n", samplefile, startpage, pagecounter, samplesize, samplefilesize);
		//NutSleep(100);
		diphonetable2[diphonerow].diphonename = malloc(5);
		strncpy(diphonetable2[diphonerow].diphonename, samplefile, strlen(samplefile)+1);
		diphonetable2[diphonerow].startpage = startpage;
		diphonetable2[diphonerow].pagesize = pagecounter-startpage;
		diphonetable2[diphonerow].pageoffset = samplesize;
		diphonetable2[diphonerow].filesize = samplefilesize;
	
		if(strlen(diphonetable2[diphonerow].diphonename) == 2 &&
			diphonetable2[diphonerow].diphonename[1] != '_')
			diphonetable2[diphonerow].addbreak = 0;
		else diphonetable2[diphonerow].addbreak = 0;
		
		if (debugspeech) {
			printf("\nsampleToFlash: File[%d]: %s, Startpage: %d, Pagesize: %d, Offset: %d, Filesize: %d\n", 
			diphonerow, diphonetable2[diphonerow].diphonename, diphonetable2[diphonerow].startpage, 
			diphonetable2[diphonerow].pagesize, diphonetable2[diphonerow].pageoffset, diphonetable2[diphonerow].filesize);
			NutSleep(100);
		}
		diphonerow++;
	}
	else {
		printf("\nsampleToFlash: DataFlash initialization FAILED!\n");
		NutSleep(100);
		error = -1;
	}

	return(error);
}

void InitEthernetDevice(void)
{
		/*
		NutRegisterDevice(&DEV_ETHER, 0x8300, 5);
    printf("\nInitEthernetDevice: Configure eth0...");
		NutSleep(100);
		*/
    //if (NutDhcpIfConfig("eth0", 0, 60000)) {
        u_char mac[6] = {FTP_MY_MAC};

        printf("\nInitEthernetDevice: initial boot...");
				NutSleep(100);
        u_long ip_addr = inet_addr(FTP_MY_IPADDR);
        u_long ip_mask = inet_addr(FTP_MY_IPMASK);
        //u_long ip_gate = inet_addr(FTP_MY_IPGATE);

        NutNetIfConfig("eth0", mac, ip_addr, ip_mask);
        /* Without DHCP we had to set the default gateway manually.*/
				/*
				if(ip_gate) {
            printf("\nInitEthernetDevice: hard coded gate...");
						NutSleep(100);
            NutIpRouteAdd(0, 0, ip_gate, &DEV_ETHER);
        }
				*/
    //}
    printf("\nInitEthernetDevice: Initialized OK IP: %s\n", FTP_MY_IPADDR);
		NutSleep(100);
}


int AnttiFunktio (char *oneword, char *newdiphones[])
{
	char tempdiphone[3] = { '0', '0', '\0' };
	char revdiphone[3] = { '0', '0', '\0' };
	char tempchar[2] = {'0', '\0'};
	int cntr;
	int cntr2=0;
	if (debugspeech) {
		printf("\nAnttiFunktio: oneword(%d): %s\n", strlen(oneword), oneword);
		NutSleep(100);
	}
	for (cntr=0; cntr<strlen(oneword); cntr++)
	{
		tempchar[0] = oneword[cntr];
		tempchar[1] = '\0';

		if (strlen(oneword)-cntr>=2) 
			{
			tempdiphone[0] = oneword[cntr];
			tempdiphone[1] = oneword[cntr+1];
			tempdiphone[2] = '\0';
			if (!findSample(tempdiphone))
				{
					if (debugspeech) {
						printf("\nAnttiFunktio: tempdiphone(%d): %s\n", strlen(tempdiphone), tempdiphone);
						NutSleep(100);
					}
					newdiphones[cntr2] = (char*)malloc(strlen(tempdiphone)+1);
					strncpy((char*)newdiphones[cntr2], tempdiphone, strlen(tempdiphone)+1);
					cntr++;
					cntr2++;
				}
			else 
				{
				revdiphone[0] = tempdiphone[1];
				revdiphone[1] = tempdiphone[0];
				revdiphone[2] = '\0';
				if (!findSample(revdiphone))
					{
						if (debugspeech) {
							printf("\nAnttiFunktio: revdiphone(%d): %s\n", strlen(revdiphone), revdiphone);
							NutSleep(100);
						}
						newdiphones[cntr2] = (char*)malloc(strlen(revdiphone)+2);
						strncpy((char*)newdiphones[cntr2], revdiphone, strlen(revdiphone)+1);
						newdiphones[cntr2][3] = '1';
						cntr2++;
					}
				else
					{
					if (!findSample(tempchar))
						{
							if (debugspeech) {
								printf("\nAnttiFunktio: tempchar(%d): %s\n", strlen(tempchar), tempchar);
								NutSleep(100);
							}
							newdiphones[cntr2] = (char*)malloc(strlen(tempchar)+1);
							strncpy((char*)newdiphones[cntr2], tempchar, strlen(tempchar)+1);
							cntr2++;
						}
					else
						printf("\nfindSample: The sample wasn't found!\n");
					}
				}
			}
		else 
			{
			if (!findSample(tempchar))
				{
					if (debugspeech) {
						printf("\nAnttiFunktio: tempchar(%d): %s\n", strlen(tempchar), tempchar);
						NutSleep(100);
					}
					newdiphones[cntr2] = (char*)malloc(strlen(tempchar)+1);
					strncpy((char*)newdiphones[cntr2], tempchar, strlen(tempchar)+1);
					cntr2++;
				}
			else
				printf("\nfindSample: The sample wasn't found!\n");
			}
		//printf("\nAnttiFunktio: tempdiphone(%d): %s, revdiphone(%d) %s, tempchar(%d): %s\n", 
		//	strlen(tempdiphone), tempdiphone, strlen(revdiphone), revdiphone, strlen(tempchar), tempchar);
		memset(tempdiphone, 0, 2);
		memset(revdiphone, 0, 2);
		memset(tempchar, 0, 1);
	}	
	if (cntr2 > 0)
		return cntr2;
	else return -1;
}	

int sampleFromFlash(char * samplefile, unsigned char * buffer)
{
	int bytesize, error, samplesize, filesize, startpage, curpage, pagesize;
	unsigned char merkki[FLASHPAGESIZE];
	error = findSample(samplefile);
	if (!error) {
		startpage  = diphonetable2[diphonerow].startpage;
		pagesize   = diphonetable2[diphonerow].pagesize;
		samplesize = diphonetable2[diphonerow].pageoffset;
		filesize   = diphonetable2[diphonerow].filesize;

		if (debugspeech) {
			printf("\sampleFromFlash: File: %s, Startpage: %d, Pagesize: %d, Offset: %d, Filesize: %d\n", samplefile, startpage, pagesize, samplesize, filesize);
			NutSleep(100);
		}
		bytesize = 0;
		if(XflashInit()) {
			NutSleep(100);
			if (debugspeech) {
				printf("\sampleFromFlash: DataFlash initialized OK!\n");
				NutSleep(100);
			}				
			bytesize = sizeof(merkki);
			for (curpage = startpage; curpage < startpage+pagesize; curpage++) {
				if (curpage == (startpage+pagesize-1))
					bytesize = samplesize;
				if (debugspeech) {
					printf("\nsampleFromFlash: reading from page %d, bytesize %d\n", curpage, bytesize);
					NutSleep(100);				
				}
				error = PageRead(curpage, merkki, bytesize, 0);
				if (!error) {
					strncat(buffer, merkki, bytesize);
					if (debugspeech) {
						printf("\nsampleFromFlash: bytes stored to buffer\n");
						NutSleep(100);
					}
				}
				else {
					printf("\sampleFromFlash: Error reading from DataFlash\n");
					NutSleep(100);
					break;
				}
			}
			if (!error)
				if (debugspeech) {
					printf("\sampleFromFlash: Done reading file %s\n", samplefile);
					NutSleep(100);
				}
		}
		else {
			printf("\sampleFromFlash: DataFlash initialization FAILED!\n");
			NutSleep(100);
		}	
	}
	return(error);
}

void reloadDiphones(void) {
	diphonetable2[0].diphonename = "a";
	diphonetable2[diphonerow].startpage = 0;
	diphonetable2[diphonerow].pagesize = 5;
	diphonetable2[diphonerow].pageoffset = 127;
	diphonetable2[diphonerow].filesize = 1228;

	diphonetable2[1].diphonename = "b";
	diphonetable2[diphonerow].startpage = 5;
	diphonetable2[diphonerow].pagesize = 5;
	diphonetable2[diphonerow].pageoffset = 35;
	diphonetable2[diphonerow].filesize = 1136;

	diphonetable2[2].diphonename = "d";
	diphonetable2[diphonerow].startpage = 10;
	diphonetable2[diphonerow].pagesize = 6;
	diphonetable2[diphonerow].pageoffset = 217;
	diphonetable2[diphonerow].filesize = 1582;

	diphonetable2[3].diphonename = "e";
	diphonetable2[diphonerow].startpage = 16;
	diphonetable2[diphonerow].pagesize = 6;
	diphonetable2[diphonerow].pageoffset = 119;
	diphonetable2[diphonerow].filesize = 1484;
}
