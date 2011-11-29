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
* $FileName: apcflash16.c$
* $Version : 3.7.2.0$
* $Date    : Feb-7-2011$
*
* Comments:
*
*   This file contains the advanced PC Card flash disk driver functions
*
*END************************************************************************/

#include "mqx.h"
#include "bsp.h"
#include "ata16.h"
#include "io_prv.h"
#include "apcflshpr16.h"

#if !BSP_USE_OLD_PCCARD_INTERFACE /* Advanced PCMCIA */
/*FUNCTION*-------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_install
* Returned Value   : uint_32 - a task error code or MQX_OK
* Comments         :
*    Install a PC Card flash driver.
*
*END*----------------------------------------------------------------------*/

uint_32 _io_apcflash_install
   (
      /* [IN] A string that identifies the device for fopen */
      char_ptr                      identifier
   )
{ /* Body */
   IO_PCFLASH_STRUCT_PTR            info_ptr;

   /* Allocate memory for the state structure */
   info_ptr = _mem_alloc_system_zero((uint_32)sizeof(IO_PCFLASH_STRUCT));
#if MQX_CHECK_MEMORY_ALLOCATION_ERRORS
   if (info_ptr == NULL) {
      return(MQX_OUT_OF_MEMORY);
   } /* Endif */
#endif
   _mem_set_type(info_ptr,MEM_TYPE_IO_PCFLASH);

   /* Fill in the state structure with the info we know */
   info_ptr->DRIVE         = 0;    /* always use ATA device 0 */     
   info_ptr->SECTOR_SIZE   = ATA_SECTOR_SIZE;          
   info_ptr->TEMP_BUFF_PTR = NULL;
   info_ptr->ERROR_CODE    = IO_OK;

   _lwsem_create(&info_ptr->LWSEM, 1L);
    
   return (_io_dev_install(identifier,
      (_mqx_int (_CODE_PTR_)(MQX_FILE_PTR, char _PTR_, char _PTR_))_io_apcflash_open,
      (_mqx_int (_CODE_PTR_)(MQX_FILE_PTR))                        _io_apcflash_close,
      (_mqx_int (_CODE_PTR_)(MQX_FILE_PTR, char_ptr, _mqx_int))    _io_apcflash_read,
      (_mqx_int (_CODE_PTR_)(MQX_FILE_PTR, char_ptr, _mqx_int))    _io_apcflash_write,
      (_mqx_int (_CODE_PTR_)(MQX_FILE_PTR, _mqx_uint, pointer))    _io_apcflash_ioctl,
      (pointer)info_ptr)); 

} /* Endbody */


/*FUNCTION*-------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_open
* Returned Value   : An error code or MQX_OK
* Comments         : Opens and initializes a PC Card flash driver.  
*                    A flash card must be inserted for this operation to be 
*                    successful 
*END*----------------------------------------------------------------------*/

