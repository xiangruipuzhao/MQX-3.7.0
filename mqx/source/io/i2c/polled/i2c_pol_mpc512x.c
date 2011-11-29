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
* $FileName: i2c_pol_mpc512x.c$
* $Version : 3.7.2.0$
* $Date    : Feb-7-2011$
*
* Comments:
*
*   This file contains low level functions for the I2C polled device driver
*   for 53XX Coldfire family.
*
*END************************************************************************/


#include <mqx.h>
#include <bsp.h>
#include <io_prv.h>
#include <fio_prv.h>
#include "i2c.h"
#include "i2c_pol_prv.h"
#include "i2c_mpc512x_prv.h"


extern uint_32 _mpc512x_i2c_polled_init (MPC512X_I2C_INIT_STRUCT_CPTR, pointer _PTR_, char_ptr);
extern uint_32 _mpc512x_i2c_polled_deinit (IO_I2C_POLLED_DEVICE_STRUCT_PTR, MPC5125_I2C_INFO_STRUCT_PTR);
extern uint_32 _mpc512x_i2c_polled_rx (IO_I2C_POLLED_DEVICE_STRUCT_PTR, uchar_ptr, uint_32);
extern uint_32 _mpc512x_i2c_polled_tx (IO_I2C_POLLED_DEVICE_STRUCT_PTR, uchar_ptr, uint_32);
extern uint_32 _mpc512x_i2c_polled_ioctl (MPC5125_I2C_INFO_STRUCT_PTR, uint_32, uint_32_ptr);
static uint_8  _mpc512x_i2c_find_baud_index (uint_32);
static uint_32 _mpc512x_i2c_polled_rx_tx (MPC5125_I2C_MODULE_STRUCT_PTR, MPC5125_I2C_INFO_STRUCT_PTR, uchar_ptr, uint_32);


/* I2C frequency divider values taken from the MCF5282UM.pdf, Figure 24-3. */
/* duplicated items are commented out, if commented out value is wanted, uncoment it,
   comment out duplicated and rebuilt bsp */

static const MPC512X_I2C_BAUDRATE_STRUCT _i2c_baudrate_tbl[] = {
   
   {BSP_I2C_CLOCK/15360,0xDF},
   {BSP_I2C_CLOCK/12288,0xDE},
   {BSP_I2C_CLOCK/10240,0x9D},
   {BSP_I2C_CLOCK/9216, 0x9C},
   {BSP_I2C_CLOCK/8192, 0xBF},
   {BSP_I2C_CLOCK/7680, 0x5F},
   {BSP_I2C_CLOCK/7168, 0xBE},
   {BSP_I2C_CLOCK/6144, 0xBD},
   {BSP_I2C_CLOCK/5120, 0xBC},
   {BSP_I2C_CLOCK/4608, 0x5C},
   {BSP_I2C_CLOCK/4096, 0x7F},
   {BSP_I2C_CLOCK/3840, 0x1F},
   {BSP_I2C_CLOCK/3072, 0x96},
   {BSP_I2C_CLOCK/2560, 0xD5},
   {BSP_I2C_CLOCK/2304, 0x1C},
   {BSP_I2C_CLOCK/2048, 0x3F},
   {BSP_I2C_CLOCK/1920, 0x1B},
   {BSP_I2C_CLOCK/1792, 0x3E},
   {BSP_I2C_CLOCK/1536, 0x1A},
   /*{BSP_I2C_CLOCK/1536, 0x3D},*/
   {BSP_I2C_CLOCK/1280, 0x19},
   /*{BSP_I2C_CLOCK/1280, 0x3C},*/
   {BSP_I2C_CLOCK/1152, 0x18},
   {BSP_I2C_CLOCK/1024, 0x3B},
   {BSP_I2C_CLOCK/960 , 0x17},
   {BSP_I2C_CLOCK/896 , 0x3A},
   {BSP_I2C_CLOCK/768 , 0x16},
   /*{BSP_I2C_CLOCK/768 , 0x39},*/
   {BSP_I2C_CLOCK/640 , 0x15},
   /*{BSP_I2C_CLOCK/640 , 0x38},*/
   {BSP_I2C_CLOCK/576 , 0x14},
   {BSP_I2C_CLOCK/512 , 0x37},
   {BSP_I2C_CLOCK/480 , 0x13},
   {BSP_I2C_CLOCK/448 , 0x36},
   {BSP_I2C_CLOCK/384 , 0x12},
   /*{BSP_I2C_CLOCK/384 , 0x35},*/
   {BSP_I2C_CLOCK/320 , 0x11},
   /*{BSP_I2C_CLOCK/320 , 0x34},*/
   {BSP_I2C_CLOCK/288 , 0x10},
   {BSP_I2C_CLOCK/256 , 0x33},
   {BSP_I2C_CLOCK/240 , 0x0F},
   {BSP_I2C_CLOCK/224 , 0x32},
   /*{BSP_I2C_CLOCK/192 , 0x31},*/
   {BSP_I2C_CLOCK/192 , 0x0E},
   {BSP_I2C_CLOCK/160 , 0x0D},
   /*{BSP_I2C_CLOCK/160 , 0x30},*/
   {BSP_I2C_CLOCK/144 , 0x0C},
   {BSP_I2C_CLOCK/128 , 0x0B},
   /*{BSP_I2C_CLOCK/128 , 0x2F},*/
   {BSP_I2C_CLOCK/112 , 0x2E},
   {BSP_I2C_CLOCK/104 , 0x0A},
   {BSP_I2C_CLOCK/96  , 0x2D},
   {BSP_I2C_CLOCK/88  , 0x09},
   /*{BSP_I2C_CLOCK/80  , 0x2C},*/
   {BSP_I2C_CLOCK/80  , 0x08},
   {BSP_I2C_CLOCK/72  , 0x2B},
   {BSP_I2C_CLOCK/68  , 0x07},
   {BSP_I2C_CLOCK/64  , 0x2A},
   {BSP_I2C_CLOCK/56  , 0x06},
   /*{BSP_I2C_CLOCK/56  , 0x29},*/
   {BSP_I2C_CLOCK/48  , 0x05},
   /*{BSP_I2C_CLOCK/48  , 0x28},*/
   {BSP_I2C_CLOCK/44  , 0x04},
   {BSP_I2C_CLOCK/40  , 0x03},
   /*{BSP_I2C_CLOCK/40  , 0x27},*/
   {BSP_I2C_CLOCK/36  , 0x26},
   {BSP_I2C_CLOCK/34  , 0x02},
   {BSP_I2C_CLOCK/32  , 0x25},
   {BSP_I2C_CLOCK/30  , 0x01},
   {BSP_I2C_CLOCK/28  , 0x00},
   /*{BSP_I2C_CLOCK/28  , 0x24},*/
   {BSP_I2C_CLOCK/26  , 0x23},
   {BSP_I2C_CLOCK/24  , 0x22},
   {BSP_I2C_CLOCK/22  , 0x21},
   {BSP_I2C_CLOCK/20  , 0x20},

};


