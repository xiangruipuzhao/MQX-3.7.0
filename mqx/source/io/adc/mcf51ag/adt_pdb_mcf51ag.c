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
* $FileName: adt_pdb_mcf51ag.c$
* $Version : 3.7.3.0$
* $Date    : Feb-22-2011$
*
* Comments:
*
*   This file contains the ADC driver CPU specific functions
*
*END************************************************************************/

#include <mqx.h>
#include <bsp.h>
#include <fio.h>
#include <fio_prv.h>
#include "io.h"
#include "adc.h"
#include "adc_prv.h"
#include "adc_conf.h"

static volatile struct {
    int_8    prescaler;
} pdb_external = {
    -1
};

#if BSPCFG_ENABLE_ADC

/* Base address of PDB peripheral */
static VMCF51AG_PDB_STRUCT_PTR pdb_ptr;

/* static variables for ADT driver */
static volatile struct {
    uint_32  period;
    PDB_INT_FCN idelay_fcn;
    PDB_INT_FCN error_fcn;
    pointer old_idelay_isr;
    pointer old_idelay_isr_data;
    pointer old_error_isr;
    pointer old_error_isr_data;
    _mqx_int old_isr_valid; /* which of interrupts handlers above are valid */
    uint_32 used_channel;  /* mask of used channels */
} pdb_internal = { 0 };

extern ADC_BUNDLE adc_bundle;

static _mqx_int adt_period_prescaler();
static void adt_idelay_isr(pointer param);
static void adt_error_isr(pointer param);
static pointer _bsp_get_psb_base_address();

#define adc_num 0

/*FUNCTION*****************************************************************
* 
* Function Name    : _adt_hw_ioctl
* Returned Value   : _mqx_int
* Comments         :
*    Returns result of ioctl operation.
*
*END*********************************************************************/
_mqx_int _adt_hw_ioctl
(
    /* [IN] the file handle for the device */
    MQX_FILE_PTR f,

    /* [IN] the ioctl command */
    _mqx_uint  cmd,

    /* [IN] the ioctl parameters */
    pointer    param_ptr
)
{
    ADC_DRIVER_BUNDLE_PTR adc_d_bundle = (ADC_DRIVER_BUNDLE_PTR) f->DEV_PTR->DRIVER_INIT_PTR;
    ADC_CHANNEL_BUNDLE_PTR adc_ch_bundle = (ADC_CHANNEL_BUNDLE_PTR) f->DEV_DATA_PTR;
    volatile uint_16 *del_reg;
    uint_32 d, pdb_period;
    
    switch (cmd) 
    {
        case ADC_IOCTL_SET_ERROR_FCN:
            /* sets callback for error interrupt in PDB module */
            pdb_internal.error_fcn = param_ptr;
            break;

        case ADC_IOCTL_SET_BASE_PERIOD:
            /* Correction of PDB period, delays are not checked against shortened period value */
            pdb_period = pdb_internal.period;
            if (pdb_period == 0) { /* period was not set yet? */
                return adt_period_prescaler((uint_32) param_ptr);
            }
            else { /* trim period if possible */
                d = (uint_32) ((uint_64) param_ptr * pdb_ptr->MOD) / pdb_period - 1;
                if (d > MAX_UINT_16)
                    return ADC_ERROR_BAD_PARAM; /* cannot set such period */
                pdb_ptr->MOD = (uint_16) d;
            }
            break;

        case ADC_IOCTL_TRIM_BASE_PERIOD:
            /* Correction of PDB period, delays are not checked against shortened period value */
            if ((uint_32) param_ptr > MAX_UINT_16)
                return ADC_ERROR_BAD_PARAM; /* cannot set such period */
            pdb_ptr->MOD = (uint_16) param_ptr;
            break;

        case ADC_IOCTL_SET_DELAYREG:
            /* Correction of delay registers */
            if (adc_ch_bundle == NULL)
                return ADC_ERROR_ONLY_CHANNEL; /* delays can be set only on channel files */
            if ((uint_32) param_ptr > MAX_UINT_16)
                return ADC_ERROR_PARAM; /* cannot set such delay */
            del_reg = &pdb_ptr->DEL[0];
            del_reg[adc_ch_bundle->ch_num] = (uint_16) param_ptr;
            break;

        case ADC_IOCTL_SET_TRIGGER:
            /* Set HW triggering options */
            pdb_ptr->SCR = (pdb_ptr->PDBC2 & ~MCF51AG_PDB_CTRL2_TRIGSEL_MASK) | (ADC_PDB_TRIGSEL) param_ptr;
            break;

        default:
            return IO_ERROR_INVALID_IOCTL_CMD;
    }

    return ADC_OK;
}