_mqx_int _io_apcflash_open
   (
      /* [IN] the file handle for the device being opened */
      FILE_DEVICE_STRUCT_PTR   fd_ptr,
      
      /* [IN] the remaining portion of the name of the device */
      char _PTR_               open_name_ptr,

      /* [IN] the flags to be used during operation */
      char _PTR_               flags
   )
{ /* Body */
   APCCARD_ADDR_INFO_STRUCT         addr_info;
   IO_DEVICE_STRUCT_PTR             io_dev_ptr = fd_ptr->DEV_PTR;
   IO_PCFLASH_STRUCT_PTR            info_ptr
      = (IO_PCFLASH_STRUCT_PTR)io_dev_ptr->DRIVER_INIT_PTR;
   ATA_IDENTIFY_DEVICE_STRUCT_PTR   identify_ptr;
   MQX_FILE_PTR                     pccard_stream = (MQX_FILE_PTR)flags;
   uint_32_ptr                      temp_ptr;
   uint_16_ptr                        temp8_ptr;
   uint_32                          temp;
   uint_32                          error;
   int_32                           io_error;
   uint_32                          num_sectors;
   uint_16                            end_defn[] = {2, 0};

#if MQX_CHECK_ERRORS
   if (pccard_stream == NULL) {
      /* No PCCARD device open */
      return(IO_ERROR);
   } /* Endif */
#endif

   /* 
   ** Allocate a ram buffer the size of one sector to be used when 
   ** reading/writing  partial sectors -- this buffer is also used in this 
   ** function for the identify info 
   */
   temp_ptr = (uint_32_ptr)_mem_alloc_system_zero(info_ptr->SECTOR_SIZE);
#if MQX_CHECK_MEMORY_ALLOCATION_ERRORS
   if (temp_ptr == NULL) {
      return(MQX_OUT_OF_MEMORY);
   } /* Endif */
#endif   
  _mem_set_type(temp_ptr,MEM_TYPE_IO_SECTOR_BUFFER);         

#if MQX_CHECK_ERRORS
   /* Is the lower layer compatible? */
   io_error = _io_ioctl(pccard_stream, IO_IOCTL_DEVICE_IDENTIFY, temp_ptr);
   if ((io_error != MQX_OK) || (temp_ptr[0] != IO_DEV_TYPE_PHYS_ADV_PCCARD)) {
      _mem_free(temp_ptr);
      if (io_error == MQX_OK) {
         io_error = IO_ERROR_DEVICE_INVALID;
      }/* Endif */
      return(io_error);
   } /* Endif */
#endif

   /* Is there a card present? */
   _io_ioctl(pccard_stream, APCCARD_IOCTL_IS_CARD_INSERTED, &temp);
   if ((boolean)temp == FALSE) {
      _mem_free(temp_ptr);
      return(IO_ERROR);
   } /* Endif */

   /* Wait for card to power up. This will apply Vcc and also Vpp if needed */
   _io_ioctl(pccard_stream, APCCARD_IOCTL_WAIT_TILL_READY, NULL);

   /* Make sure card is an ATA flash card */

   /* Read CISTPL_FUNCID tuple */
   temp_ptr[0] = IO_APCCARD_TUPLE_CISTPL_FUNCID;
   _io_ioctl(pccard_stream, APCCARD_IOCTL_READ_TUPLE, temp_ptr);

   temp8_ptr = (uint_16_ptr)(temp_ptr + 1);
   if (temp8_ptr[0] != 0x4) {
      /* This is not a flash disk */
      _mem_free(temp_ptr);
      return(IO_ERROR_DEVICE_INVALID);
   } /* Endif */
      
   info_ptr->TEMP_BUFF_PTR = (uint_16_ptr)temp_ptr;   

   /* Save the PCCard stream */
   info_ptr->PCCARD_STREAM = pccard_stream;


   /* Get memory locations of the flash card */
   /* Register memory */
   addr_info.OFFSET    = ATA_REG_BASE;
   addr_info.TYPE      = APCCARD_COMMON_SPACE;
   addr_info.BANK_SIZE = 32;
   addr_info.PORT_SIZE = 16;
   _io_ioctl(pccard_stream, APCCARD_IOCTL_GET_ADDR_SPACE, (pointer)&addr_info);
   info_ptr->ATA_REG_PTR  = (uint_16_ptr)addr_info.ADDRESS;

   /* Data memory */
   addr_info.OFFSET    = ATA_REG_BASE + ATA_DATA_BUF;
   addr_info.TYPE      = APCCARD_COMMON_SPACE;
   addr_info.BANK_SIZE = 0x400;
   addr_info.PORT_SIZE = 16;
   _io_ioctl(pccard_stream, APCCARD_IOCTL_GET_ADDR_SPACE, (pointer)&addr_info);
   info_ptr->ATA_DATA_PTR  = (uint_16_ptr)addr_info.ADDRESS;

   /* Perform a soft reset on the device */
   error = _io_apcflash_reset(info_ptr);
   if (error != PCFLASH_NO_ERROR) {
      _io_ioctl(info_ptr->PCCARD_STREAM, APCCARD_IOCTL_POWERDOWN_CARD, NULL);
      _io_ioctl(pccard_stream, APCCARD_IOCTL_FREE_ADDR_SPACE, (pointer)info_ptr->ATA_REG_PTR);
      _io_ioctl(pccard_stream, APCCARD_IOCTL_FREE_ADDR_SPACE, (pointer)info_ptr->ATA_DATA_PTR);
      _mem_free(temp_ptr);
      return(IO_ERROR);
   } /* Endif */

   /* Get information about the device */
   identify_ptr = (ATA_IDENTIFY_DEVICE_STRUCT_PTR)((pointer)temp_ptr);
   _io_apcflash_identify_device(info_ptr, (uint_16_ptr)identify_ptr);

   /* Store the info in the state structure */
#if (PSP_ENDIAN == MQX_BIG_ENDIAN)
   _mem_swap_endian((uchar_ptr)end_defn, &(identify_ptr->NUMBER_CYLINDERS));
   _mem_swap_endian((uchar_ptr)end_defn, &(identify_ptr->SECTORS_PER_TRACK));
   _mem_swap_endian((uchar_ptr)end_defn, &(identify_ptr->NUMBER_HEADS));
#endif
   num_sectors =  identify_ptr->NUMBER_CYLINDERS * identify_ptr->SECTORS_PER_TRACK
                * identify_ptr->NUMBER_HEADS ;

   info_ptr->NUM_SECTORS   = num_sectors;  
   info_ptr->SIZE          = num_sectors * info_ptr->SECTOR_SIZE;

   info_ptr->NUMBER_OF_HEADS   = (uint_32)identify_ptr->NUMBER_HEADS;
   info_ptr->NUMBER_OF_TRACKS  = (uint_32)identify_ptr->NUMBER_CYLINDERS;
   info_ptr->SECTORS_PER_TRACK = (uint_32)identify_ptr->SECTORS_PER_TRACK;

   return(MQX_OK);   
} /* Endbody */


/*FUNCTION*-------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_close
* Returned Value   : ERROR CODE
* Comments         : Closes the PC Card flash driver
* 
*END*----------------------------------------------------------------------*/

_mqx_int _io_apcflash_close
   (
      /* [IN] the file handle for the device being closed */
      FILE_DEVICE_STRUCT_PTR fd_ptr
   )
{ /* Body */
   IO_DEVICE_STRUCT_PTR   io_dev_ptr = fd_ptr->DEV_PTR;
   IO_PCFLASH_STRUCT_PTR  info_ptr = (IO_PCFLASH_STRUCT_PTR)io_dev_ptr->DRIVER_INIT_PTR;

   /* power down card */
   _io_ioctl(info_ptr->PCCARD_STREAM, APCCARD_IOCTL_POWERDOWN_CARD, NULL);

   _io_ioctl(info_ptr->PCCARD_STREAM, APCCARD_IOCTL_FREE_ADDR_SPACE, 
      (uint_32_ptr)&info_ptr->ATA_DATA_PTR);
   _io_ioctl(info_ptr->PCCARD_STREAM, APCCARD_IOCTL_FREE_ADDR_SPACE, 
      (pointer)&info_ptr->ATA_REG_PTR);

   _mem_free((pointer)info_ptr->TEMP_BUFF_PTR);
   info_ptr->TEMP_BUFF_PTR = NULL;

   return(MQX_OK);

} /* Endbody */


/*FUNCTION*-------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_read
* Returned Value   : number of bytes read
* Comments         : Reads data from the PC Card flash device
* 
*END*----------------------------------------------------------------------*/