/*FUNCTION****************************************************************
* 
* Function Name    : _mpc512x_i2c_find_baud_index
* Returned Value   : index into table of baudrate dividers
* Comments         :
*    Find best I2C setting for given baudrate.
*
*END*********************************************************************/

static uint_8 _mpc512x_i2c_find_baud_index 
   (
      /* [IN] Desired baudrate */
      uint_32 input_baud
   )
{ /* Body */
   uint_8 low, high, i;
   
   low = 0; 
   high = sizeof(_i2c_baudrate_tbl)/sizeof(MPC512X_I2C_BAUDRATE_STRUCT) - 1;
   do 
   {
      i = (low + high) >> 1;
      if (input_baud < _i2c_baudrate_tbl[i].BAUD_RATE)
      {
         high = i - 1;
      } else {
         low = i + 1;
      }
   } while ((_i2c_baudrate_tbl[i].BAUD_RATE != input_baud) && (high >= low));
   
   /* in case we did not found exact BAUD rate use nearest slower settings than requested */
   if ((_i2c_baudrate_tbl[i].BAUD_RATE > input_baud) && (i != 0))
   {
      i--;
   }
   
   return i;
} /* Endbody */

/*FUNCTION****************************************************************
* 
* Function Name    : _mpc512x_i2c_polled_install
* Returned Value   : MQX error code
* Comments         :
*    Install an I2C device.
*
*END*********************************************************************/

uint_32 _mpc512x_i2c_polled_install
   (
      /* [IN] A string that identifies the device for fopen */
      char_ptr           identifier,
  
      /* [IN] The I/O init data pointer */
      MPC512X_I2C_INIT_STRUCT_CPTR	init_data_ptr
   )
{ /* Body */

   return _io_i2c_polled_install(identifier,
      (uint_32 (_CODE_PTR_)(pointer, pointer _PTR_, char_ptr))_mpc512x_i2c_polled_init,
      
      (uint_32 (_CODE_PTR_)(pointer, pointer))_mpc512x_i2c_polled_deinit,
      (int_32 (_CODE_PTR_)(pointer, char_ptr, int_32))_mpc512x_i2c_polled_rx,
      (int_32 (_CODE_PTR_)(pointer, char_ptr, int_32))_mpc512x_i2c_polled_tx,
      (int_32 (_CODE_PTR_)(pointer, uint_32, uint_32_ptr))_mpc512x_i2c_polled_ioctl, 
      (pointer)init_data_ptr);

} /* Endbody */