/*FUNCTION*****************************************************************
* 
* Function Name    : _adt_hw_install
* Returned Value   : ADC_OK
* Comments         :
*    Performs CPU specific initialization for ADC driver
*
*END*********************************************************************/
_mqx_int _adt_hw_install(char_ptr adc_name, ADC_DRIVER_BUNDLE_PTR adc_d_bundle, pointer p)
{
    return ADC_OK;
}

/*FUNCTION*****************************************************************
* 
* Function Name    : _adt_hw_init
* Returned Value   : ADC_OK
* Comments         :
*    Performs CPU specific initialization for ADC driver
*
*END*********************************************************************/
_mqx_int _adt_hw_init(MQX_FILE_PTR f, ADT_PTR adt_d)
{ /* Body */
    VMCF51AG_STRUCT_PTR reg_ptr = (VMCF51AG_STRUCT_PTR)BSP_IPSBAR; 
    pdb_ptr = (MCF51AG_PDB_STRUCT_PTR) _bsp_get_pdb_base_address();
    
    /* set PDB to be source for ADC conversion start */
    reg_ptr->SIMX.SOPT2 &=~MCF51XX_SOPT2_ADHWTS_MASK;

    return ADC_OK;
} /* Endbody */

/*FUNCTION*****************************************************************
* 
* Function Name    : _adt_hw_deinit
* Returned Value   : _mqx_int
* Comments         :
*    Performs CPU specific deinitialization for ADC driver
*
*END*********************************************************************/
_mqx_int _adt_hw_deinit(MQX_FILE_PTR, ADT_PTR) {
    return ADC_OK;
}

/*FUNCTION*****************************************************************
* 
* Function Name    : adt_period_prescaler
* Returned Value   : ADC_OK or ADC_ERROR
* Comments         :
*    Finds the best prescaler for given period
*
*END*********************************************************************/
static _mqx_int adt_period_prescaler(uint_32 period)
{
    uint_32  pdb_prescaler, pdb_period1, pdb_period;

    /* PDB was not run till yet..so setup period */
    /* period to be set in microseconds */
    pdb_period = (uint_32) ((uint_64) 8388608000000L / BSP_BUS_CLOCK); /* 128 * 65536 * 1000000 = 8388608000000 */

    if (pdb_external.prescaler == -1) {
        /* PDB prescaler was not set yet */

        pdb_prescaler = 7;
        while ((pdb_prescaler != -1) && (period < pdb_period)) {
            pdb_prescaler--;
            pdb_period1 = pdb_period;
            pdb_period >>= 1;
        }
        pdb_prescaler++;
        if (pdb_prescaler == 8)
            return ADC_ERROR_BAD_PARAM; /* cannot set period */

        pdb_external.prescaler = (int_8) pdb_prescaler;
        pdb_period = pdb_period1; // pdb_period <<= 1; not done this way as we dont want to lose LSB
    }
    else {
        /* compute PDB period */
        pdb_period >>= (7 - pdb_external.prescaler);

        if (period > pdb_period)
            return ADC_ERROR_BAD_PARAM; /* cannot set period */
    }

    pdb_period1 = ((uint_64) period << 16) / pdb_period;
    if (pdb_period1 > 0xFFFF)
        return ADC_ERROR_BAD_PARAM; /* cannot set period */
        
    pdb_ptr->MOD =  pdb_period1 - 1;
    pdb_internal.period = period;

    /* set prescaler */
    pdb_ptr->PDBC2 = (pdb_prescaler << MCF51AG_PDB_CTRL2_PRESCALER_BITNUM) | \
        MCF51AG_PDB_CTRL2_CONT ;

    return ADC_OK;
}