_mqx_int _io_apcflash_read
   (
      /* [IN] the file handle for the device */
      FILE_DEVICE_STRUCT_PTR   fd_ptr,

      /* [IN] where the newly-read bytes are to be stored */
      char_ptr                 data_ptr,

      /* [IN] the number of bytes to read */
      _mqx_int                 num
   )
{ /* Body */
   IO_DEVICE_STRUCT_PTR   io_dev_ptr = fd_ptr->DEV_PTR;
   IO_PCFLASH_STRUCT_PTR  info_ptr = (IO_PCFLASH_STRUCT_PTR)io_dev_ptr->DRIVER_INIT_PTR;
   uint_16_ptr            dst_ptr;
   uint_32                total_size;
   uint_32                sector_size;
    int_32                results;
   uint_32                start_sector;
   uint_32                end_sector;
   uint_32                temp_num1;
   uint_32                offset;
   uint_32                remains;
   uint_32                num_sectors;
   uint_32                sectors_to_read;
   uint_32                i;
   uint_32                temp;

   _lwsem_wait(&info_ptr->LWSEM);
   
   /* Is there a card present? */
   _io_ioctl(info_ptr->PCCARD_STREAM, APCCARD_IOCTL_IS_CARD_INSERTED, &temp);
   if ((boolean)temp == FALSE) {
      fd_ptr->ERROR = IO_ERROR_READ_ACCESS;
      _lwsem_post(&info_ptr->LWSEM);
      return(IO_ERROR);
   } /* Endif */

   /* Start CR 812 */
   if (info_ptr->BLOCK_MODE) {
      results = _io_apcflash_read_write_blocks(fd_ptr, info_ptr, data_ptr, 
         num, FALSE);
      _lwsem_post(&info_ptr->LWSEM);
      return results;
   } /* Endif */
   /* End CR 812*/

   total_size = info_ptr->SIZE;
   sector_size = info_ptr->SECTOR_SIZE;
   if (fd_ptr->LOCATION > total_size) {
      fd_ptr->ERROR = IO_ERROR_READ_ACCESS;
      _lwsem_post(&info_ptr->LWSEM);
      return(IO_ERROR);
   } else {
      if ((num + fd_ptr->LOCATION) > total_size) {
         fd_ptr->ERROR = IO_ERROR_READ_ACCESS;
         num = (int_32)(total_size - fd_ptr->LOCATION + 1);
      } /* Endif */
      start_sector = fd_ptr->LOCATION / info_ptr->SECTOR_SIZE; 
      end_sector   = (fd_ptr->LOCATION + num - 1) / info_ptr->SECTOR_SIZE;
      num_sectors  = end_sector - start_sector + 1;
      temp_num1    = num;
      dst_ptr      = (uint_16_ptr)data_ptr;
      
      /*
      ** We have three conditions:
      **   1. The read is contained in one PCflash sector
      **   2. The read is contained in two adjacent PCflash sectors
      **   3. The read spans many PCflash sectors   
      **
      ** In case 1, it is possible to have a partial PCflash sector. 
      ** In case 2, it is possible that both the end and the start are only 
      **  partial PCflash sectors.
      ** In case 3(really a subset of case 2), the start and end may be partial
      **  but the others will all be complete PCflash sectors.
      */
      
      offset = fd_ptr->LOCATION - info_ptr->SECTOR_SIZE * start_sector;
           
      if (offset > 0) {
         num_sectors--;
         remains = info_ptr->SECTOR_SIZE - offset;
         if (remains > temp_num1) {
            remains = temp_num1;
         }/* Endif */
         results = _io_apcflash_read_partial_sector(fd_ptr, start_sector, offset,
                     remains, dst_ptr);
         if (results == IO_ERROR)
         {
            fd_ptr->ERROR = IO_ERROR_WRITE;
            _lwsem_post(&info_ptr->LWSEM);
            return(results);
         } else {
            fd_ptr->LOCATION += results;
            dst_ptr           = (uint_16_ptr)data_ptr + results;
            temp_num1        -= results;
            start_sector++;
         } /* Endif */
      } /* Endif */
      
      if (num_sectors > 1) {
         /*
         ** Read the middle flash sectors (if any )
         */
         
         /* Calculate number of sectors to read but do not include last one */
         sectors_to_read = num_sectors - 1;
         
         /* Read all the sectors, one at a time */
         for (i = 0; i < sectors_to_read; i++) {
            if (_io_apcflash_read_sector(info_ptr, start_sector + i, dst_ptr)
                   == PCFLASH_NO_ERROR) {
               results          += sector_size;
               fd_ptr->LOCATION += sector_size;
               num_sectors--;
               dst_ptr          += sector_size;
               temp_num1        -= sector_size;
            } else {
               fd_ptr->ERROR = IO_ERROR_WRITE;
               _lwsem_post(&info_ptr->LWSEM);
               return(IO_ERROR);
            } /* Endif */
         } /* Endfor */

      } /* Endif */
      
      if (num_sectors) {
         /* Read the last flash sector */
         offset  = 0;
         remains = temp_num1;
         results  = _io_apcflash_read_partial_sector(fd_ptr, end_sector, offset, 
                        remains, dst_ptr);
         if (results == IO_ERROR) {
            fd_ptr->ERROR = IO_ERROR_WRITE;
            _lwsem_post(&info_ptr->LWSEM);
            return(results);
         } else {
            fd_ptr->LOCATION += results;
         } /* Endif */
      } /* Endif */

   } /* Endif */

   _lwsem_post(&info_ptr->LWSEM);
   return(num);
   
} /* Endbody */


/*FUNCTION*-------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_write
* Returned Value   : number of bytes written
* Comments         : Writes data to the PC Card flash device
* 
*END*----------------------------------------------------------------------*/