/*FUNCTION****************************************************************
* 
* Function Name    : _mpc512x_i2c_polled_init
* Returned Value   : MQX error code
* Comments         :
*    This function initializes an I2C device.
*
*END*********************************************************************/

uint_32 _mpc512x_i2c_polled_init
   (
      /* [IN] Initialization information for the device being opened */
      MPC512X_I2C_INIT_STRUCT_CPTR   io_init_ptr,

      /* [OUT] Address to store device specific information */
      pointer _PTR_                 io_info_ptr_ptr,
       
      /* [IN] The rest of the name of the device opened */
      char_ptr                      open_name_ptr

   )
{ /* Body */
   MPC5125_I2C_MODULE_STRUCT_PTR    i2c_ptr;
   MPC5125_I2C_INFO_STRUCT_PTR      io_info_ptr;

   i2c_ptr = _bsp_get_i2c_base_address (io_init_ptr->CHANNEL);
   if (NULL == i2c_ptr)
   {
        return I2C_ERROR_CHANNEL_INVALID;
   }
  
   io_info_ptr = (MPC5125_I2C_INFO_STRUCT _PTR_)_mem_alloc_system_zero ((uint_32)sizeof (MPC5125_I2C_INFO_STRUCT));
   if (NULL == io_info_ptr) 
   {
      return MQX_OUT_OF_MEMORY;
   }
   _mem_set_type (io_info_ptr, MEM_TYPE_IO_I2C_INFO_STRUCT);

   /* I2C board specific initialization */
   _bsp_i2c_io_init (io_init_ptr->CHANNEL);

   *io_info_ptr_ptr = io_info_ptr;
   io_info_ptr->INIT                             = *io_init_ptr;
   io_info_ptr->I2C_PTR                          = i2c_ptr;
   io_info_ptr->MODE                             = io_init_ptr->MODE;
   io_info_ptr->STATE                            = I2C_STATE_READY;
   io_info_ptr->ADDRESSEE                        = 0;
   io_info_ptr->ONTHEWAY                         = FALSE;
   io_info_ptr->RX_REQUEST                       = 0;
   io_info_ptr->RX_BUFFER                        = NULL;
   io_info_ptr->RX_IN                            = 0;
   io_info_ptr->RX_OUT                           = 0;
   io_info_ptr->TX_BUFFER                        = NULL;
   io_info_ptr->TX_IN                            = 0;
   io_info_ptr->TX_OUT                           = 0;
   io_info_ptr->STATISTICS.INTERRUPTS            = 0;
   io_info_ptr->STATISTICS.RX_PACKETS            = 0;
   io_info_ptr->STATISTICS.TX_PACKETS            = 0;
   io_info_ptr->STATISTICS.TX_LOST_ARBITRATIONS  = 0;
   io_info_ptr->STATISTICS.TX_ADDRESSED_AS_SLAVE = 0;
   io_info_ptr->STATISTICS.TX_NAKS               = 0;

   /* Disable and clear I2C before initializing it */
   i2c_ptr->MCR = 0x00;
  
   /* Clear out all I2C events */
   i2c_ptr->MSR = 0x00;
   
   _PSP_SYNC();

   /* Set the station address for SLAVE receive operations */      
   i2c_ptr->MADR = (io_init_ptr->STATION_ADDRESS << 1)<<MPC5125_I2C_SHIFT_REG;
   
   /* Set the frequency divider for the nearest found baud rate */
   io_info_ptr->BAUD_INDEX = _mpc512x_i2c_find_baud_index (io_init_ptr->BAUD_RATE);
   i2c_ptr->MFDR = (_i2c_baudrate_tbl[io_info_ptr->BAUD_INDEX].IC) << MPC5125_I2C_SHIFT_REG;
   /* Enable I2C - workaround - enable interrupt request flag on MCF53015 */
   i2c_ptr->MCR |= I2C_MCR_EN_MASK;

   return I2C_OK;
   
} /* Endbody */


/*FUNCTION****************************************************************
* 
* Function Name    : _mpc512x_i2c_polled_deinit
* Returned Value   : MQX error code
* Comments         :
*    This function de-initializes an I2C device.
*
*END*********************************************************************/

