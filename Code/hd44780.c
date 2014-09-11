/*
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
 *
 */

/*
 * $Log: hd44780.c,v $
 * Revision 1.6  2005/09/21 13:04:51  chwieser
 * *** empty log message ***
 *
 * Revision 1.5  2005/09/21 12:54:37  chwieser
 * *** empty log message ***
 *
 * Revision 1.4  2005/08/22 14:08:21  chwieser
 * *** empty log message ***
 *
 * Revision 1.3  2005/08/22 13:56:08  chwieser
 * *** empty log message ***
 *
 * Revision 1.2  2005/08/22 13:06:08  chwieser
 * *** empty log message ***
 *
 * Revision 1.1  2005/08/22 12:48:23  chwieser
 * Initial revision
 *
 * Revision 1.1  2005/08/18 14:18:26  chwieser
 * Initial revision
 *
 * Revision 1.6  2005/06/06 10:43:45  haraldkipp
 * Fixed to re-enable ICCAVR compilation.
 *
 * Revision 1.5  2005/05/27 14:02:11  olereinhardt
 * Added support for new display sizes configurable by macros
 * LCD_4x20, LCD_4x16, LCD_2x40, LCD_2x20, LCD_2x16, LCD_2x8,
 * LCD_1x20, LCD_1x16, LCD_1x8, KS0078_CONTROLLER (4x20))
 * Also added support for different delay types.
 * For not to wait busy too long, I added support for busy bit
 * read back and use NutSleep instead NutDelay if NUT_CPU_FREQ
 * is defined.
 *
 * Revision 1.4  2004/05/24 17:11:05  olereinhardt
 * dded terminal device driver for hd44780 compatible LCD displays directly
 * connected to the memory bus (memory mapped). See hd44780.c for more
 * information.Therefore some minor changed in include/dev/term.h and
 * dev/term.c are needet to
 * pass a base address to the lcd driver.
 *
 * Revision 1.3  2004/03/16 16:48:27  haraldkipp
 * Added Jan Dubiec's H8/300 port.
 *
 * Revision 1.2  2003/07/17 09:41:35  haraldkipp
 * Setting the data direction during init only may fail on some hardware.
 * We are now doing this immediately before using the port.
 *
 * Revision 1.1.1.1  2003/05/09 14:40:37  haraldkipp
 * Initial using 3.2.1
 *
 * Revision 1.3  2003/05/06 18:30:10  harald
 * ICCAVR port
 *
 * Revision 1.2  2003/04/21 16:22:46  harald
 * Moved back to outp/inp for portability
 *
 * Revision 1.1  2003/03/31 14:53:06  harald
 * Prepare release 3.1
 *
 */

#include <stdlib.h>
#include <string.h>

#include <sys/nutconfig.h>
#include <dev/hd44780.h>
#include <dev/term.h>
#include <sys/timer.h>


#include "hd44780.h"

/*!
 * \addtogroup xgDisplay
 */
/*@{*/

/*!
 * \brief Wait until controller will be ready again
 *
 * If LCD_WR_BIT is defined we will wait until the ready bit is set, otherwise 
 * We will either busy loop with NutDelay or sleep with NutSleep. The second 
 * option will be used if we have defined NUT_CPU_FREQ. In this case we have a higher 
 * timer resolution.
 *
 * \param xt Delay time in milliseconds
 */

 u_char during_init = 1;
#define LCD_DELAY _NOP(); _NOP()

#ifdef LCD_RW_BIT

 INLINE u_char LcdReadNibble(void)
{

    sbi(LCD_RW_PORT, LCD_RW_BIT);
    outp(inp(LCD_DATA_DDR) & ~LCD_DATA_BITS, LCD_DATA_DDR);   // enable data input
    sbi(LCD_ENABLE_PORT, LCD_ENABLE_BIT);
    LCD_DELAY;
    cbi(LCD_ENABLE_PORT, LCD_ENABLE_BIT);
    return inp(LCD_DATA_PIN) & LCD_DATA_BITS;
}

 INLINE u_char LcdReadByte(void)
{    
    u_char data;
#if LCD_DATA_BITS == 0x0F
    data = LcdReadNibble();
    LCD_DELAY;
    data = data | (LcdReadNibble() << 4);
    LCD_DELAY;
#elif LCD_DATA_BITS == 0xF0
    data = LcdReadNibble() >> 4;
    LCD_DELAY;
    data |= LcdReadNibble();
#elif LCD_DATA_BITS == 0xFF
    data = LcdReadNibble();
#else
#error "Bad definition of LCD_DATA_BITS"
#endif
    return data;
}

/*!
 * \brief Read command byte from LCD controller.
 */

 u_char LcdReadCmd(void)
{
    sbi(LCD_REGSEL_DDR, LCD_REGSEL_BIT);
    cbi(LCD_REGSEL_PORT, LCD_REGSEL_BIT);
    return LcdReadByte();
}

#endif


