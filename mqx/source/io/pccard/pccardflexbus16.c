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
* $FileName: pccardflexbus16.c$
* $Version : 3.7.2.0$
* $Date    : Feb-7-2011$
*
* Comments:
*
*   Advanced PC Card device driver functions for the PowerPC 850.
*   This replaces the original 850 PC Card driver.
*
*END************************************************************************/

#include "mqx_inc.h"
#include "bsp.h"
#include "bsp_prv.h"
#include "fio.h"
#include "io.h"
#include "io_prv.h"
#include "pccardflexbus16.h"
#include "pccardflexbuspr16.h"


/*FUNCTION*---------------------------------------------------------------------
*
* Function Name    : _io_pccard_install
* Returned Value   : uint_32 - a task error code or MQX_OK 
* Comments         : Install the PC Card device driver
*
*END*-------------------------------------------------------------------------*/

uint_32 _io_pccardflexbus_install
   (
      /* [IN] A string that identifies the device for fopen */
      char_ptr                    identifier,

      /* [IN] Pointer to initialization info. */
      PCCARDFLEXBUS_INIT_STRUCT_CPTR  init_ptr
   )
{ /* Body */
  IO_PCCARDFLEXBUS_STRUCT_PTR      info_ptr;

   /* Allocate memory for the PCCARD state structure */
  info_ptr = (IO_PCCARDFLEXBUS_STRUCT_PTR)_mem_alloc_system_zero((uint_32)sizeof(IO_PCCARDFLEXBUS_STRUCT));

#if MQX_CHECK_MEMORY_ALLOCATION_ERRORS
   if (!info_ptr) {
      return MQX_OUT_OF_MEMORY;
   } /* Endif */
#endif
   
   info_ptr->INIT_DATA = *init_ptr;

   return (_io_dev_install(identifier,
      (_mqx_int (_CODE_PTR_)(MQX_FILE_PTR, char _PTR_, char _PTR_))_io_pccardflexbus_open,
      (_mqx_int (_CODE_PTR_)(MQX_FILE_PTR))                        _io_pccardflexbus_close,
      (_mqx_int (_CODE_PTR_)(MQX_FILE_PTR, char_ptr, _mqx_int))    0,
      (_mqx_int (_CODE_PTR_)(MQX_FILE_PTR, char_ptr, _mqx_int))    0,
      (_mqx_int (_CODE_PTR_)(MQX_FILE_PTR, _mqx_uint, pointer))    _io_pccardflexbus_ioctl,
      (pointer)info_ptr));
} /* Endbody */   

/*FUNCTION*---------------------------------------------------------------------
*
* Function Name    : _io_pccardflexbus_open
* Returned Value   : 
* Comments         :
*
*END*-------------------------------------------------------------------------*/

_mqx_int _io_pccardflexbus_open
   (
      /* [IN] the file handle for the device being opened */
      FILE_DEVICE_STRUCT_PTR   fd_ptr,
      
      /* [IN] the remaining portion of the name of the device */
      char _PTR_               open_name_ptr,

      /* [IN] the flags to be used during operation (not used) */
      char _PTR_               flags
   )
{ /* Body */
   IO_DEVICE_STRUCT_PTR          io_dev_ptr = fd_ptr->DEV_PTR;
   IO_PCCARDFLEXBUS_STRUCT_PTR   info_ptr   = (IO_PCCARDFLEXBUS_STRUCT_PTR)io_dev_ptr->DRIVER_INIT_PTR;
   
   /* Initialize mini flexbus */
   _bsp_flexbus_pccard_setup((uint_32)info_ptr->INIT_DATA.PCMCIA_BASE);   

    /* Detect if card is present in slot */
   if (_io_pccardflexbus_card_detect(info_ptr)) 
   {
      _time_get_elapsed(&info_ptr->START_TIME);
      info_ptr->STATE   = PCCARDFLEXBUS_CARD_INSERTED;
      info_ptr->CARD_IN = TRUE;
   } 
   else 
   {
      info_ptr->STATE   = PCCARDFLEXBUS_CARD_REMOVED;
      info_ptr->CARD_IN = FALSE;
   } 

  /* 
   ** Allocate driver a PCMCIA CS for attribute space. This is needed for
   ** functions that access the CIS. Slot A has offset 0 while slot B has 
   ** the next offset PCCARDFLEXBUS_ATTRIB_SIZE to ensure they don't overlap.
   */
   info_ptr->PCMCIA_BASE = (volatile uint_16 _PTR_)info_ptr->INIT_DATA.PCMCIA_BASE;
   info_ptr->ATTRIB_PTR  = (volatile uint_16 _PTR_)((uint_32)info_ptr->PCMCIA_BASE | PCCARDFLEXBUS_REGISTER_MASK);

   return MQX_OK;

} /* Endbody */

