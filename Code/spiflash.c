/*
 * Copyright (C) 2002 by egnite Software GmbH. All rights reserved.
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
 * $Log: spiflash.c,v $
 * Revision 1.1  2005/07/26 18:02:40  haraldkipp
 * Moved from dev.
 *
 * Revision 1.2  2004/03/18 14:06:52  haraldkipp
 * Deprecated header file replaced
 *
 * Revision 1.1.1.1  2003/05/09 14:40:50  haraldkipp
 * Initial using 3.2.1
 *
 * Revision 1.4  2003/02/04 17:50:54  harald
 * Version 3 released
 *
 * Revision 1.3  2002/09/15 16:39:44  harald
 * *** empty log message ***
 *
 * Revision 1.2  2002/08/08 17:20:47  harald
 * Imagecraft support by Klaus Ummenhofer
 *
 */

#include <string.h>

#include <sys/atom.h>
#include <sys/event.h>
#include <sys/timer.h>
#include <sys/heap.h>
#include <sys/device.h>

#include "spiflash.h"

/*!
 * \addtogroup xgSpiFlash
 */
/*@{*/

/*!
 * \brief Exchange SPI byte.
 */
#ifdef __GNUC__
static __inline u_char SpiByte(u_char c)
#else
static u_char SpiByte(u_char c)
#endif
{
    outp(c, SPDR);
    loop_until_bit_is_set(SPSR, SPIF);
    return inp(SPDR);
}

/*!
 * \brief Enable SPI device flash programming.
 *
 * \return 0 if device could be located, -1 otherwise.
 */
int SpiFlashEnable(void)
{
    u_char i;
    u_char rc;

    /*
     * PB0(O): SS
     * PB1(O): SCK
     * PB2(O): MOSI
     * PB3(I): MISO
     *
     * PB4(O): Reset target
     * PB5(-): Unused
     * PB6(-): Unused
     * PB7(-): Unused
     */

    /*
     * SCK and MOSI outputs need configuration, 
     * even if SPI mode is enabled.
     */
    cbi(PORTB, 1);
    sbi(DDRB, 1);
    cbi(PORTB, 2);
    sbi(DDRB, 2);

    /*
     * Enable pull-up on MISO.
     */
    sbi(PORTB, 3);

    for (i = 0; i < 32; i++) {

        /*
         * Set reset low.
         */
        cbi(PORTB, 4);
        sbi(DDRB, 4);

        /*
         * Set slave select pin  to output. Otherwise a low signal 
         * on this pin might force us to SPI slave mode.
         */
        sbi(DDRB, 0);
        outp(BV(MSTR) | BV(SPE) | BV(SPR0), SPCR);

        /*
         * Try to enable programming.
         */
        SpiByte(0xAC);
        SpiByte(0x53);
        rc = SpiByte(0xFF);
        SpiByte(0xff);

        if (rc == 0x53)
            return 0;

        /*
         * Programming enable failed. This may be because the
         * target is not synchronized. A positive pulse on the
         * clock line should help.
         */
        outp(0, SPCR);
        sbi(PORTB, 1);
        cbi(PORTB, 1);
    }
    return -1;
}

/*!
 * Read SPI device ID.
 *
 * \param id Three byte character array, which receives
 *           the CPU ID.
 */
void SpiFlashId(u_char * id)
{
    u_char i;

    for (i = 0; i < 3; i++) {
        SpiByte(0x30);
        SpiByte(0x00);
        SpiByte(i);
        id[i] = SpiByte(0x00);
    }
}

/*!
 * \brief Write byte to the target's flash memory.
 *
 * The target must have been erased by a previous call
 * to SpiFlashErase().
 *
 * \param high Must be 0 to write the low byte or 8 to
 *             write the high byte.
 * \param addr Word address to write to.
 * \param data Byte value to write.
 *
 * \return 0 on success, -1 otherwise.
 */
int SpiFlashWriteByte(u_char high, u_short addr, u_char data)
{
    u_char d;

    if (data != 0xff) {
        SpiByte(0x40 | high);
        SpiByte(addr >> 8);
        SpiByte(addr & 0xFF);
        SpiByte(data);

        /*
         * During programming a value of 0x7F appears at the memory location. 
         * If we are programming this value, we delay execution by 10 ms.
         * Otherwise we poll the memory location until we read back the 
         * programmed value.
         */
        if (data == 0x7f)
            NutDelay(10);
        else {
            for (d = 0; d < 255; d++) {

                /*
                 * Read program flash byte.
                 */
                SpiByte(0x20 | high);
                SpiByte(addr >> 8);
                SpiByte(addr & 0xFF);
                if (SpiByte(0xFF) == data)
                    break;
            }
            if (d == 255)
                return -1;
        }
    }
    return 0;
}

/*!
 * \brief Write word to the target's flash memory.
 *
 * \param addr Word address to write to.
 * \param data Word value to write.
 *
 * \return 0 on success, -1 otherwise.
 */
