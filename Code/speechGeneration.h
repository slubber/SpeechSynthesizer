/* * Header file for speechGeneration
 * * SOP Group 28
 *   Teemu Autio, Jussi Pollari, Antti Seppälä
 *
/* @{ */
void ftpGetFiles(int debugenable);
int nextSample(void);
void changeChar(char *, int);
int sampleToFlash(char * samplefile, unsigned char*storebuf, int samplefilesize);
void setup_pwm(unsigned int);
int sampleFromFlash2(char * samplefile, unsigned char * buffer, int reverse);
int sampleFromFlash(char * samplefile, unsigned char * buffer);
int findSample(char * samplefile);
void speakSentence(char * sentence, int debugenable);
void speakWord(char * oneword);
void ftpGet(void);
void ftpReceiveFile(FILE *stream, char *inbuf, char *filename, u_long ftp_addr);
void InitEthernetDevice(void);
int AnttiFunktio (char *oneword, char *newdiphones[]);
void reloadDiphones(void);

/* Timer1 interrupt */
#define enable_timer1()	TIMSK |= _BV(TOIE1);					/**< Enable timer1 interrupt(TOV) */
#define disable_timer1() TIMSK &= ~(_BV(TOIE1));OCR1A=0x00;		/**< Disable timer1 interrupt(TOV) */

/* System clock (frequency) */
#define SYSTEMCLOCK 14745600

/* @} */
struct diphonerecord
{
	char *diphonename;
	int startpage;
	int pagesize;
	int pageoffset;
	int filesize;
	int addbreak;
};
