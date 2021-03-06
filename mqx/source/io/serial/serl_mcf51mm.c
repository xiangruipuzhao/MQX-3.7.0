/**HEADER********************************************************************
* 
* Copyright (c) 2008 Freescale Semiconductor;
* All Rights Reserved
*
* Copyright (c) 2004-2008 Embedded Access Inc.;
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
* $FileName: serl_mcf51mm.c$
* $Version : 3.6.2.0$
* $Date    : Jun-4-2010$
*
* Comments:
*
*   This file contains board-specific serial initialization functions.
*
*END************************************************************************/
#include <user_config.h>
#include <mqx.h>
#include <bsp.h>
#include "serl_mcf51xx.h"

#if (BSPCFG_ENABLE_ITTYA || BSPCFG_ENABLE_ITTYB || BSPCFG_ENABLE_TTYA || BSPCFG_ENABLE_TTYB)

/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : _bsp_get_serial_base_address
* Returned Value   : Address upon success, NULL upon failure
* Comments         :
*    This function returns the base register address of the corresponding UART
*
*END*----------------------------------------------------------------------*/
pointer _bsp_get_serial_base_address
(
    uint_8 channel
)
{
   switch (channel) {
   case MCF51XX_SCI0:
      return (pointer)(&((VMCF51MM_STRUCT_PTR)BSP_IPSBAR)->SCI1);
   break;
   case MCF51XX_SCI1:
      return (pointer)(&((VMCF51MM_STRUCT_PTR)BSP_IPSBAR)->SCI2);
   break;
   break;
   default:
   break;
   }
   
   return NULL;
}

#endif // (BSPCFG_ENABLE_ITTYA || BSPCFG_ENABLE_ITTYB || BSPCFG_ENABLE_TTYA || BSPCFG_ENABLE_TTYB)/* EOF */