_mqx_int _io_apcflash_write
   (
      /* [IN] the file handle for the device */
      FILE_DEVICE_STRUCT_PTR fd_ptr,

      /* [IN] where the outgoing data is store */
      char_ptr               data_ptr,

      /* [IN] the number of bytes to output */
      _mqx_int               num
   )
{ /* Body */
   IO_DEVICE_STRUCT_PTR   io_dev_ptr = fd_ptr->DEV_PTR;
   IO_PCFLASH_STRUCT_PTR  info_ptr = (IO_PCFLASH_STRUCT_PTR)io_dev_ptr->DRIVER_INIT_PTR;
   uint_32                total_size;
   uint_32                sector_size;
   uint_16_ptr              src_ptr;
    int_32                results;
   uint_32                temp_num1; 
   uint_32                start_sector;
   uint_32                end_sector;
   uint_32                offset;
   uint_32                remains;
   uint_32                num_sectors;
   uint_32                sectors_to_write;
   uint_32                i;
   uint_32                temp;


   _lwsem_wait(&info_ptr->LWSEM);
   
   /* Is there a card present? */
   _io_ioctl(info_ptr->PCCARD_STREAM, APCCARD_IOCTL_IS_CARD_INSERTED, &temp);
   if ((boolean)temp == FALSE) {
      fd_ptr->ERROR = IO_ERROR_READ_ACCESS;
      _lwsem_post(&info_ptr->LWSEM);
      return(IO_ERROR);
   } /* Endif */

   /* Start CR 812 */
   if (info_ptr->BLOCK_MODE) {
      results = _io_apcflash_read_write_blocks(fd_ptr, info_ptr, data_ptr, 
         num, TRUE);
      _lwsem_post(&info_ptr->LWSEM);
      return results;
   } /* Endif */
   /* End   CR 82 */

   total_size = info_ptr->SIZE;
   sector_size = info_ptr->SECTOR_SIZE;
   if (fd_ptr->LOCATION > total_size) 
   {
      fd_ptr->ERROR = IO_ERROR_WRITE_ACCESS;
      _lwsem_post(&info_ptr->LWSEM);
      return(IO_ERROR);
   } else {
      if ((num + fd_ptr->LOCATION) > total_size) {
          fd_ptr->ERROR = IO_ERROR_WRITE_ACCESS;
          num = (int_32)(total_size - fd_ptr->LOCATION + 1);
      } /* Endif */
      
      start_sector = fd_ptr->LOCATION / info_ptr->SECTOR_SIZE; 
      end_sector   = (fd_ptr->LOCATION + num - 1) / info_ptr->SECTOR_SIZE;
      num_sectors  = end_sector - start_sector + 1;
      temp_num1    = num;
      src_ptr      = (uint_16_ptr)data_ptr;
      
      /*
      ** We have three conditions:
      **   1. The write is contained in one PCflash sector
      **   2. The write is contained in two adjacent PCflash sectors
      **   3. The write spans many PCflash sectors   
      **
      ** In case 1, it is possible to have a partial PCflash sector. 
      ** In case 2, it is possible that both the end and the start are only 
      **  partial PCflash sectors.
      ** In case 3(really a subset of case 2), the start and end maybe partial
      **  but the others will all be complete PCflash sectors.
      */
      
      offset = fd_ptr->LOCATION - info_ptr->SECTOR_SIZE * start_sector;
           
      if (offset > 0) {
         num_sectors--;
         remains = info_ptr->SECTOR_SIZE - offset;
         if (remains > temp_num1) {
            remains = temp_num1;
         }/* Endif */
         results = _io_apcflash_write_partial_sector(fd_ptr, start_sector, offset,
                     remains, src_ptr);
         if (results == IO_ERROR)
         {
            fd_ptr->ERROR = IO_ERROR_WRITE;
            _lwsem_post(&info_ptr->LWSEM);
            return(results);
         } else {
            fd_ptr->LOCATION += results;
            src_ptr           = (uint_16_ptr)data_ptr + results;
            temp_num1        -= results;
            start_sector++;
         } /* Endif */
      } /* Endif */
      
      if (num_sectors > 1) {
         /*
         ** Write the middle flash sectors (if any )
         */
         
         /* Calculate number of sectors to write but do not include last one */
         sectors_to_write = num_sectors - 1;
         
         /* Calculate the number of bytes need to write in this case */
         for (i = 0; i < sectors_to_write; i++) {
            if (_io_apcflash_write_sector(info_ptr, start_sector + i, src_ptr)
                   == PCFLASH_NO_ERROR) {
               results          += sector_size;
               fd_ptr->LOCATION += sector_size;
               num_sectors--;
               src_ptr          += sector_size;
               temp_num1        -= sector_size;
            } else {
               fd_ptr->ERROR = IO_ERROR_WRITE;
               _lwsem_post(&info_ptr->LWSEM);
               return(IO_ERROR);
            } /* Endif */
         } /* Endfor */

      } /* Endif */
      
      if (num_sectors) {
         /* Write the last flash sector */
         offset  = 0;
         remains = temp_num1;
         results  = _io_apcflash_write_partial_sector(fd_ptr, end_sector, offset, 
                     remains, src_ptr);
         if (results == IO_ERROR) {
            fd_ptr->ERROR = IO_ERROR_WRITE;
            _lwsem_post(&info_ptr->LWSEM);
            return(results);
         } else {
            fd_ptr->LOCATION += results;
         } /* Endif */
      } /* Endif */
      
   } /* Endif */
   
   _lwsem_post(&info_ptr->LWSEM);
   return(num);

} /* Endbody */


/*FUNCTION*---------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_ioctl
* Returned Value   : int_32
* Comments         :
*    Returns result of ioctl operation.
*
*END*-------------------------------------------------------------------------*/

