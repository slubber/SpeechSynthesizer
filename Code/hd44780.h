#ifndef _DEV_HD44780_H_CWI
#define _DEV_HD44780_H_CWI
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
 * 3. All advertising materials mentioning features or use of this
 *    software must display the following acknowledgement:
 *
 *    This product includes software developed by egnite Software GmbH
 *    and its contributors.
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
 * -
 * Portions Copyright (C) 2001 Jesper Hansen <jesperh@telia.com>.
 * 
 * This file is part of the yampp system.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/*
 * $Log: hd44780.h,v $
 * Revision 1.3  2005/09/21 13:04:46  chwieser
 * *** empty log message ***
 *
 * Revision 1.2  2005/09/21 08:07:15  chwieser
 * *** empty log message ***
 *
 * Revision 1.1  2005/08/22 12:57:04  chwieser
 * Initial revision
 *
 * Revision 1.2  2005/05/27 14:02:01  olereinhardt
 * Added support for new display sizes configurable by macros
 * LCD_4x20, LCD_4x16, LCD_2x40, LCD_2x20, LCD_2x16, LCD_2x8,
 * LCD_1x20, LCD_1x16, LCD_1x8, KS0078_CONTROLLER (4x20))
 * Also added support for different delay types.
 * For not to wait busy too long, I added support for busy bit
 * read back and use NutSleep instead NutDelay if NUT_CPU_FREQ
 * is defined.
 *
 * Revision 1.1.1.1  2003/05/09 14:41:05  haraldkipp
 * Initial using 3.2.1
 *
 * Revision 1.2  2003/05/06 18:40:43  harald
 * Cleanup
 *
 * Revision 1.1  2003/03/31 14:53:23  harald
 * Prepare release 3.1
 *
 */

#include <sys/device.h>

#define LCD_4x16

#undef  LCD_DATA_PORT
#undef  LCD_DATA_DDR
#undef  LCD_DATA_BITS
#undef  LCD_DATA_PIN

#define LCD_DATA_PORT   PORTD   /*!< Port output register of \ref LCD_DATA_BITS. */
#define LCD_DATA_DDR    DDRD    /*!< Data direction register of \ref LCD_DATA_BITS. */
#define LCD_DATA_BITS   0xF0    /*!< \brief LCD data lines, either upper or lower 4 bits. */
#define LCD_DATA_PIN    PIND    /*!< Port input register of \ref LCD_DARA_BITS. */

#undef  LCD_ENABLE_PORT
#undef  LCD_ENABLE_DDR
#undef  LCD_ENABLE_BIT

#define LCD_ENABLE_PORT PORTD   /*!< Port output register of \ref LCD_ENABLE_BIT. */
#define LCD_ENABLE_DDR  DDRD    /*!< Data direction register of \ref LCD_ENABLE_BIT. */
#define LCD_ENABLE_BIT  3       /*!< \brief LCD enable output. */

#undef  LCD_RW_PORT
#undef  LCD_RW_DDR
#undef  LCD_RW_BIT

#define LCD_RW_PORT     PORTE   /*!< Port output register of \ref LCD_RW_BIT. */
#define LCD_RW_DDR      DDRE    /*!< Data direction register of \ref LCD_RW_BIT. */
#define LCD_RW_BIT      0       /*!< \brief LCD read/write output. */

#undef  LCD_REGSEL_PORT
#undef  LCD_REGSEL_DDR
#undef  LCD_REGSEL_BIT

#define LCD_REGSEL_PORT PORTE   /*!< Port output register of \ref LCD_REGSEL_BIT. */
#define LCD_REGSEL_DDR  DDRE    /*!< Data direction register of \ref LCD_REGSEL_BIT. */
#define LCD_REGSEL_BIT  2       /*!< \brief LCD register select output. */

/*@}*/


/*!
 * \addgroup xgDevDisplay
 */
/*@{*/

/*
 * \brief LCD Display definitions
 */

#define LCD_4x16
 
#undef  LCD_DATA_PORT
#undef  LCD_DATA_DDR
#undef  LCD_DATA_BITS
#undef  LCD_DATA_PIN