/*FUNCTION*****************************************************************
* 
* Function Name    : _adt_hw_channel_init
* Returned Value   : ADC_OK or ADC_ERROR
* Comments         :
*    Returns result of HW channel initialization
*
*END*********************************************************************/
_mqx_int _adt_hw_channel_init(MQX_FILE_PTR f, ADT_CHANNEL_PTR adt_ch)
{
    ADC_CHANNEL_BUNDLE_PTR adc_ch_bundle = (ADC_CHANNEL_BUNDLE_PTR) f->DEV_DATA_PTR;
    uint_32  pdb_period;
    _mqx_int ch_num;
    _mqx_int status;

    if ((adt_ch->g.trigger & ADC_PDB_TRIGGER) != ADC_PDB_TRIGGER)
        return ADC_ERROR_HWTRIGGER; /* the channel can be run only from HW trigger */

    if (pdb_internal.period != 0) {
        _mqx_int N;
        N = adt_ch->g.period / pdb_internal.period;
        if (N * pdb_internal.period != adt_ch->g.period)
            return ADC_ERROR_BAD_PARAM; /* it's only allowed to have periods as multiple of basic period */
        adc_ch_bundle->periods_preset = N;
        adc_ch_bundle->periods = 0;
    }
    else {
        adc_ch_bundle->periods_preset = 1;
        adc_ch_bundle->periods = 0;

        status = adt_period_prescaler(adt_ch->g.period);
        if (status != ADC_OK)
            return status;
    }
    pdb_ptr->PDBC2 |= MCF51AG_PDB_CTRL2_TRIGSEL_SWTRIG; /* set to software trigger by default */

    ch_num = adc_ch_bundle->ch_num;

    pdb_period = pdb_internal.period; /* <err001>: DON'T REMOVE THIS LINE! CW 6.2 COMPILER ERROR WITH DIVIDING OCCURS OTHERWISE */

    pdb_ptr->PDBC1 |= MCF51AG_PDB_CTRL1_LDOK; /* write new MOD value */
    if (adc_ch_bundle->ch_num == 0) {
        /* channel A is free, configure it */
        pdb_ptr->DEL[ADC_A] = ((uint_64) adt_ch->g.offset * pdb_ptr->MOD) / pdb_period;/* <err001>: we must use pdb_period */
        pdb_ptr->PDBC1 &= ~MCF51AG_PDB_CTRL1_AOS_MASK;
    }
    else {
        /* channel B is free, configure it */
        /* Note: we lose 7 bits of precision. This is the price for that 64 division is not working properly */
        pdb_ptr->DEL[ADC_B] = ((uint_64) adt_ch->g.offset * pdb_ptr->MOD) / pdb_period;/* <err001>: we must use pdb_period */
        pdb_ptr->PDBC1 &= ~MCF51AG_PDB_CTRL1_BOS_MASK;
    }
    pdb_ptr->PDBC1 |= MCF51AG_PDB_CTRL1_LDOK; /* write new MOD value */
    
    pdb_internal.used_channel |= 0x01 << (ADC_MAX_CHANNELS * adc_num + ch_num);

    adt_ch->g.samples = adt_ch->g.samples_preset;
    adt_ch->b = adc_ch_bundle;

    return ADC_OK;
    
}

/*FUNCTION*****************************************************************
* 
* Function Name    : _adt_hw_channel_deinit
* Returned Value   : ADC_OK or ADC_ERROR
* Comments         :
*    Performs CPU specific deinitialization for ADC channel
*
*END*********************************************************************/
_mqx_int _adt_hw_channel_deinit(MQX_FILE_PTR f, ADT_CHANNEL_PTR adt_ch)
{
    ADC_CHANNEL_BUNDLE_PTR adc_ch_bundle = (ADC_CHANNEL_BUNDLE_PTR) f->DEV_DATA_PTR;
    _mqx_int ch_num;

    
    ch_num = adc_ch_bundle->ch_num;

    if (ch_num == 0)
        pdb_ptr->PDBC1 &= ~(MCF51AG_PDB_CTRL1_AOS_MASK | MCF51AG_PDB_CTRL1_ENA);
    else
        pdb_ptr->PDBC1 &= ~(MCF51AG_PDB_CTRL1_BOS_MASK | MCF51AG_PDB_CTRL1_ENB);

    pdb_internal.used_channel &= ~(0x01 << (ADC_MAX_CHANNELS * adc_num + ch_num));

    if (pdb_internal.used_channel == 0) {
        pdb_ptr->SCR &= ~MCF51AG_PDB_SCR_PDBEN;
        pdb_internal.period = 0;
    }

    return ADC_OK;
}