_mqx_int _io_apcflash_ioctl
   (
      /* [IN] the file handle for the device */
      FILE_DEVICE_STRUCT_PTR fd_ptr,

      /* [IN] the ioctl command */
      _mqx_uint              cmd,

      /* [IN] the ioctl parameters */
      pointer                input_param_ptr
   )
{ /* Body */
   PCFLASH_DRIVE_INFO_STRUCT_PTR drive_info_ptr;
   IO_DEVICE_STRUCT_PTR          io_dev_ptr = fd_ptr->DEV_PTR;
   IO_PCFLASH_STRUCT_PTR         info_ptr = 
      (IO_PCFLASH_STRUCT_PTR)io_dev_ptr->DRIVER_INIT_PTR;
   int_32                        result = MQX_OK;
   uint_32_ptr                   param_ptr = input_param_ptr;

   switch (cmd) {
      case PCFLASH_IOCTL_IS_CARD_INSERTED:
         _lwsem_wait(&info_ptr->LWSEM);
         /* Is there a card present? (TRUE/FALSE) */
         _io_ioctl(info_ptr->PCCARD_STREAM, APCCARD_IOCTL_IS_CARD_INSERTED, param_ptr);
         _lwsem_post(&info_ptr->LWSEM);
         break;     
      case PCFLASH_IOCTL_GET_NUM_SECTORS:
         *param_ptr = info_ptr->NUM_SECTORS;
         break;
      /* Start CR 812 */
      case IO_IOCTL_GET_BLOCK_SIZE:
      /* End   CR 812 */
      case PCFLASH_IOCTL_GET_SECTOR_SIZE:
         *param_ptr = info_ptr->SECTOR_SIZE;
         break;
      case IO_IOCTL_DEVICE_IDENTIFY:
         param_ptr[0] = IO_DEV_TYPE_PHYS_PCFLASH;
         param_ptr[1] = IO_DEV_TYPE_LOGICAL_MFS;
         param_ptr[2] = IO_DEV_ATTR_ERASE | IO_DEV_ATTR_POLL
                         | IO_DEV_ATTR_READ | IO_DEV_ATTR_REMOVE
                         | IO_DEV_ATTR_SEEK | IO_DEV_ATTR_WRITE;
         /* Start CR 812 */
         if (info_ptr->BLOCK_MODE) {
            param_ptr[2] |= IO_DEV_ATTR_BLOCK_MODE;
         } /* Endif */
         /* End   CR 812 */
         break;
      case PCFLASH_IOCTL_GET_DRIVE_PARAMS:
         drive_info_ptr = (PCFLASH_DRIVE_INFO_STRUCT_PTR)((pointer)param_ptr);
         drive_info_ptr->NUMBER_OF_HEADS   = info_ptr->NUMBER_OF_HEADS; 
         drive_info_ptr->NUMBER_OF_TRACKS  = info_ptr->NUMBER_OF_TRACKS;
         drive_info_ptr->SECTORS_PER_TRACK = info_ptr->SECTORS_PER_TRACK;
         break;
      /* Start CR 812 */
      case PCFLASH_IOCTL_SET_BLOCK_MODE:
         info_ptr->BLOCK_MODE = TRUE;
         break;
      /* End   CR 812 */
      case IO_IOCTL_GET_NUM_SECTORS:
         result = 0;
         break;
      default:
         result = IO_ERROR_INVALID_IOCTL_CMD;
         break;
   } /* Endswitch */
   return(result);

} /* Endbody */


/* Private functions */
/*FUNCTION*-------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_read_sector
* Returned Value   : error code
* Comments         : 
*
* 
*END*----------------------------------------------------------------------*/

uint_32 _io_apcflash_read_sector
   (
      /* [IN] PC Flash state structure */
      IO_PCFLASH_STRUCT_PTR  info_ptr,

      /* [IN] The sector number to read */
      uint_32                sector,

      /* [IN] Location to read data into */
      uint_16_ptr              data_ptr 
   )
{ /* Body */
   volatile uint_16 _PTR_         ata_reg_ptr;
   volatile uint_16 _PTR_       ata_data_ptr;
   volatile uint_16 _PTR_       data16_ptr;
   uint_32                      data16_size;
   uint_32                      i;
   uint_16                      tmp;
   volatile uint_16             temp;

   ata_reg_ptr = info_ptr->ATA_REG_PTR;
   ata_data_ptr = info_ptr->ATA_DATA_PTR;
   
   data16_size = info_ptr->SECTOR_SIZE >> 1;
   data16_ptr = (uint_16_ptr)data_ptr;

   /* Wait for device to become free */
   if (_io_apcflash_bsy_timeout(ata_reg_ptr, ATA_BSY_DEFAULT_TIMEOUT)) {
      return(PCFLASH_READ_ERROR);
   } /* Endif */
   
   if(info_ptr->DRIVE){
      ata_reg_ptr[ATA_CARD_DRIVE_HEAD] = (ata_reg_ptr[ATA_CARD_DRIVE_HEAD] & 0xFFEF) | (uint_16)(info_ptr->DRIVE << 4);
   }
   else if(ata_reg_ptr[ATA_CARD_DRIVE_HEAD] & 0x10 ) {
      ata_reg_ptr[ATA_CARD_DRIVE_HEAD] = (ata_reg_ptr[ATA_CARD_DRIVE_HEAD] & 0xFFEF) | (uint_16)(info_ptr->DRIVE << 4);  
   }
   
   /* Wait for device to become free */
   if (_io_apcflash_bsy_timeout(ata_reg_ptr, ATA_BSY_DEFAULT_TIMEOUT)) {
      return(PCFLASH_READ_ERROR);
   } /* Endif */

   ata_reg_ptr[ATA_SECTOR_COUNT] = ((uint_16)(0x000000FF & sector) << 8) | (uint_16)1 ;

   /*
    * Setup for Logical Block Addressing (LBA)
    * A7 - A0 reside in the sector register
    * A15 - A8 reside in the cylinder low register
    * A23 - A16 reside in the cylinder high register
    * A27 - A24 reside in the drive/head register
    */
   
   ata_reg_ptr[ATA_CYLINDER_LOW]   = ((uint_16)((0x00FF0000 & sector) >> 16) << 8)|(uint_16)((0x0000FF00 & sector) >> 8);
   
   tmp = (uint_16)((0x0F000000 & sector) >> 24);
   
   if (_io_apcflash_bsy_timeout(ata_reg_ptr, ATA_BSY_DEFAULT_TIMEOUT)) {
      return(PCFLASH_READ_ERROR);
   }
   
   ata_reg_ptr[ATA_CARD_DRIVE_HEAD] = (ATA_READ_SECTOR << 8) | 0x80 | ATA_DRIVE_REG_LBA | 0x20 | tmp;
   
   /* Wait for device to become free */
   if (_io_apcflash_bsy_timeout(ata_reg_ptr, ATA_BSY_DEFAULT_TIMEOUT)) {
      return(PCFLASH_READ_ERROR);
   } 
   
   tmp = ata_reg_ptr[ATA_STATUS] >> 8;

   /* If the data ready bit is not set then an error occurred */
   if (tmp & ATA_STAT_DRDY) {
          /* Fill the sector buffer */
         for (i = 0; i < data16_size; i++) {
            temp = *(ata_data_ptr++);
#if PSP_ENDIAN == MQX_LITTLE_ENDIAN
            *data_ptr++   = temp;
#else
            *data_ptr++   = ((temp & 0xFF00) >> 8) | ((temp & 0xFF) << 8);
#endif            
         } /* Endfor */     
     
   } else {
      return(PCFLASH_READ_ERROR);
   } /* Endif */

   return(PCFLASH_NO_ERROR);
} /* Endbody */