/*FUNCTION*---------------------------------------------------------------------
*
* Function Name    : _io_pccardflexbus_close
* Returned Value   : 
* Comments         :
*
*END*-------------------------------------------------------------------------*/

_mqx_int _io_pccardflexbus_close
   (
      /* [IN] the file handle for the device being closed */
      FILE_DEVICE_STRUCT_PTR fd_ptr
   )
{ /* Body */
   /* Nothing to be done here */
   return MQX_OK;
} /* Endbody */

/*FUNCTION*---------------------------------------------------------------------
* 
* Function Name    : _io_pccardflexbus_ioctl
* Returned Value   : int_32
* Comments         :
*    Returns result of ioctl operation.
*
*END*-------------------------------------------------------------------------*/

_mqx_int _io_pccardflexbus_ioctl
   (
      /* [IN] the file handle for the device */
      FILE_DEVICE_STRUCT_PTR fd_ptr,

      /* [IN] the ioctl command */
      _mqx_uint              cmd,

      /* [IN] the ioctl parameters */
      pointer                input_param_ptr
   )
{ /* Body */
   APCCARD_ADDR_INFO_STRUCT_PTR   addr_ptr;
   IO_DEVICE_STRUCT_PTR           io_dev_ptr = fd_ptr->DEV_PTR;
   IO_PCCARDFLEXBUS_STRUCT_PTR    info_ptr = (IO_PCCARDFLEXBUS_STRUCT_PTR)io_dev_ptr->DRIVER_INIT_PTR;
   int_32                         result   = MQX_OK;
   uint_32_ptr                    param_ptr = input_param_ptr;
   volatile uint_16 _PTR_           tmp_ptr;

   switch (cmd) {
      case APCCARD_IOCTL_IS_CARD_INSERTED:
         *param_ptr = _io_pccardflexbus_card_detect(info_ptr);
         break;
      case APCCARD_IOCTL_GET_ADDR_SPACE:
         addr_ptr = (APCCARD_ADDR_INFO_STRUCT_PTR)((pointer)param_ptr);
         tmp_ptr  = (volatile uint_16 *)(((uint_32)info_ptr->PCMCIA_BASE | PCCARDFLEXBUS_COMMON_MEM_MASK) + addr_ptr->OFFSET);
         addr_ptr->ADDRESS = (pointer)tmp_ptr;
         break;

      case APCCARD_IOCTL_FREE_ADDR_SPACE:
         break;
      case APCCARD_IOCTL_POWERDOWN_CARD:
         break;
      case APCCARD_IOCTL_CARD_READY:
         *param_ptr = info_ptr->STATE == PCCARDFLEXBUS_ACCESS_ALLOWED;
         break;
      case APCCARD_IOCTL_WAIT_TILL_READY:
         _io_pccardflexbus_card_wait_till_ready(info_ptr);
         break;
      case APCCARD_IOCTL_READ_TUPLE:
         _io_pccardflexbus_read_tuple((uint_16 _PTR_)info_ptr->ATTRIB_PTR,
            *param_ptr, (uint_16_ptr)(param_ptr + 1));
            break;
      case IO_IOCTL_DEVICE_IDENTIFY:
         param_ptr[0] = IO_DEV_TYPE_PHYS_ADV_PCCARD;
         param_ptr[1] = 0;
         param_ptr[2] = 0;
         break;
      default:
         result = IO_ERROR_INVALID_IOCTL_CMD;
         break;
   } /* Endswitch */

   return result;

} /* Endbody */



/* Private functions */
/*FUNCTION*---------------------------------------------------------------------
*
* Function Name    : _io_pccardflexbus_card_detect
* Returned Value   : TRUE is card is in, FALSE otherwise
* Comments         :
*
*END*-------------------------------------------------------------------------*/

boolean _io_pccardflexbus_card_detect    
   (
      /* [IN] The info driver */
      IO_PCCARDFLEXBUS_STRUCT_PTR  info_ptr
   )
{ /* Body */
   volatile unsigned short card_state; 
   
   card_state = *(volatile uint_16 *)((uint_32)info_ptr->PCMCIA_BASE | PCCARDFLEXBUS_CARD_PRESENT_MASK);
   
   if(card_state == 0x01) /* card is in the slot */
      return (TRUE);
   else                   /* card is not present in the slot */
      return (FALSE);   
} /* Endbody */