/*FUNCTION*****************************************************************
* 
* Function Name    : _adt_trigger
* Returned Value   : ADC_OK or ADC_ERROR
* Comments         : Starts channel measurement or more channels defined
*                    by trigger mask
*
*END*********************************************************************/
_mqx_int _adt_trigger(MQX_FILE_PTR f, ADT_CHANNEL_GENERIC_PTR channel, ADT_TRIGGER_MASK mask)
{
    ADC_CHANNEL_BUNDLE_PTR adc_ch_bundle = (ADC_CHANNEL_BUNDLE_PTR) f->DEV_DATA_PTR;
    ADC_DRIVER_BUNDLE_PTR adc_d_bundle = (ADC_DRIVER_BUNDLE_PTR) f->DEV_PTR->DRIVER_INIT_PTR;

    if (mask & ~ADC_PDB_TRIGGER)
        return ADC_ERROR_HWTRIGGER; /* cannot trigger SW-triggers */
    
    pdb_ptr->PDBC1 |= MCF51AG_PDB_CTRL1_LDOK;
    if (((adc_ch_bundle == NULL) && ((pdb_internal.used_channel >> (ADC_MAX_CHANNELS * adc_num + 0)) & 0x01)) \
        || (adc_ch_bundle->ch_num == 0)) {
        pdb_ptr->PDBC1 |= MCF51AG_PDB_CTRL1_AOS(2) | MCF51AG_PDB_CTRL1_ENA;
    }
    if (((adc_ch_bundle == NULL) && ((pdb_internal.used_channel >> (ADC_MAX_CHANNELS * adc_num + 1)) & 0x01)) \
        || (adc_ch_bundle->ch_num == 1)) {
        /* Check this, there is only Trigger A out put from PSB module */
    }
    pdb_ptr->PDBC1 |= MCF51AG_PDB_CTRL1_LDOK;
    
    pdb_ptr->SCR |= MCF51AG_PDB_SCR_PDBEN; /* enable PDB module */
    pdb_ptr->PDBC2 |= MCF51AG_PDB_CTRL2_SWTRIG; /* trigger PDB module */

    return ADC_OK;
}

/*FUNCTION*****************************************************************
* 
* Function Name    : _adt_pause
* Returned Value   : ADC_OK
* Comments         : Pauses measurement of single channel, or more channels
*                    defined by trigger mask
*
*END*********************************************************************/
_mqx_int _adt_pause(MQX_FILE_PTR f, ADT_CHANNEL_GENERIC_PTR channel, ADT_TRIGGER_MASK mask)
{
    ADC_CHANNEL_BUNDLE_PTR adc_ch_bundle = (ADC_CHANNEL_BUNDLE_PTR) f->DEV_DATA_PTR;
    ADC_DRIVER_BUNDLE_PTR adc_d_bundle = (ADC_DRIVER_BUNDLE_PTR) f->DEV_PTR->DRIVER_INIT_PTR;

    if (mask & ~ADC_PDB_TRIGGER)
        return ADC_ERROR_HWTRIGGER; /* cannot pause SW-triggers */

    if (((adc_ch_bundle == NULL) && ((pdb_internal.used_channel >> (ADC_MAX_CHANNELS * adc_num + 0)) & 0x01)) \
        || (adc_ch_bundle->ch_num == 0))
        pdb_ptr->PDBC1 &= ~MCF51AG_PDB_CTRL1_AOS_MASK;
    if (((adc_ch_bundle == NULL) && ((pdb_internal.used_channel >> (ADC_MAX_CHANNELS * adc_num + 1)) & 0x01)) \
        || (adc_ch_bundle->ch_num == 1))
        pdb_ptr->PDBC1 &= ~MCF51AG_PDB_CTRL1_BOS_MASK;

    return ADC_OK;
}

/*FUNCTION*****************************************************************
* 
* Function Name    : _adt_resume
* Returned Value   : ADC_OK
* Comments         : Resumes measurement of single paused channel, or more
*                    channels defined by trigger mask
*
*END*********************************************************************/
_mqx_int _adt_resume(MQX_FILE_PTR f, ADT_CHANNEL_GENERIC_PTR channel, ADT_TRIGGER_MASK mask)
{
    ADC_CHANNEL_BUNDLE_PTR adc_ch_bundle = (ADC_CHANNEL_BUNDLE_PTR) f->DEV_DATA_PTR;
    ADC_DRIVER_BUNDLE_PTR adc_d_bundle = (ADC_DRIVER_BUNDLE_PTR) f->DEV_PTR->DRIVER_INIT_PTR;

    if (mask & ~ADC_PDB_TRIGGER)
        return ADC_ERROR_HWTRIGGER; /* cannot resume SW-triggers */

    if (((adc_ch_bundle == NULL) && ((pdb_internal.used_channel >> (ADC_MAX_CHANNELS * adc_num + 0)) & 0x01)) \
        || (adc_ch_bundle->ch_num == 0))
        pdb_ptr->PDBC1 |= MCF51AG_PDB_CTRL1_AOS(2);
    if (((adc_ch_bundle == NULL) && ((pdb_internal.used_channel >> (ADC_MAX_CHANNELS * adc_num + 1)) & 0x01)) \
        || (adc_ch_bundle->ch_num == 1))
        pdb_ptr->PDBC1 |= MCF51AG_PDB_CTRL1_BOS(2);

    return ADC_OK;
}