/*FUNCTION*-------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_read_partial_sector
* Returned Value   : uint_32 - number of bytes read if successful,
*                    IO_ERROR otherwise
* Comments         : Read a partial pcflash sector  
*END*----------------------------------------------------------------------*/

int_32 _io_apcflash_read_partial_sector
   (
      FILE_DEVICE_STRUCT_PTR fd_ptr,
      uint_32                sector_number,
      uint_32                offset_in_sector,
      uint_32                remains_in_sector,
      uint_16_ptr              dst_ptr
   )
{ /* Body */
   IO_DEVICE_STRUCT_PTR    io_dev_ptr = fd_ptr->DEV_PTR;
   IO_PCFLASH_STRUCT_PTR   info_ptr   = (IO_PCFLASH_STRUCT_PTR)io_dev_ptr->DRIVER_INIT_PTR;
   uint_16_ptr               tmp_ptr    = info_ptr->TEMP_BUFF_PTR;
   uint_32                 error;
         
   /* Read the sector into temporary buffer */
   error = _io_apcflash_read_sector(info_ptr, sector_number, tmp_ptr);
   if (error != PCFLASH_NO_ERROR) {
      return(IO_ERROR);
   } /* Endif */ 
   
   /* Copy only the part of the sector we want */
   _mem_copy(tmp_ptr + offset_in_sector, dst_ptr, remains_in_sector);

   return(remains_in_sector);   
} /* Endbody */


/*FUNCTION*-------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_write_sector
* Returned Value   : error code
* Comments         : 
*
* 
*END*----------------------------------------------------------------------*/