int SpiFlashWriteWord(u_short addr, u_short data)
{
    if (SpiFlashWriteByte(0, addr, data & 0xFF))
        return -1;
    if (SpiFlashWriteByte(8, addr, data >> 8))
        return -1;

    return 0;
}

/*!
 * \brief Erase target's flash memory.
 *
 * Sets all bytes on the target's flash memory to 0xFF.
 * In addtion all lock bits are set to 1 (unprogrammed).
 */
void SpiFlashErase(void)
{
    /*
     * Send chip erase command.
     */
    SpiByte(0xAC);
    SpiByte(0x80);
    SpiByte(0x00);
    SpiByte(0x00);
    NutDelay(25);
}

int XflashInit (void)
{
  int           nError = -1;
  volatile BYTE bValue;

  /*
   * Init SPI memory chip select
   */     
  SPIMEM_CS_PORT &= ~SPIMEM_CS;
  SPIMEM_CS_DDR  |=  SPIMEM_CS;
     
  /* 
   * Init SS pin. When configured as input, we will lose master
   * mode, if this pin goes low. Thus we switch on the pull-up.
   */
  if(bit_is_clear(DDRB, 0)) {
    sbi(PORTB, 0);
  }

  /* Set SCK output. */
  sbi(DDRB, 1);
  /* Set MOSI output. */
  sbi(DDRB, 2);
  /* Enable MISO pullup. */
  sbi(PORTB, 3);

  /* Set double speed. */
  SPCR = (1 << SPI2X);

  SPCR = (1 << SPE) | (1 << MSTR);  /* enable SPI as master, set clk divider */
                                    /* set to max speed */
  
  /* Clean-up the status. */
  outb(SPSR, inb(SPSR));
  bValue = inb(SPDR);
    
  /* Read the status register for a rudimentary check. */
  /* Check if this is a AT45DB041B                     */
  bValue = SpiMemStatus();
  if ((bValue & 0x80) == 0x80) {
    bValue = (bValue >> 2) & 0x0F;
    if(bValue == 7) {
      nError = 1;
    }
  }

  return (nError); 
}

BYTE SpiMemStatus(void)
{
  BYTE bData;

  SPI_CS_LOW();
  
  SpiByte(0xD7);
  bData = SpiByte(0);
  
  SPI_CS_HIGH();
  
  return (bData);
} 

void WriteBlock (BYTE *pBuffer, WORD wSize)
{
  while(wSize--) {
    outb(SPDR, *pBuffer);
    pBuffer++;
    while ((inb(SPSR) & 0x80) == 0);
  }
} /* WriteBlock */

/***************************************************************************/
/*  ReadBlock                                                              */
/*                                                                         */
/*  Read a block with the size wSize from the spi bus.                     */
/***************************************************************************/
void ReadBlock (BYTE *pBuffer, WORD wSize)
{
  while(wSize--) {
    outb(SPDR, 0x00);
    while ((inb(SPSR) & 0x80) == 0);
    *pBuffer = inb(SPDR);
    pBuffer++;
  }
} /* ReadBlock */

/***************************************************************************/
/*  PageWrite                                                              */
/*                                                                         */
/*  Write a block of data with the size of wSize to the page wPage.        */
/***************************************************************************/
int PageWrite (WORD wPage, BYTE *pBuffer, WORD wSize, WORD wOffset)
{
  BYTE bByte1;
  BYTE bByte2;
  BYTE bByte3;
  
  bByte1 = LOBYTE(wPage >> 7);
  
  bByte2 = LOBYTE(wPage << 1);  
  if (wOffset & 0x100) {
    bByte2 |= 0x01;
  }
  
  bByte3 = LOBYTE(wOffset);
  
  SPI_CS_LOW();

  /* Select mode */
  SpiByte(0x82);
  SpiByte(bByte1);
  SpiByte(bByte2);
  SpiByte(bByte3);
  
  /* Write data */
  WriteBlock(pBuffer, wSize);

  SPI_CS_HIGH();  

  while((SpiMemStatus() & 0x80) == 0);

  return (XFLASH_OK);
} /* PageWrite */

/***************************************************************************/
/*  PageRead                                                               */
/*                                                                         */
/*  Read a block of data with the size of wSize from the page wPage.       */
/***************************************************************************/
int PageRead (WORD wPage, BYTE *pBuffer, WORD wSize, WORD wOffset)
{
  BYTE bByte1;
  BYTE bByte2;
  BYTE bByte3;    
  
  bByte1 = LOBYTE(wPage >> 7);
  
  bByte2 = LOBYTE(wPage << 1);  
  if (wOffset & 0x100) {
    bByte2 |= 0x01;
  }
  
  bByte3 = LOBYTE(wOffset);

  while((SpiMemStatus() & 0x80) == 0);

  SPI_CS_LOW();  

  SpiByte(0xE8);
  SpiByte(bByte1);
  SpiByte(bByte2);
  SpiByte(bByte3);
  SpiByte(0x00);
  SpiByte(0x00);
  SpiByte(0x00);
  SpiByte(0x00);
  ReadBlock(pBuffer, wSize);

  SPI_CS_HIGH();  

  return (XFLASH_OK);
}