uint_32 _mpc512x_i2c_polled_deinit
   (
      /* [IN] the initialization information for the device being opened */
      IO_I2C_POLLED_DEVICE_STRUCT_PTR pol_io_dev_ptr,

      /* [IN] the address of the device specific information */
      MPC5125_I2C_INFO_STRUCT_PTR     io_info_ptr
   )
{ /* Body */
   MPC5125_I2C_MODULE_STRUCT_PTR      i2c_ptr;

   if ((NULL == io_info_ptr) || (NULL == pol_io_dev_ptr)) 
   {
      return I2C_ERROR_INVALID_PARAMETER;
   }
      
   i2c_ptr = io_info_ptr->I2C_PTR;

   if (i2c_ptr->MSR & I2C_MSR_BB_MASK) 
   {
      return I2C_ERROR_DEVICE_BUSY;
   }
   
   /* Disable the I2C */
   i2c_ptr->MCR = 0x00;

   /* Clear the I2C events */
   i2c_ptr->MSR = 0x00; 
 
   /* Free info struct */
   _mem_free (pol_io_dev_ptr->DEV_INFO_PTR);
   pol_io_dev_ptr->DEV_INFO_PTR = NULL;

   return MQX_OK;

} /* Endbody */

 
/*FUNCTION****************************************************************
* 
* Function Name    : _mcf53xx_i2c_polled_ioctl
* Returned Value   : MQX error code.
* Comments         : 
*    This function performs miscellaneous services for
*    the I2C I/O device.  
*
*END*********************************************************************/