#define LCD_DATA_PORT   PORTD   /*!< Port output register of \ref LCD_DATA_BITS. */
#define LCD_DATA_DDR    DDRD    /*!< Data direction register of \ref LCD_DATA_BITS. */
#define LCD_DATA_BITS   0xF0    /*!< \brief LCD data lines, either upper or lower 4 bits. */
#define LCD_DATA_PIN    PIND    /*!< Port input register of \ref LCD_DARA_BITS. */

#undef  LCD_ENABLE_PORT
#undef  LCD_ENABLE_DDR
#undef  LCD_ENABLE_BIT

#define LCD_ENABLE_PORT PORTD   /*!< Port output register of \ref LCD_ENABLE_BIT. */
#define LCD_ENABLE_DDR  DDRD    /*!< Data direction register of \ref LCD_ENABLE_BIT. */
#define LCD_ENABLE_BIT  3       /*!< \brief LCD enable output. */

#undef  LCD_RW_PORT
#undef  LCD_RW_DDR
#undef  LCD_RW_BIT

#define LCD_RW_PORT     PORTD   /*!< Port output register of \ref LCD_RW_BIT. */
#define LCD_RW_DDR      DDRD    /*!< Data direction register of \ref LCD_RW_BIT. */
#define LCD_RW_BIT      0       /*!< \brief LCD read/write output. */

#undef  LCD_REGSEL_PORT
#undef  LCD_REGSEL_DDR
#undef  LCD_REGSEL_BIT

#define LCD_REGSEL_PORT PORTD   /*!< Port output register of \ref LCD_REGSEL_BIT. */
#define LCD_REGSEL_DDR  DDRD    /*!< Data direction register of \ref LCD_REGSEL_BIT. */
#define LCD_REGSEL_BIT  2       /*!< \brief LCD register select output. */


// HD44780 Commandset
#define LCD_CLR             0      // DB0: clear display
#define LCD_HOME            1      // DB1: return to home position
#define LCD_ENTRY_MODE      2      // DB2: set entry mode
#define LCD_ENTRY_INC       1      //   DB1: increment
#define LCD_ENTRY_SHIFT     0      //   DB2: shift
#define LCD_ON_CTRL         3      // DB3: turn lcd/cursor on
#define LCD_ON_DISPLAY      2      //   DB2: turn display on
#define LCD_ON_CURSOR       1      //   DB1: turn cursor on
#define LCD_ON_BLINK        0      //   DB0: blinking cursor
#define LCD_MOVE            4      // DB4: move cursor/display
#define LCD_MOVE_DISP       3      //   DB3: move display (0-> move cursor)
#define LCD_MOVE_RIGHT      2      //   DB2: move right (0-> left)
#define LCD_FUNCTION        5      // DB5: function set
#define LCD_FUNCTION_8BIT   4      //   DB4: set 8BIT mode (0->4BIT mode)
#define LCD_FUNCTION_2LINES 3      //   DB3: two lines (0->one line)
#define LCD_FUNCTION_RE     2      //   DB2: KS0073 Controller: Extended Register
#define LCD_FUNCTION_10DOTS 2      //   DB2: 5x10 font (0->5x7 font)
#define LCD_FUNCTION_DS     1      //   DB1: DisplayShift / DotScroll
#define LCD_FUNCTION_REV    0      //   DB0: Reverse Display
#define LCD_EXT             3      // DB3: Extended Register Set
#define LCD_EXT_FONT        2      //   DB2: Fontwidth: 5 / 6 Pixel
#define LCD_EXT_INVCURS     1      //   DB1: Normal / Inverted Cursor
#define LCD_EXT_4LINES      0      //   DB0: 1/2 Lines (normal) or 4Lines
#define LCD_CGRAM           6      // DB6: set CG RAM address
#define LCD_DDRAM           7      // DB7: set DD RAM address
// reading:
#define LCD_BUSY            7      // DB7: LCD is busy

#undef LCD_RW_BIT

#ifndef LCD_SHORT_DELAY
#define LCD_SHORT_DELAY 1
#endif

#ifndef LCD_LONG_DELAY
#define LCD_LONG_DELAY  2
#endif

/*@}*/

extern NUTDEVICE devLcd;

void LcdInit(NUTDEVICE *dev);
void LcdClear(void);
void LcdWriteData(u_char ch);
void LcdSetCursor(u_char pos);
#endif