void LcdDelay(u_char xt) 
{
    if (during_init) {
        NutDelay(xt);
    } else {
#if defined(LCD_RW_BIT)
    while (LcdReadCmd() & (1 << LCD_BUSY)) 
        LCD_DELAY;
    LCD_DELAY;
    LCD_DELAY;
    LCD_DELAY;
    LCD_DELAY;
    LCD_DELAY;
    LCD_DELAY;
    LCD_DELAY;
    LCD_DELAY;
    LCD_DELAY;
    LCD_DELAY;
    LCD_DELAY;
#elif defined(NUT_CPU_FREQ)    
    NutSleep(xt);
#else
    NutDelay(xt);
#endif
    }
}

INLINE void LcdSendNibble(u_char nib)
{
#ifdef LCD_RW_BIT
    cbi(LCD_RW_PORT, LCD_RW_BIT);
#endif
    outp(inp(LCD_DATA_DDR) | LCD_DATA_BITS, LCD_DATA_DDR);
    outp((inp(LCD_DATA_PORT) & ~LCD_DATA_BITS) | (nib & LCD_DATA_BITS), LCD_DATA_PORT);
    sbi(LCD_ENABLE_PORT, LCD_ENABLE_BIT);
    LCD_DELAY; 
    cbi(LCD_ENABLE_PORT, LCD_ENABLE_BIT); 
}

/*!
 * \brief Send byte to LCD controller.
 *
 * The byte is sent to a 4-bit interface in two nibbles. If one has configured 
 * LCD_DATA_BITS to 0xFF this will send a whole byte at once
 *
 * \param ch Byte to send.
 * \param xt Delay time in milliseconds.
 */
INLINE void LcdSendByte(u_char ch, u_char xt)
{    
#if LCD_DATA_BITS == 0x0F
    LcdSendNibble(ch >> 4);
    if(xt)
        LcdDelay(xt);
    LcdSendNibble(ch);
#elif LCD_DATA_BITS == 0xF0
    LcdSendNibble(ch);
    if(xt)
        LcdDelay(xt);
    LcdSendNibble(ch << 4);
#elif LCD_DATA_BITS == 0xFF
    LcdSendNibble(ch);
#else
#error "Bad definition of LCD_DATA_BITS"
#endif
    if(xt)
        LcdDelay(xt);
}

void LcdWriteData(u_char ch)
{
    sbi(LCD_REGSEL_DDR, LCD_REGSEL_BIT);
    sbi(LCD_REGSEL_PORT, LCD_REGSEL_BIT);
    LcdSendByte(ch, LCD_SHORT_DELAY);
}

/*!
 * \brief Write command byte to LCD controller.
 */
void LcdWriteCmd(u_char cmd, u_char xt)
{
    sbi(LCD_REGSEL_DDR, LCD_REGSEL_BIT);
    cbi(LCD_REGSEL_PORT, LCD_REGSEL_BIT);
    LcdSendByte(cmd, xt);
}

void LcdSetCursor(u_char pos)
{
    u_char x = 0;
    u_char y = 0;
    
    u_char  offset  [4] = {0x80, 0xC0, 0x90, 0xD0};
    y = pos / 16;
    x = pos % 16;
    if (y>3) 
      y=3;
    pos = x + offset[y];
    LcdWriteCmd(1 << LCD_DDRAM | pos, LCD_SHORT_DELAY);
}

void LcdCursorHome(void)
{
    LcdWriteCmd(1 << LCD_HOME, LCD_LONG_DELAY);
}

void LcdCursorLeft(void)
{
    LcdWriteCmd(1 << LCD_MOVE, LCD_SHORT_DELAY);
}

void LcdCursorRight(void)
{
    LcdWriteCmd(1 << LCD_MOVE | 1 << LCD_MOVE_RIGHT, LCD_SHORT_DELAY);
}

void LcdClear(void)
{
    LcdWriteCmd(1 << LCD_CLR, LCD_LONG_DELAY);
}

void LcdCursorMode(u_char on)
{
    LcdWriteCmd(1 << LCD_ON_CTRL | on ? 1 << LCD_ON_CURSOR : 0x00, LCD_LONG_DELAY);
}

