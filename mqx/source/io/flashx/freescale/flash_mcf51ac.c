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
* $FileName: flash_mcf51ac.c$
* $Version : 3.6.2.0$
* $Date    : Jun-4-2010$
*
* Comments:
*
*   The file contains functions for internal flash read, write, erase 
*
*END************************************************************************/
#include <user_config.h> 
#include "mqx.h"
#include "bsp.h"
 
#include "flash_mcf51xx.h"
#include "flash_mcf51ac.h"
#if BSPCFG_ENABLE_FLASHX


/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : _bsp_get_cfm_address
* Returned Value   : Address upon success
* Comments         :
*    This function returns the base register address of the CFM
*
*END*----------------------------------------------------------------------*/
pointer _bsp_get_cfm_address()
{
   VMCF51AC_STRUCT_PTR reg_ptr = _PSP_GET_MBAR(); 
   VMCF51XX_FTSR_STRUCT_PTR ftsr_ptr = (VMCF51XX_FTSR_STRUCT_PTR)&reg_ptr->FTSR;
   return (pointer)ftsr_ptr;
}

/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : _bsp_get_pmc_address
* Returned Value   : Address upon success
* Comments         :
*    This function returns the base register address of the PMC
*
*END*----------------------------------------------------------------------*/
pointer _bsp_get_pmc_address()
{
   VMCF51AC_STRUCT_PTR reg_ptr = _PSP_GET_MBAR(); 
   uint_8_ptr pmc_ptr = (uint_8_ptr)&(reg_ptr->SIM.SPMSC1);
   return (pointer)pmc_ptr;   
}


/*FUNCTION*-------------------------------------------------------------------
*
* Function Name    : _mcf51ac_internal_flash_install
* Returned Value   : uint_32 a task error code or MQX_OK
* Comments         :
*    Install a internal flash device.
*
*END*----------------------------------------------------------------------*/
uint_32 _mcf51ac_internal_flash_install(
    /* [IN] A string that identifies the device for fopen */
    char_ptr           identifier,
    uint_32            flashx_size
) 
{ /* Body */
   return mcf51xx_internal_flash_install(identifier, flashx_size);  
} /* Endbody */
#endif

/* EOF */