/*FUNCTION*****************************************************************
* 
* Function Name    : _adt_stop
* Returned Value   : ADC_OK or ADC_ERROR
* Comments         : Stops (resets) measurement of single channel, or more
*                    channels defined by trigger mask
*
*END*********************************************************************/
_mqx_int _adt_stop(MQX_FILE_PTR f, ADT_CHANNEL_GENERIC_PTR channel, ADT_TRIGGER_MASK mask)
{
    ADC_CHANNEL_BUNDLE_PTR adc_ch_bundle = (ADC_CHANNEL_BUNDLE_PTR) f->DEV_DATA_PTR;
    ADC_DRIVER_BUNDLE_PTR adc_d_bundle = (ADC_DRIVER_BUNDLE_PTR) f->DEV_PTR->DRIVER_INIT_PTR;

    if (mask & ~ADC_PDB_TRIGGER)
        return ADC_ERROR_HWTRIGGER; /* cannot stop SW-triggers */

    if (((adc_ch_bundle == NULL) && ((pdb_internal.used_channel >> (ADC_MAX_CHANNELS * adc_num + 0)) & 0x01)) \
        || (adc_ch_bundle->ch_num == 0))
        pdb_ptr->PDBC1 &= ~(MCF51AG_PDB_CTRL1_AOS_MASK | MCF51AG_PDB_CTRL1_ENA);
    if (((adc_ch_bundle == NULL) && ((pdb_internal.used_channel >> (ADC_MAX_CHANNELS * adc_num + 1)) & 0x01)) \
        || (adc_ch_bundle->ch_num == 1))
        pdb_ptr->PDBC1 &= ~(MCF51AG_PDB_CTRL1_BOS_MASK | MCF51AG_PDB_CTRL1_ENB);

    return ADC_OK;
}

/*FUNCTION*****************************************************************
* 
* Function Name    : adt_process_data
* Returned Value   : void
* Comments         : called from adc_process_all_data
*
*END*********************************************************************/
_mqx_int _adt_process_data(ADC_CHANNEL_BUNDLE_PTR adc_ch_bundle)
{ /* Body */
    ADT_CHANNEL_PTR adt_ch = adc_ch_bundle->adt_ch;    
    
    if (adt_ch->g.samples)
        adt_ch->g.samples--;
    else {
        if (adt_ch->g.init_flags & ADC_CHANNEL_MEASURE_ONCE) {
            if (adc_ch_bundle->ch_num == 0)
                pdb_ptr->PDBC1 &= ~MCF51AG_PDB_CTRL1_ENA;
            else
                pdb_ptr->PDBC1 &= ~MCF51AG_PDB_CTRL1_ENB;
        }
#if MQX_USE_LWEVENTS
        if (adt_ch->g.complete_event != NULL)
            _lwevent_set(adt_ch->g.complete_event, adt_ch->g.event_mask); /* set completion event */
#endif
        adt_ch->g.samples = adt_ch->g.samples_preset;
    }
    
    return ADC_OK;
}
#endif // BSPCFG_ENABLE_ADCx

/*FUNCTION****************************************************************
* 
* Function Name    : _bsp_get_pdb_base_address
* Returned Value   : pointer to base of ADC registers
* Comments         :
*    This function returns base address of ADC related register space.
*
*END*********************************************************************/
pointer _bsp_get_pdb_base_address()
{
    return (pointer)(&((VMCF51AG_STRUCT_PTR)BSP_IPSBAR)->PDB);
}

/*FUNCTION*****************************************************************
* 
* Function Name    : pdb_get_prescaler
* Returned Value   : none
* Comments         : Gets PDB raw value or -1 if prescaler was not set yet
*
*END*********************************************************************/
int_16 _pdb_get_prescaler()
{
    if (pdb_external.prescaler == 0)
        return -1;
    else
        return pdb_external.prescaler;
}

/*FUNCTION*****************************************************************
* 
* Function Name    : pdb_set_prescaler
* Returned Value   : none
* Comments         : Sets PDB prescaler value
*
*END*********************************************************************/
_mqx_int _pdb_set_prescaler(int_16 prescaler)
{
    VMCF51AG_PDB_STRUCT_PTR pdb_ptr = _bsp_get_pdb_base_address();
    if ((prescaler < 0) || (prescaler > 7))
        return IO_ERROR;

    /* set prescaler */
    pdb_ptr->PDBC2 &= 0x07 << MCF51AG_PDB_CTRL2_PRESCALER_BITNUM;
    pdb_ptr->PDBC2 |= prescaler << MCF51AG_PDB_CTRL2_PRESCALER_BITNUM;

    pdb_external.prescaler = prescaler;

    return IO_OK;
}

/* EOF */