uint_32 _mpc512x_i2c_polled_ioctl
   (
      /* [IN] the address of the device specific information */
      MPC5125_I2C_INFO_STRUCT_PTR io_info_ptr,

      /* [IN] The command to perform */
      uint_32                     cmd,

      /* [IN] Parameters for the command */
      uint_32_ptr                 param_ptr
   )
{ /* Body */
   MPC5125_I2C_MODULE_STRUCT_PTR  i2c_ptr;
   uint_32                        result = MQX_OK;
   volatile uint_32                tmp, state;
  
   i2c_ptr = io_info_ptr->I2C_PTR;
   
   switch (cmd) 
   {
      case IO_IOCTL_FLUSH_OUTPUT:
         do 
         {
            tmp = i2c_ptr->MSR;
         } while ((0 == (tmp & I2C_MSR_CF_MASK)) || (io_info_ptr->ONTHEWAY) || ((i2c_ptr->MCR & I2C_MCR_TX_MASK) && (io_info_ptr->TX_IN != io_info_ptr->TX_OUT)));
         if (NULL != param_ptr) 
         {
            *param_ptr = (tmp & I2C_MSR_RXAK_MASK)>>MPC5125_I2C_SHIFT_REG;
         }
         break;
      case IO_IOCTL_I2C_REPEATED_START:
         if (i2c_ptr->MCR & I2C_MCR_STA_MASK)
         {
            if ((I2C_STATE_TRANSMIT == io_info_ptr->STATE) || (I2C_STATE_RECEIVE == io_info_ptr->STATE) || (I2C_STATE_FINISHED == io_info_ptr->STATE))
            {
               io_info_ptr->STATE = I2C_STATE_REPEATED_START;
               io_info_ptr->TX_IN = io_info_ptr->TX_OUT;
               io_info_ptr->RX_OUT = io_info_ptr->RX_IN;
               io_info_ptr->RX_REQUEST = 0;
            }
         }
         break;
      case IO_IOCTL_I2C_STOP:
         i2c_ptr->MCR &= (~ (I2C_MCR_TX_MASK | I2C_MCR_TXAK_MASK));
         if (! (i2c_ptr->MCR & I2C_MCR_STA_MASK))
         {
            tmp = i2c_ptr->MDR;
         }
         i2c_ptr->MSR &= (~ (I2C_MSR_AL_MASK | I2C_MSR_IF_MASK));
         i2c_ptr->MCR &= (~ I2C_MCR_STA_MASK);
         io_info_ptr->TX_IN = io_info_ptr->TX_OUT;
         io_info_ptr->RX_OUT = io_info_ptr->RX_IN;
         io_info_ptr->RX_REQUEST = 0;
         io_info_ptr->STATE = I2C_STATE_READY;
         while (i2c_ptr->MSR & I2C_MSR_BB_MASK)
            { };   /* wait for completion */
         break;
        case IO_IOCTL_I2C_SET_RX_REQUEST:
         if (NULL == param_ptr) 
         {
            result = I2C_ERROR_INVALID_PARAMETER;
         } 
         else 
         {
            io_info_ptr->RX_REQUEST = *param_ptr;
         }
           break;
      case IO_IOCTL_I2C_DISABLE_DEVICE:
         i2c_ptr->MCR &= (~ I2C_MCR_EN_MASK);
         break;
      case IO_IOCTL_I2C_ENABLE_DEVICE:
         i2c_ptr->MCR |= I2C_MCR_EN_MASK;
         break;
      case IO_IOCTL_I2C_SET_MASTER_MODE:
         if (I2C_STATE_READY != io_info_ptr->STATE) 
         {
            result = I2C_ERROR_DEVICE_BUSY;
         } 
         else 
         {
            io_info_ptr->MODE = I2C_MODE_MASTER;
         }
         break;
      case IO_IOCTL_I2C_SET_SLAVE_MODE:
         if (I2C_STATE_READY != io_info_ptr->STATE) 
         {
            result = I2C_ERROR_DEVICE_BUSY;
         } 
         else 
         {
            io_info_ptr->MODE = I2C_MODE_SLAVE;
         }
         break;
      case IO_IOCTL_I2C_GET_MODE:
         if (NULL == param_ptr) 
         {
            result = I2C_ERROR_INVALID_PARAMETER;
         } 
         else 
         {
            *param_ptr = io_info_ptr->MODE;
         }
         break;
      case IO_IOCTL_I2C_SET_BAUD:
         if (NULL == param_ptr) 
         {
            result = I2C_ERROR_INVALID_PARAMETER;
         } 
         else if (I2C_STATE_READY != io_info_ptr->STATE) 
         {
            result = I2C_ERROR_DEVICE_BUSY;
         } 
         else 
         {
            io_info_ptr->BAUD_INDEX = _mpc512x_i2c_find_baud_index (*param_ptr);
            i2c_ptr->MFDR = (_i2c_baudrate_tbl[io_info_ptr->BAUD_INDEX].IC)<<MPC5125_I2C_SHIFT_REG;
         }
         break;
      case IO_IOCTL_I2C_GET_BAUD:
         if (NULL == param_ptr) 
         {
            result = I2C_ERROR_INVALID_PARAMETER;
         } 
         else 
         {
            *param_ptr = _i2c_baudrate_tbl[io_info_ptr->BAUD_INDEX].BAUD_RATE;
         }
         break;
      case IO_IOCTL_I2C_SET_DESTINATION_ADDRESS:
         if (NULL == param_ptr) 
         {
            result = I2C_ERROR_INVALID_PARAMETER;
         } 
         else 
         {
            io_info_ptr->ADDRESSEE = *param_ptr;
         }
         break;
      case IO_IOCTL_I2C_GET_DESTINATION_ADDRESS:
         if (NULL == param_ptr) 
         {
            result = I2C_ERROR_INVALID_PARAMETER;
         } 
         else 
         {
            *param_ptr = io_info_ptr->ADDRESSEE;
         }
         break;
      case IO_IOCTL_I2C_SET_STATION_ADDRESS:
         if (NULL == param_ptr) 
         {
            result = I2C_ERROR_INVALID_PARAMETER;
         } 
         else 
         {
            i2c_ptr->MADR = ((*param_ptr) << 1)<<MPC5125_I2C_SHIFT_REG;
         }
         break;
      case IO_IOCTL_I2C_GET_STATION_ADDRESS:
         if (NULL == param_ptr) 
         {
            result = I2C_ERROR_INVALID_PARAMETER;
         } 
         else 
         {
            *param_ptr = (i2c_ptr->MADR >> 1)>>MPC5125_I2C_SHIFT_REG;
         }
         break;
      case IO_IOCTL_I2C_GET_STATE:
         if (NULL == param_ptr) 
         {
            result = I2C_ERROR_INVALID_PARAMETER;
         } 
         else 
         {
            *param_ptr = io_info_ptr->STATE;
         }
         break;
      case IO_IOCTL_I2C_GET_STATISTICS:
         if (NULL == param_ptr) 
         {
            result = I2C_ERROR_INVALID_PARAMETER;
         } 
         else 
         {
            *((I2C_STATISTICS_STRUCT_PTR)param_ptr) = io_info_ptr->STATISTICS;
         }
         break;         
      case IO_IOCTL_I2C_CLEAR_STATISTICS:
         io_info_ptr->STATISTICS.INTERRUPTS = 0;
         io_info_ptr->STATISTICS.RX_PACKETS = 0;
         io_info_ptr->STATISTICS.TX_PACKETS = 0;
         io_info_ptr->STATISTICS.TX_LOST_ARBITRATIONS = 0;
         io_info_ptr->STATISTICS.TX_ADDRESSED_AS_SLAVE = 0;
         io_info_ptr->STATISTICS.TX_NAKS = 0;
         break;
         
      case IO_IOCTL_I2C_GET_BUS_AVAILABILITY:
         if (NULL == param_ptr) 
         {
            result = I2C_ERROR_INVALID_PARAMETER;
         } 
         else 
         {
            if (i2c_ptr->MSR & I2C_MSR_BB_MASK) 
            {
                *param_ptr = I2C_BUS_BUSY;
            } 
            else 
            {
                *param_ptr = I2C_BUS_IDLE;
            }
         }
         break;
      default:
         break;
   }
   return result;
} /* Endbody */