void LcdInit(NUTDEVICE *dev)
{
    /*
     * Set LCD register select and enable outputs.
     */
    sbi(LCD_REGSEL_DDR, LCD_REGSEL_BIT);
    sbi(LCD_ENABLE_DDR, LCD_ENABLE_BIT);
#ifdef LCD_RW_BIT
    sbi(LCD_RW_DDR, LCD_RW_BIT);
    cbi(LCD_RW_PORT, LCD_RW_BIT);
#endif

	
    /*
     * Send a dummy data byte.
     */
    //LcdWriteData(0);

    /*
     * Initialize for 4-bit operation.
     */
    cbi(LCD_REGSEL_PORT, LCD_REGSEL_BIT);

#if (LCD_DATA_BITS == 0xFF)     // 8 Bit initialisation
    LcdWriteCmd((1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_8BIT), 50);
    LcdWriteCmd((1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_8BIT), 50);
    LcdWriteCmd((1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_8BIT), 50);
    #ifdef  KS0073_CONTROLLER
        LcdWriteCmd((1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_8BIT) | (1 << LCD_FUNCTION_RE), LCD_SHORT_DELAY);
        LcdWriteCmd((1 << LCD_EXT) | ((((TERMDCB *) dev->dev_dcb)->dcb_nrows > 2) ? (1 << LCD_EXT_4LINES) : 0), LCD_SHORT_DELAY);
        LcdWriteCmd((1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_8BIT), LCD_SHORT_DELAY);
    #endif
    LcdWriteCmd((1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_8BIT) | ((((TERMDCB *) dev->dev_dcb)->dcb_nrows > 1) ?(1 << LCD_FUNCTION_2LINES):0), LCD_SHORT_DELAY);


#else                           // 4 Bit initialisation
    LcdSendNibble((1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_8BIT) | (((1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_8BIT)) >> 4));
    LcdDelay(50);
    LcdSendNibble((1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_8BIT) | (((1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_8BIT)) >> 4));
    LcdDelay(50);
    LcdSendNibble((1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_8BIT) | (((1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_8BIT)) >> 4));
    LcdDelay(50);
    LcdSendNibble((1 << LCD_FUNCTION) | ((1 << LCD_FUNCTION) >> 4));    // Enter 4 Bit mode
    LcdDelay(50);
    #ifdef  KS0073_CONTROLLER
        LcdWriteCmd((1 << LCD_FUNCTION) | (1 << LCD_FUNCTION_RE), LCD_SHORT_DELAY);
        LcdWriteCmd((1 << LCD_EXT) | ((((TERMDCB *) dev->dev_dcb)->dcb_nrows > 2) ? (1 << LCD_EXT_4LINES) : 0), LCD_LONG_DELAY);
        LcdWriteCmd((1 << LCD_FUNCTION), LCD_LONG_DELAY);
    #endif
    LcdWriteCmd((1 << LCD_FUNCTION) | ((((TERMDCB *) dev->dev_dcb)->dcb_nrows > 1) ? (1 << LCD_FUNCTION_2LINES):0), LCD_SHORT_DELAY);
#endif

    // clear LCD
    LcdWriteCmd(1 << LCD_CLR, LCD_LONG_DELAY);
    // set entry mode
    LcdWriteCmd(1 << LCD_ENTRY_MODE | 1 << LCD_ENTRY_INC, LCD_LONG_DELAY);
    // set display to on
    LcdWriteCmd(1 << LCD_ON_CTRL | 1 << LCD_ON_DISPLAY, LCD_LONG_DELAY);
    // move cursor to home
    LcdWriteCmd(1 << LCD_HOME, LCD_LONG_DELAY);
    // set data address to 0
    LcdWriteCmd(1 << LCD_DDRAM | 0x00, LCD_LONG_DELAY);    
    during_init = 0;
}

/*!
 * \brief Terminal device control block structure.
 */
TERMDCB dcb_term = {
    LcdInit,            /*!< \brief Initialize display subsystem, dss_init. */
    LcdWriteData,       /*!< \brief Write display character, dss_write. */
    LcdWriteCmd,        /*!< \brief Write display command, dss_command. */
    LcdClear,           /*!< \brief Clear display, dss_clear. */
    LcdSetCursor,       /*!< \brief Set display cursor, dss_set_cursor. */
    LcdCursorHome,      /*!< \brief Set display cursor home, dss_cursor_home. */
    LcdCursorLeft,      /*!< \brief Move display cursor left, dss_cursor_left. */
    LcdCursorRight,     /*!< \brief Move display cursor right, dss_cursor_right. */
    LcdCursorMode,      /*!< \brief Switch cursor on/off, dss_cursor_mode. */
    0,                  /*!< \brief Mode flags. */
    0,                  /*!< \brief Status flags. */
#ifdef LCD_4x16
    4,                  /*!< \brief Number of rows. */
    16,                 /*!< \brief Number of columns per row. */
    16,                 /*!< \brief Number of visible columns. */
#endif
};

/*!
 * \brief LCD device information structure.
 */
NUTDEVICE devLcd = {
    0,              /*!< Pointer to next device. */
    {'l', 'c', 'd', 0, 0, 0, 0, 0, 0},      /*!< Unique device name. */
    IFTYP_STREAM,   /*!< Type of device. */
    0,              /*!< Base address. */
    0,              /*!< First interrupt number. */
    0,              /*!< Interface control block. */
    &dcb_term,      /*!< Driver control block. */
    TermInit,       /*!< Driver initialization routine. */
    TermIOCtl,      /*!< Driver specific control function. */
    0,
    TermWrite,
    TermWrite_P,
    TermOpen,
    TermClose,
    0
};

/*@}*/
