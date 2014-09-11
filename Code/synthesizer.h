/* * Header for synthesizer
 * * SOP Group 28
 *   Teemu Autio, Jussi Pollari, Antti Seppälä
 *
/* @{ */

static void speechSynthesize(void);
static void runSNMP(void);
static void openSettings(void);
static void openNetworkSettings(void);
static void showHelp(void);
void readLine(char *,uint8_t );
static void lcdPrint(char *);
static void buzzer (int);
static void getDiphones(void);
void init_devices(void);
void print_main_menu(void);
void light_led(int lednro);