/*FUNCTION****************************************************************
* 
* Function Name    : _mpc512x_i2c_polled_rx
* Returned Value   : number of bytes read
* Comments         : 
*   Returns the number of bytes received.
*   Reads the data into provided array when data is available.
*
*END*********************************************************************/

uint_32 _mpc512x_i2c_polled_rx
   (
      /* [IN] the address of the device specific information */
      IO_I2C_POLLED_DEVICE_STRUCT_PTR pol_io_dev_ptr,

      /* [IN] The array to copy data into */
      uchar_ptr                       buffer,
      
      /* [IN] number of bytes to read */
      uint_32                         length
   )
{ /* Body */
   MPC5125_I2C_INFO_STRUCT_PTR        io_info_ptr;
   MPC5125_I2C_MODULE_STRUCT_PTR      i2c_ptr;   
   uint_8                             tmp;
         
   io_info_ptr = pol_io_dev_ptr->DEV_INFO_PTR;
   i2c_ptr = io_info_ptr->I2C_PTR;

   /* If beginning of transmission, set state and send address (master only) */
   tmp = io_info_ptr->STATE;
   if ((I2C_STATE_READY == tmp) || (I2C_STATE_REPEATED_START == tmp))
   {
      io_info_ptr->STATE = I2C_STATE_RECEIVE;
      if (I2C_MODE_MASTER == io_info_ptr->MODE)
      {
         i2c_ptr->MCR |= I2C_MCR_TX_MASK;
         if (I2C_STATE_REPEATED_START == tmp)
         {
            i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
            i2c_ptr->MCR |= I2C_MCR_RSTA_MASK;
            while (0 == (i2c_ptr->MSR & I2C_MSR_CF_MASK)) 
               { };   /* wait for completion */
         }
         else
         {
            i2c_ptr->MCR |= I2C_MCR_STA_MASK;
         }
         io_info_ptr->ONTHEWAY = TRUE;
         i2c_ptr->MDR = ((io_info_ptr->ADDRESSEE << 1) | I2C_OPERATION_READ)<<MPC5125_I2C_SHIFT_REG;
         io_info_ptr->STATISTICS.TX_PACKETS++;
      }
   }

   return _mpc512x_i2c_polled_rx_tx (i2c_ptr, io_info_ptr, buffer, length);
} /* Endbody */


/*FUNCTION****************************************************************
* 
* Function Name    : _mcf53xx_i2c_polled_tx
* Returned Value   : number of bytes transmitted
* Comments         : 
*   Writes the provided data buffer and loops until transmission complete.
*
*END*********************************************************************/

uint_32 _mpc512x_i2c_polled_tx
   (
      /* [IN] the address of the device specific information */
      IO_I2C_POLLED_DEVICE_STRUCT_PTR pol_io_dev_ptr,

      /* [IN] The array characters are to be read from */
      uchar_ptr                       buffer,
      
      /* [IN] number of bytes to output */
      uint_32                         length
   )
{ /* Body */
   MPC5125_I2C_INFO_STRUCT_PTR        io_info_ptr;
   MPC5125_I2C_MODULE_STRUCT_PTR      i2c_ptr;   
   uint_32                            tmp;
   
   io_info_ptr  = pol_io_dev_ptr->DEV_INFO_PTR;
   i2c_ptr = io_info_ptr->I2C_PTR;

   /* If beginning of transmission, set state and send address (master only) */
   tmp = io_info_ptr->STATE;
   if ((I2C_STATE_READY == tmp) || (I2C_STATE_REPEATED_START == tmp))
   {
      io_info_ptr->STATE = I2C_STATE_TRANSMIT;
      if (I2C_MODE_MASTER == io_info_ptr->MODE)
      {
         i2c_ptr->MCR |= I2C_MCR_TX_MASK;
         if (I2C_STATE_REPEATED_START == tmp)
         {
            i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
            i2c_ptr->MCR |= I2C_MCR_RSTA_MASK;
            while (0 == (i2c_ptr->MSR & I2C_MSR_CF_MASK))
               { };   /* wait for completion */
         }
         else
         {
            i2c_ptr->MCR |= I2C_MCR_STA_MASK;
         }
         io_info_ptr->ONTHEWAY = TRUE;
         i2c_ptr->MDR = ((io_info_ptr->ADDRESSEE << 1) | I2C_OPERATION_WRITE)<<MPC5125_I2C_SHIFT_REG;
         io_info_ptr->STATISTICS.TX_PACKETS++;
      }
   }

   return _mpc512x_i2c_polled_rx_tx (i2c_ptr, io_info_ptr, buffer, length);
} /* Endbody */


