/**HEADER********************************************************************
* 
* Copyright (c) 2008 Freescale Semiconductor;
* All Rights Reserved                       
*
* Copyright (c) 1989-2008 ARC International;
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
* $FileName: init_uart0.c$
* $Version : 3.6.4.0$
* $Date    : Jun-21-2010$
*
* Comments:
*
*   This file contains the definition for the baud rate for the serial
*   channel
*
*END************************************************************************/

#include "mqx.h"
#include "bsp.h"

MCF52XX_UART_SERIAL_INIT_STRUCT _bsp_uart0_init = {
   /* queue size         */ 128,
   /* Channel            */ MCF52XX_IO_UART0,
   /* Clock Speed        */ BSP_SYSTEM_CLOCK/2,
   /* Interrupt Vector   */ BSP_UART0_INT_VECTOR,
   /* Interrupt Level    */ BSP_UART0_INT_LEVEL,
   /* Interrupt Sublevel */ 0, /* not used */
   /* UMR1 Value         */ MCF5227_UART_UMR1_NO_PARITY |
                            MCF5227_UART_UMR1_8_BITS,
   /* UMR2 Value         */ MCF5227_UART_UMR2_1_STOP_BIT,
   /* Baud rate          */ BSPCFG_UART0_BAUD_RATE
};

/* EOF */
