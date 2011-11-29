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
* $FileName: timer_mcf51ac.c$
* $Version : 3.6.2.0$
* $Date    : Jun-9-2010$
*
* Comments:
*
*   This file contains timer functions for use with a MCF5208.
*
*END************************************************************************/

#include "mqx.h"
#include "bsp.h"

/* function and varibales needed for sw prescaller */
boolean _bsp_timer_sw_prescaller_check();
static int_16 _bsp_timer_sw_prescaller;
static int_16 _bsp_timer_sw_prescaller_cnt;

/*FUNCTION*-----------------------------------------------------------------
*
* Function Name   : _bsp_timer_sw_prescaller
* Returned Value  : TRUE if functions was called _bsp_timer_sw_prescaller-times 
* Comments        :
*     Sw prescaller funciton
*
*END*---------------------------------------------------------------------*/

boolean _bsp_timer_sw_prescaller_check() {
    _bsp_timer_sw_prescaller_cnt++;
   if( _bsp_timer_sw_prescaller_cnt >= _bsp_timer_sw_prescaller) {
      _bsp_timer_sw_prescaller_cnt = 0;
      return TRUE;
   } else {
      return FALSE;
   }
}

/*FUNCTION*-----------------------------------------------------------------
*
* Function Name   : _mcf51AC_timer_init_freq
* Returned Value  : the clock rate for the timer (ns per hw tick)
* Comments        :
*   this function will set up a timer to interrupt
*
*END*---------------------------------------------------------------------*/
uint_32 _mcf51AC_timer_init_freq
   (
      /* [IN] the timer to initialize */
      _mqx_uint timer,

      /* [IN] the timer source clock in HZ / tick per second */
      uint_32   clock,

      /* [IN] Unmask the timer after initializing */
      boolean   unmask_timer
   )
{ /* Body */
   VMCF51AC_STRUCT_PTR reg_ptr = _PSP_GET_MBAR();
   VMCF51AC_TPM_STRUCT_PTR  tmp_ptr;
   int_32 rate, prescale, tmpclock = clock;
   
     tmp_ptr = &reg_ptr->TPM3;
     /*  bus clock source for MTIM */
     reg_ptr->SIM.SCGC1 |= MCF51XX_SCGC1_TPM3_MASK;  

   _bsp_timer_sw_prescaller = 0;

   /* compute rate, sw prescaller and precaller */
   do{
      prescale = -1;
      _bsp_timer_sw_prescaller++;
      tmpclock = clock / _bsp_timer_sw_prescaller;
      /* compute the rate & prescaller*/
      do {
         prescale++;
         rate = tmpclock >> prescale;  
      } while ((rate > 0x10000) && (prescale < 8));
   } while((rate > 0xFFFF) || (prescale > 7)); /* rate and prescaller are out of range, increment sw_prescaller */


   /* set registers */   
   tmp_ptr->TPMxMOD = (uint_16) (rate - 1);
   
   tmp_ptr->TPMxSC |= ((uint_8) prescale)|MCF51XX_TPMxSC_CLK_BUSCLK;  
   
   tmp_ptr->TPMxSC |= (unmask_timer ? (MCF51XX_TPMxSC_TOIE_MASK) : 0);
   tmp_ptr->TPMxCNT = (uint_16)0xffff; /* reset the counter */
   
   /* return number of hw-ticks per MQX tick */
   return rate * _bsp_timer_sw_prescaller;
} /* Endbody */


/*FUNCTION*-----------------------------------------------------------------
*
* Function Name   : _mcf51AC_timer_init_int
* Returned Value  : the clock rate for the timer (ns per hw tick)
* Comments        :
*   this function will set up a timer to interrupt
*
*END*---------------------------------------------------------------------*/
uint_32 _mcf51AC_timer_init_int
   (
      /* [IN] the timer to initialize */
      _mqx_uint timer,

      /* [IN] the timer source clock in HZ / tick per second */
      uint_32   clock,
      
      /* [IN] Unmask the timer after initializing */
      boolean   unmask_timer
   )
{ /* Body */

   return _mcf51AC_timer_init_freq(timer, clock, unmask_timer);

} /* Endbody */


/*FUNCTION*-----------------------------------------------------------------
*
* Function Name   : _mcf51AC_get_hwticks
* Returned Value  : Number of ticks
* Comments        :
*   This function will return the number of ticks elapsed since the last
* timer tick.
*
*END*---------------------------------------------------------------------*/

uint_32 _mcf51AC_get_hwticks
   (
      /* [IN] the timer to clear */
      _mqx_uint timer
   )
{ /* Body */

  VMCF51AC_STRUCT_PTR  reg_ptr = _PSP_GET_MBAR();
  VMCF51AC_TPM_STRUCT_PTR  tpm_ptr = &reg_ptr->TPM3;
  uint_32 ticks;   

  /* Start CR 1973 */   
  ticks = (uint_16)tpm_ptr->TPMxCNT;   

  if ((tpm_ptr->TPMxSC & MCF51XX_TPMxSC_TOF_MASK) != 0) {
    // Another full TICK period has expired since we handled
    // the last timer interrupt.  We need to read the counter (PCNTR) again, since the wrap may have
    // occurred between the previous read and the checking of the PIF bit.
    ticks = (uint_16)tpm_ptr->TPMxMOD + (uint_16)tpm_ptr->TPMxCNT;
  } /* Endif */

  /* End CR 1973 */   

  /* actual ticks in MTIMCNT + what we have missed because of sw prescaller */ 
  return ticks +  (tpm_ptr->TPMxMOD * _bsp_timer_sw_prescaller_cnt);

} /* Endbody */


/*FUNCTION*-----------------------------------------------------------------
*
* Function Name   : _mcf51AC_timer_get_nsec
* Returned Value  : nseconds
* Comments        :
*   this function will return the number of nanoseconds elapsed since
* the last timer tick.
*
*END*---------------------------------------------------------------------*/

uint_32 _mcf51AC_timer_get_nsec
   (
      /* [IN] the timer to use */
      _mqx_uint timer,

      /* [IN] the alarm resolution in milliseconds */
      uint_32   resolution
   )
{ /* Body */

   VMCF51AC_STRUCT_PTR  reg_ptr = _PSP_GET_MBAR();
   uint_16 pmr;
   uint_32 ticks = _mcf51AC_get_hwticks(timer);
   /* like to make it nanoseconds here but don't want to overflow uint_32 */
   uint_32 tmsec_resolution = resolution * 10000; /* microseconds * 10 */
   uint_32 tms = ticks * tmsec_resolution;
   
   pmr   = reg_ptr->TPM3.TPMxMOD * _bsp_timer_sw_prescaller;
   return ((100 * tms) / pmr ); /* microseconds * 10 *100 = nanoseconds */

} /* Endbody */

 
/* EOF */