/*FUNCTION****************************************************************
* 
* Function Name    : _mcf5xx_i2c_polled_rx_tx
* Returned Value   : number of bytes processed
* Comments         : 
*   Actual data transfer on I2C bus.
*
*END*********************************************************************/

static uint_32 _mpc512x_i2c_polled_rx_tx
   (
      /* [IN] I2C register structure */
      MPC5125_I2C_MODULE_STRUCT_PTR     i2c_ptr,

      /* [IN] I2C state structure */
      MPC5125_I2C_INFO_STRUCT_PTR io_info_ptr,
      
      /* [IN] The buffer for IO operation */
      uchar_ptr                   buffer,
      
      /* [IN] Number of bytes in buffer */
      uint_32                     length
   )
{ /* Body */
   uint_32                        i;
   uint_32                        i2csr;
   volatile uint_32               tmp;
   
   for (i = 0; i <= length; i++)
   {
      do 
      {
         i2csr = i2c_ptr->MSR;
      } while (0 == (i2csr & I2C_MSR_IF_MASK));
      
     
      io_info_ptr->ONTHEWAY = FALSE;
   
      /* Master */
      if (i2c_ptr->MCR & I2C_MCR_STA_MASK)
      {
         /* Transmit */
         if (i2c_ptr->MCR & I2C_MCR_TX_MASK)
         {
            /* Not ack */
            if (i2csr & I2C_MSR_RXAK_MASK)
            {
               i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
               i2c_ptr->MCR &= (~ I2C_MCR_TX_MASK);
               io_info_ptr->STATISTICS.TX_NAKS++;
               io_info_ptr->STATE = I2C_STATE_FINISHED;
               length = i;
            }
            /* Ack */
            else
            {
               /* Transmit requested */
               if (I2C_STATE_TRANSMIT == io_info_ptr->STATE)
               {
                  /* Anything to transmit? */
                  if (i < length)
                  {
                     i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
                     io_info_ptr->ONTHEWAY = TRUE;
                     i2c_ptr->MDR = (*buffer++)<<MPC5125_I2C_SHIFT_REG;   /* transmit data */
                     io_info_ptr->STATISTICS.TX_PACKETS++;
                  }
               }
               /* Receive requested */
               else if (I2C_STATE_RECEIVE == io_info_ptr->STATE)
               {
                  i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
                  i2c_ptr->MCR &= (~ I2C_MCR_TX_MASK);
                  if (1 == io_info_ptr->RX_REQUEST)
                  {
                     i2c_ptr->MCR |= I2C_MCR_TXAK_MASK;
                  }
                  if (0 == io_info_ptr->RX_REQUEST)
                  {
                     i2c_ptr->MCR &= (~ (I2C_MCR_STA_MASK | I2C_MCR_TX_MASK | I2C_MCR_TXAK_MASK));
                     io_info_ptr->STATE = I2C_STATE_FINISHED;
                     length = i;
                  }
                  else
                  {
                     io_info_ptr->ONTHEWAY = TRUE;
                     tmp = i2c_ptr->MDR;   /* dummy read to clock in 1st byte */
                     i--;
                  }
               }
               else
               {
                  i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
                  length = i;
               }
            }
         }
         /* Receive */
         else
         {
            /* Receive requested */
            if ((I2C_STATE_RECEIVE == io_info_ptr->STATE) && (0 != io_info_ptr->RX_REQUEST))
            {
               if (i < length)
               {
                  i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
                  io_info_ptr->RX_REQUEST--;
                  if (0 == io_info_ptr->RX_REQUEST)
                  {
                     i2c_ptr->MCR &= (~ (I2C_MCR_STA_MASK | I2C_MCR_TX_MASK | I2C_MCR_TXAK_MASK));
                     io_info_ptr->STATE = I2C_STATE_FINISHED;
                     length = i + 1;
                     i = length;
                  }
                  else
                  {
                     io_info_ptr->ONTHEWAY = TRUE;
                  }
                  if (1 == io_info_ptr->RX_REQUEST)
                  {
                     i2c_ptr->MCR |= I2C_MCR_TXAK_MASK;
                  }
                  *buffer++ = i2c_ptr->MDR >> MPC5125_I2C_SHIFT_REG;   /* receive data */
                  io_info_ptr->STATISTICS.RX_PACKETS++;
               }
            }
            else 
            {
               i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
               length = i;
            }
         }
      }
      /* Slave */
      else
      {
         /* Master arbitration lost */
         if (i2csr & I2C_MSR_AL_MASK)
         {
            i2c_ptr->MSR &= (~ I2C_MSR_AL_MASK);
            length = i;
            io_info_ptr->STATISTICS.TX_LOST_ARBITRATIONS++;
            io_info_ptr->STATE = I2C_STATE_LOST_ARBITRATION;
         }
         /* Adressed as slave */
         if (i2csr & I2C_MSR_AAS_MASK)
         {
            if (I2C_MODE_MASTER == io_info_ptr->MODE)
            {
               length = i;
               io_info_ptr->STATISTICS.TX_ADDRESSED_AS_SLAVE++;
            }
            /* Transmit requested */
            if (i2csr & I2C_MSR_SRW_MASK)
            {  
               if ((I2C_STATE_TRANSMIT != io_info_ptr->STATE) && (I2C_STATE_ADDRESSED_AS_SLAVE_TX != io_info_ptr->STATE))
               {
                  length = i;
               }
               i2c_ptr->MCR |= I2C_MCR_TX_MASK;
               io_info_ptr->STATE = I2C_STATE_ADDRESSED_AS_SLAVE_TX;
               if (i < length)
               {
                  i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
                  io_info_ptr->ONTHEWAY = TRUE;
                  i2c_ptr->MDR = (*buffer++)<<MPC5125_I2C_SHIFT_REG;   /* transmit data */
                  io_info_ptr->STATISTICS.TX_PACKETS++;
               }
            }
            /* Receive requested */
            else
            {
               i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
               i2c_ptr->MCR &= (~ I2C_MCR_TX_MASK);
               if ((I2C_STATE_RECEIVE != io_info_ptr->STATE) && (I2C_STATE_ADDRESSED_AS_SLAVE_RX != io_info_ptr->STATE))
               {
                  length = i;
               }
               else
               {
                  if (1 == io_info_ptr->RX_REQUEST)
                  {
                     i2c_ptr->MCR |= I2C_MCR_TXAK_MASK;
                  }
               }
               io_info_ptr->STATE = I2C_STATE_ADDRESSED_AS_SLAVE_RX;
               io_info_ptr->ONTHEWAY = TRUE;
               tmp = i2c_ptr->MDR;   /* dummy read to release bus */
               i--;
            }
         }
         /* Normal slave operation */
         else
         {
            /* No master arbitration lost */
            if (! (i2csr & I2C_MSR_AL_MASK))
            {
               /* Transmit */
               if (i2c_ptr->MCR & I2C_MCR_TX_MASK)
               {
                  /* Not ack */
                  if (i2csr & I2C_MSR_RXAK_MASK)
                  {
                     i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
                     i2c_ptr->MCR &= (~ I2C_MCR_TX_MASK);
                     tmp = i2c_ptr->MDR;   /* dummy read to release bus */
                     io_info_ptr->STATISTICS.TX_NAKS++;
                     io_info_ptr->STATE = I2C_STATE_FINISHED;
                     length = i;
                  }
                  /* Ack */
                  else
                  {
                     /* Transmit requested */
                     if ((I2C_STATE_TRANSMIT == io_info_ptr->STATE) || (I2C_STATE_ADDRESSED_AS_SLAVE_TX == io_info_ptr->STATE))
                     {
                        if (i < length)
                        {
                           i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
                           io_info_ptr->ONTHEWAY = TRUE;
                           i2c_ptr->MDR = (*buffer++)<<MPC5125_I2C_SHIFT_REG;   /* transmit data */
                           io_info_ptr->STATISTICS.TX_PACKETS++;
                        }
                     }
                     else
                     {
                        length = i;
                     }
                  }
               }
               /* Receive */
               else
               {
                  /* Receive requested */
                  if (((I2C_STATE_RECEIVE == io_info_ptr->STATE) || (I2C_STATE_ADDRESSED_AS_SLAVE_RX == io_info_ptr->STATE)) && (0 != io_info_ptr->RX_REQUEST))
                  {
                     if (i < length)
                     {
                        i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
                        io_info_ptr->RX_REQUEST--;
                        if (1 == io_info_ptr->RX_REQUEST)
                        {
                           i2c_ptr->MCR |= I2C_MCR_TXAK_MASK;
                        }
                        io_info_ptr->ONTHEWAY = TRUE;
                        *buffer++ = i2c_ptr->MDR;   /* receive data */
                        io_info_ptr->STATISTICS.RX_PACKETS++;
                     }
                  }
                  else
                  {
                     length = i;
                  }
               }
            }
            else
            {
               i2c_ptr->MSR &= (~ I2C_MSR_IF_MASK);
            }
         }
      }
   }
   return length;
} /* Endbody */

/* EOF */