uint_32 _io_apcflash_write_sector
   (
      /* [IN] PC Flash state structure */
      IO_PCFLASH_STRUCT_PTR  info_ptr,

      /* [IN] The sector number to write */
      uint_32                sector,

      /* [IN] Source data location */
      uint_16_ptr              data_ptr 
   )
{ /* Body */
   volatile uint_16 _PTR_       ata_reg_ptr;
   volatile uint_16 _PTR_       ata_data_ptr;
   volatile uint_16 _PTR_       data16_ptr;
   uint_32                      data16_size;
   uint_32                      i;
   uint_16                      tmp;
   volatile uint_16             temp;
   
   if (sector > info_ptr->NUM_SECTORS) {
      return(PCFLASH_INVALID_SECTOR);
   } /* Endif */

   ata_reg_ptr = info_ptr->ATA_REG_PTR;
   ata_data_ptr = info_ptr->ATA_DATA_PTR;
   
   data16_size = info_ptr->SECTOR_SIZE >> 1;
   data16_ptr = (uint_16_ptr)data_ptr;

   /* Make sure device isn't busy */
   if (_io_apcflash_bsy_timeout(ata_reg_ptr, ATA_BSY_DEFAULT_TIMEOUT) ) {
      return(PCFLASH_WRITE_ERROR);
   } /* Endif */
   
   if(info_ptr->DRIVE){
      ata_reg_ptr[ATA_CARD_DRIVE_HEAD] = (ata_reg_ptr[ATA_CARD_DRIVE_HEAD] & 0xFFEF) | (uint_16)(info_ptr->DRIVE << 4);
   }
   else if(ata_reg_ptr[ATA_CARD_DRIVE_HEAD] & 0x10 ) {
      ata_reg_ptr[ATA_CARD_DRIVE_HEAD] = (ata_reg_ptr[ATA_CARD_DRIVE_HEAD] & 0xFFEF) | (uint_16)(info_ptr->DRIVE << 4);  
   }
   
   if (_io_apcflash_bsy_timeout(ata_reg_ptr, ATA_BSY_DEFAULT_TIMEOUT)) {
      return(PCFLASH_WRITE_ERROR);
   } /* Endif */

   ata_reg_ptr[ATA_SECTOR_COUNT] = ((uint_16)1) | ((uint_16)(0x000000FF & sector) << 8);
   
   /*
    * Setup for Logical Block Addressing (LBA)
    * A7 - A0 reside in the sector register
    * A15 - A8 reside in the cylinder low register
    * A23 - A16 reside in the cylinder high register
    * A27 - A24 reside in the drive/head register
    */
   ata_reg_ptr[ATA_CYLINDER_LOW]  = ((uint_16)((0x00FF0000 & sector) >> 16) << 8) | (uint_16)((0x0000FF00 & sector) >> 8);
   
   if (_io_apcflash_bsy_timeout(ata_reg_ptr, ATA_BSY_DEFAULT_TIMEOUT)) {
      return(PCFLASH_READ_ERROR);
   }
   tmp  = (uint_16)((0x0F000000 & sector) >> 24); 
  
   ata_reg_ptr[ATA_CARD_DRIVE_HEAD] = (ATA_WRITE_SECTOR << 8) | 0x80 | ATA_DRIVE_REG_LBA | 0x20 | tmp;
   
   /* Wait for device to become ready */
   if (_io_apcflash_bsy_timeout(ata_reg_ptr, ATA_BSY_DEFAULT_TIMEOUT)) {
      return(PCFLASH_WRITE_ERROR);
   } /* Endif */

   tmp = ata_reg_ptr[ATA_STATUS] >> 8;

   /* If the data ready bit is not set then an error occurred */
   if (tmp & ATA_STAT_DRDY) {   
       /* Fill the sector buffer */
       for (i = 0; i < data16_size; i++) {
#if PSP_ENDIAN == MQX_LITTLE_ENDIAN
       temp = *data_ptr;
#else
       temp = ((*data_ptr & 0xFF00) >> 8) | ((*data_ptr & 0x00FF) << 8);
#endif
	   *ata_data_ptr++ = temp;
       data_ptr++;
       } /* Endfor */ 

      /* Wait for device to finish writing data */
      if (_io_apcflash_bsy_timeout(ata_reg_ptr, ATA_BSY_DEFAULT_TIMEOUT)) {
         return(PCFLASH_WRITE_ERROR);
      } /* Endif */

      tmp = ata_reg_ptr[ATA_ERROR] >> 8;

      if (tmp & ATA_ERR_MASK) {
         return(PCFLASH_WRITE_ERROR);
      } /* Endif */

   } else {
      return(PCFLASH_WRITE_ERROR);
   } /* Endif */

   return(PCFLASH_NO_ERROR);
} /* Endbody */


/*FUNCTION*-------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_write_partial_sector
* Returned Value   : uint_32 - number of bytes written if successful,
*                    IO_ERROR otherwise
* Comments         : Writes a partial pcflash sector while preserving contents of
*                    unused portions of the sector
* 
*END*----------------------------------------------------------------------*/

int_32 _io_apcflash_write_partial_sector
   (
      FILE_DEVICE_STRUCT_PTR fd_ptr,
      uint_32                sector_number,
      uint_32                offset_in_sector,
      uint_32                remains_in_sector,
      uint_16_ptr              src_ptr
   )
{ /* Body */
   IO_DEVICE_STRUCT_PTR    io_dev_ptr = fd_ptr->DEV_PTR;
   IO_PCFLASH_STRUCT_PTR   info_ptr   = (IO_PCFLASH_STRUCT_PTR)io_dev_ptr->DRIVER_INIT_PTR;
   uint_32                 results;
   uint_16_ptr               tmp_ptr    = info_ptr->TEMP_BUFF_PTR;
   uint_32                 error;
         
   /* Read the sector into temporary buffer */
   error = _io_apcflash_read_sector(info_ptr, sector_number, tmp_ptr);
   if (error != PCFLASH_NO_ERROR) {
      return(IO_ERROR);
   } /* Endif */
   
   /* Write new data into temporary buffer */
   _mem_copy(src_ptr, tmp_ptr + offset_in_sector, remains_in_sector);
   
   /* Write the newly modified ram sector back into pcflash */
   if (_io_apcflash_write_sector(info_ptr, sector_number, tmp_ptr) == PCFLASH_NO_ERROR) {
      results = remains_in_sector;
      return(results);
   } else {
      return(IO_ERROR);
   } /* Endif */
} /* Endbody */


/*FUNCTION*-------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_identify_device
* Returned Value   : error code
* Comments         : 
*
* 
*END*----------------------------------------------------------------------*/

uint_32 _io_apcflash_identify_device
   (
      /* [IN] PC Flash state structure */
      IO_PCFLASH_STRUCT_PTR  info_ptr,

      /* [IN] Location to read data into */
      uint_16_ptr              data_ptr 
   )
{ /* Body */
   volatile uint_16 _PTR_       ata_reg_ptr;
   volatile uint_16 _PTR_       ata_data_ptr;
   volatile uint_16 _PTR_       data16_ptr;
   uint_32                      data16_size;
   uint_32                      i;
   uint_16                        tmp;

   ata_reg_ptr = info_ptr->ATA_REG_PTR;
   ata_data_ptr = info_ptr->ATA_DATA_PTR;
   
   data16_size = info_ptr->SECTOR_SIZE >> 1;
   data16_ptr = (uint_16_ptr)data_ptr;

   /* Make sure device isn't busy */
   if (_io_apcflash_bsy_timeout(ata_reg_ptr, ATA_BSY_DEFAULT_TIMEOUT)) {
      return(PCFLASH_READ_ERROR);
   } /* Endif */
  
   ata_reg_ptr[ATA_COMMAND] = (info_ptr->DRIVE << 4) | (ATA_IDENTIFY_DEVICE << 8);   

   /* Wait for device to become free */
   if (_io_apcflash_bsy_timeout(ata_reg_ptr, ATA_BSY_DEFAULT_TIMEOUT)) {
      return(PCFLASH_READ_ERROR);
   } /* Endif */

   tmp = ata_reg_ptr[ATA_STATUS] >> 8;

   /* If the data ready bit is not set then an error occurred */
   if (!(tmp & ATA_STAT_BSY)) {

      /* Fill the sector buffer */
      for ( i = 0; i < data16_size; i++)  {
        *data16_ptr++ = *ata_data_ptr++;
         
      } /* Endfor */
   } else {
      return(PCFLASH_READ_ERROR);
   } /* Endif */

   return(PCFLASH_NO_ERROR);
} /* Endbody */


