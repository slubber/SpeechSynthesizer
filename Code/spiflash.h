#ifndef _DEV_SPIFLASH_H_
#define _DEV_SPIFLASH_H_

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
 */

#include <sys/device.h>
//#include <fs/typedefs.h>
#include <dev/wlantypes.h>

/*!
 * \file dev/spiflash.h
 * \brief SPI flash definitions.
 */

extern int SpiFlashEnable(void);
extern void SpiFlashId(u_char *id);
extern int SpiFlashWriteByte(u_char high, u_short addr, u_char data);
extern int SpiFlashWriteWord(u_short addr, u_short data);
extern void SpiFlashErase(void);
int XflashInit (void);
BYTE SpiMemStatus(void);
int PageRead (WORD wPage, BYTE *pBuffer, WORD wSize, WORD wOffset);
int PageWrite (WORD wPage, BYTE *pBuffer, WORD wSize, WORD wOffset);
void ReadBlock (BYTE *pBuffer, WORD wSize);
void WriteBlock (BYTE *pBuffer, WORD wSize);
/* 
 * SPI memory chip select, see schematics. 
 */
#define SPIMEM_CS_PORT      PORTB
#define SPIMEM_CS_DDR       DDRB
#define SPIMEM_CS           0x10


#define SPI_CS_LOW()        PORTB |= SPIMEM_CS
#define SPI_CS_HIGH()       PORTB &= ~SPIMEM_CS

#define XFLASH_OK       0
#define XFLASH_ERROR    -1

#endif
