#ifndef __io_gpio_cpu_h__
#define __io_gpio_cpu_h__
/**HEADER********************************************************************
* 
* Copyright (c) 2008 Freescale Semiconductor;
* All Rights Reserved
*
*************************************************************************** 
*
* THIS SOFTWARE IS PROVIDED BY FREESCALE "AS IS" AND ANY EXPRESSED OR 
* IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES 
* OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  
* IN NO EVENT SHALL FREESCALE OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
* INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES 
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, 
* STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
* IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
* THE POSSIBILITY OF SUCH DAMAGE.
*
**************************************************************************
*
* $FileName: io_gpio_mcf532.h$
* $Version : 3.7.2.0$
* $Date    : Feb-7-2011$
*
* Comments:
*
*   The file contains definitions used in user program and/or in other
*   kernel modules to access GPIO pins
*
*END************************************************************************/

/*----------------------------------------------------------------------*/
/*
**                          CONSTANT DEFINITIONS
*/

#ifdef __cplusplus
extern "C" {
#endif

#define GPIO_PORT_FECH          ((0x0000 << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_FECL          ((0x0001 << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_SSI           ((0x0002 << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_BUSCTL        ((0x0003 << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_BE            ((0x0004 << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_CS            ((0x0005 << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_PWM           ((0x0006 << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_FECI2C        ((0x0007 << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_UART          ((0x0009 << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_QSPI          ((0x000A << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_TIMER         ((0x000B << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_LCDDATAH      ((0x000D << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_LCDDATAM      ((0x000E << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_LCDDATAL      ((0x000F << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_LCDCTLH       ((0x0010 << 3) | GPIO_PIN_VALID)
#define GPIO_PORT_LCDCTLAL      ((0x0011 << 3) | GPIO_PIN_VALID)

#ifdef __cplusplus
}
#endif

#endif

/* EOF */