/*FUNCTION*-------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_reset
* Returned Value   : error code
* Comments         : 
* 
*END*----------------------------------------------------------------------*/

uint_32 _io_apcflash_reset
   (
      /* [IN] PC Flash state structure */
      IO_PCFLASH_STRUCT_PTR  info_ptr
   )
{ /* Body */
   volatile uint_16 _PTR_        ata_reg_ptr = info_ptr->ATA_REG_PTR;
   uint_32                       temp;
   MQX_TICK_STRUCT               start, current;

   /* Perform soft reset */
   ata_reg_ptr[ATA_DEV_CNTRL] = (ata_reg_ptr[ATA_DEV_CNTRL] & 0xFF00) | ATA_DEV_CNTL_SRST;
   
   /* wait 5 ms */ 
   _time_get_ticks(&start);
   do {
      _time_get_ticks(&current);
   } while (_time_diff_milliseconds(&start,&current, &temp) < 5);
   
   /* Bring device out of reset */
   ata_reg_ptr[ATA_DEV_CNTRL] = 0;
   
   /* Wait for device to come out of reset */
   if (_io_apcflash_bsy_timeout(ata_reg_ptr, ATA_RST_TIMEOUT)) {
      return(PCFLASH_HARDWARE_ERROR);
   } /* Endif */

   return(PCFLASH_NO_ERROR);
} /* Endbody */


/*FUNCTION*-------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_bsy_timeout
* Returned Value   : TRUE if timed out, FALSE otherwise
* Comments         : 
* 
*END*----------------------------------------------------------------------*/

boolean _io_apcflash_bsy_timeout
   (
      /* [IN] ATA register pointer */
      volatile uint_16 _PTR_ ata_reg_ptr,
      /* [IN] Number of seconds to timeout */
      uint_32 timeout_val
   )
{ /* Body */
   TIME_STRUCT time;
   uint_32     seconds;
   uchar       status;

   _time_get_elapsed(&time);
   seconds = time.SECONDS;

   do {
      _time_get_elapsed(&time);
      status = (uchar)(ata_reg_ptr[ATA_STATUS] >> 8);

      if ((time.SECONDS - seconds) >= timeout_val) {
         break;
      } /* Endif */

   } while(status & ATA_STAT_BSY);

   /* Get status again just in case... */
   status = ata_reg_ptr[ATA_STATUS] >> 8;

   if (status & ATA_STAT_ERR) {
      return(TRUE);
   } /* Endif */

   if (status & ATA_STAT_BSY) {
      /* We timed out */
      return(TRUE);
   } /* Endif */

   return(FALSE);
} /* Endbody */


/* Start CR 812 */
/*FUNCTION*-------------------------------------------------------------------
* 
* Function Name    : _io_apcflash_read_write_blocks
* Returned Value   : error or number of blocks read/written
* Comments         : This function is used to read/write blocks when driver
*   is in block mode
* 
*END*----------------------------------------------------------------------*/

_mqx_int _io_apcflash_read_write_blocks
   (
      /* [IN] the file handle for the device */
      FILE_DEVICE_STRUCT_PTR   fd_ptr,

      /* [IN] device info */
      IO_PCFLASH_STRUCT_PTR    info_ptr,

      /* [IN] where the newly-read bytes are to be stored */
      char_ptr                 data_ptr,

      /* [IN] the number of bytes to read */
      _mqx_int                 num,

      /* [IN] Read/write mode */
      boolean                  write
     
   )
{ /* Body */
   uint_32    i;
   _mqx_int   temp;
   _mqx_int   error_code;
   uint_32 (_CODE_PTR_ rw_func)(IO_PCFLASH_STRUCT_PTR, uint_32, uint_16_ptr);

   if (write) {
      error_code = IO_ERROR_WRITE_ACCESS;
      rw_func    = _io_apcflash_write_sector;
   } else {
      error_code = IO_ERROR_READ_ACCESS;
      rw_func    = _io_apcflash_read_sector;
   } /* Endif */


   if (fd_ptr->LOCATION >= info_ptr->NUM_SECTORS) {
      fd_ptr->ERROR = error_code;
      _lwsem_post(&info_ptr->LWSEM);
      return(IO_ERROR);
   } else {
      if ((num + fd_ptr->LOCATION) >= info_ptr->NUM_SECTORS) {
         fd_ptr->ERROR = error_code;
         num = (int_32)(info_ptr->NUM_SECTORS - fd_ptr->LOCATION + 1);
      } /* Endif */
      
      /* Read all the sectors, one at a time */
      temp = 0;
      for (i = 0; i < num; i++) {
         if (rw_func(info_ptr, fd_ptr->LOCATION, (uint_16_ptr)data_ptr) == 
            PCFLASH_NO_ERROR) 
         {
            temp++;
            fd_ptr->LOCATION++;
            data_ptr += info_ptr->SECTOR_SIZE >> 1;
         } else {
            fd_ptr->ERROR = error_code;
            _lwsem_post(&info_ptr->LWSEM);
            return(temp);
         } /* Endif */
      } /* Endfor */

   } /* Endif */
      
   return(temp);
   
} /* Endbody */
/* End   CR 812 */
#endif /* Advanced PCMCIA */

/* EOF */