/*FUNCTION*-----------------------------------------------------------------
*
* Function Name   : _io_pccardflexbus_card_wait_till_ready
* Returned Value  : none
* Comments        : waits until the card has had a chance to powerup 
*   completely.
*   
*
*END*---------------------------------------------------------------------*/

void _io_pccardflexbus_card_wait_till_ready
   (
      /* [IN] The info for this slot */
      IO_PCCARDFLEXBUS_STRUCT_PTR  info_ptr
   )
{ /* Body */

   /* Wait until the card is ready */
   if (!info_ptr->CARD_IN) {
      return;
   } /* Endif */

   _io_pccardflexbus_wait(info_ptr, PCCARDFLEXBUS_RESET_ALLOWED_TIME);

} /* Endbody */



/*FUNCTION*-----------------------------------------------------------------
*
* Function Name   : _io_pccardflexbus_card_wait
* Returned Value  : none
* Comments        : 
*   
*
*END*---------------------------------------------------------------------*/

void _io_pccardflexbus_wait
   (
      /* [IN] The info for this slot */
      IO_PCCARDFLEXBUS_STRUCT_PTR  info_ptr,

      /* [IN] The time to wait in ms */
      uint_32                    wait_ms
   )
{ /* Body */
   TIME_STRUCT   now, diff;
   uint_32       diff_ms = 0;

   while (diff_ms < wait_ms) {
      _time_get_elapsed(&now);
      _time_diff(&info_ptr->START_TIME, &now, &diff);
      diff_ms = diff.MILLISECONDS + diff.SECONDS * 1000;
   } /* Endwhile */
      
} /* Endbody */



/*FUNCTION*-----------------------------------------------------------------
*
* Function Name   : _io_apccardflexbus_read_tuple
* Returned Value  : uint_32 - actual size of tuple info data
* Comments        :
*   This function finds and reads the specified tuple
*
*END*---------------------------------------------------------------------*/

uint_32 _io_pccardflexbus_read_tuple
   (
      /* [IN] Pointer to the start of the CIS */
      volatile uint_16 _PTR_  cis_ptr,

      /* [IN] The tuple to search for */
      uint_32               tuple_code,

      /* [IN] Address to store the tuple data */
      uint_16_ptr             dest_ptr

   )
{ /* Body */
   uint_32     size;
   uint_32     i;
   volatile uint_16             cis_data;
   volatile uint_16 _PTR_       cis_ptr_base = cis_ptr;
   
   /* Skipping Null tuples */
   cis_data = *cis_ptr;
   while (cis_data == 0) {
      cis_ptr += 1;
      cis_data = *cis_ptr;
      if(cis_ptr - cis_ptr_base > 0x1000)
        return 0; /* tuple not found */
   } /* Endwhile */
   
   /* Searching for Memory descriptor tuple */
   while (cis_data != IO_APCCARD_TUPLE_CISTPL_DEVICE) {
      cis_ptr += 1;
      cis_data = *cis_ptr;
      if(cis_ptr - cis_ptr_base > 0x1000)
        return 0; /* device descriptor tuple not found */
   } /* Endwhile */


   /* search CIS for tuple_code */
   while ((uint_32)cis_data != tuple_code) {
      if (cis_data == IO_APCCARD_TUPLE_END_OF_CIS) {
         /* Reached end of CIS */
         return 0;
      } /* Endif */
      /* Get link field */
      cis_ptr += IO_APCCARD_TUPLE_LINK_CIS_OFFSET;
      if(cis_ptr - cis_ptr_base > 0x1000)
        return 0; /*  tuple not found */
      cis_data = *cis_ptr;
      /* Calculate the location of the next tuple based on link field */
      cis_ptr += (cis_data + 1);
      /* Read tuple code */
      cis_data = *cis_ptr;
   } /* Endwhile */

   /* Get link field */
   cis_data = cis_ptr[IO_APCCARD_TUPLE_LINK_CIS_OFFSET];
  
   size = (uint_32)cis_data;

   cis_ptr += IO_APCCARD_TUPLE_DATA_CIS_OFFSET;

   for (i = 0; i < size; i++) {
      *dest_ptr = *cis_ptr;
      dest_ptr++;
      cis_ptr += 1;
   } /* Endfor */

   return size;

} /* Endbody */

