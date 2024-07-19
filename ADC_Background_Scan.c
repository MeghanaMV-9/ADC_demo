/**********************************************************************************************************************
 * \file ADC_Background_Scan.c
 * \copyright Copyright (C) Infineon Technologies AG 2019
 *
 * Use of this file is subject to the terms of use agreed between (i) you or the company in which ordinary course of
 * business you are acting and (ii) Infineon Technologies AG or its licensees. If and as long as no such terms of use
 * are agreed, use of this file is subject to following:
 *
 * Boost Software License - Version 1.0 - August 17th, 2003
 *
 * Permission is hereby granted, free of charge, to any person or organization obtaining a copy of the software and
 * accompanying documentation covered by this license (the "Software") to use, reproduce, display, distribute, execute,
 * and transmit the Software, and to prepare derivative works of the Software, and to permit third-parties to whom the
 * Software is furnished to do so, all subject to the following:
 *
 * The copyright notices in the Software and this entire statement, including the above license grant, this restriction
 * and the following disclaimer, must be included in all copies of the Software, in whole or in part, and all
 * derivative works of the Software, unless such copies or derivative works are solely in the form of
 * machine-executable object code generated by a source language processor.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *********************************************************************************************************************/

/*********************************************************************************************************************/
/*-----------------------------------------------------Includes------------------------------------------------------*/
/*********************************************************************************************************************/
#include "ADC_Background_Scan.h"
#include "IfxVadc_Adc.h"

/*********************************************************************************************************************/
/*------------------------------------------------------Macros-------------------------------------------------------*/
/*********************************************************************************************************************/
#define CHANNELS_NUM    4                       /* Number of used channels                                          */

/*********************************************************************************************************************/
/*------------------------------------------------Function Prototypes------------------------------------------------*/
/*********************************************************************************************************************/
void initVADCModule(void);                      /* Function to initialize the VADC module with default parameters   */
void initVADCGroup(void);                       /* Function to initialize the VADC group                            */
void initVADCChannels(void);                    /* Function to initialize the VADC used channels                    */

/*********************************************************************************************************************/
/*-------------------------------------------------Global variables--------------------------------------------------*/
/*********************************************************************************************************************/
IfxVadc_Adc g_vadc;                                           /* Global variable for configuring the VADC module    */
IfxVadc_Adc_Group g_vadcGroup;                                /* Global variable for configuring the VADC group     */
IfxVadc_Adc_Channel g_vadcChannel[CHANNELS_NUM];              /* Global variable for configuring the VADC channels  */
/* Define the used channels */
IfxVadc_ChannelId g_vadcChannelIDs[] = {IfxVadc_ChannelId_4,  /* AN36: channel 4 of group 4                         */
                                        IfxVadc_ChannelId_5,  /* AN37: channel 5 of group 4                         */
                                        IfxVadc_ChannelId_6,  /* AN38: channel 6 of group 4                         */
                                        IfxVadc_ChannelId_7}; /* AN39: channel 7 of group 4                         */

/*********************************************************************************************************************/
/*---------------------------------------------Function Implementations----------------------------------------------*/
/*********************************************************************************************************************/
/* Function to initialize the VADC module */
void initADC(void)
{
    initVADCModule();                                                   /* Initialize the VADC module               */
    initVADCGroup();                                                    /* Initialize the VADC group                */
    initVADCChannels();                                                 /* Initialize the used channels             */

    /* Start the scan */
    IfxVadc_Adc_startBackgroundScan(&g_vadc);
}

/* Function to initialize the VADC module with default parameters */
void initVADCModule(void)
{
    IfxVadc_Adc_Config adcConf;                                         /* Define a configuration structure         */
    IfxVadc_Adc_initModuleConfig(&adcConf, &MODULE_VADC);               /* Fill it with default values              */
    IfxVadc_Adc_initModule(&g_vadc, &adcConf);                          /* Apply the configuration                  */
}

/* Function to initialize the VADC group */
void initVADCGroup(void)
{
    IfxVadc_Adc_GroupConfig adcGroupConf;                               /* Define a configuration structure         */
    IfxVadc_Adc_initGroupConfig(&adcGroupConf, &g_vadc);                /* Fill it with default values              */

    adcGroupConf.groupId = IfxVadc_GroupId_4;                           /* Select the Group 4                       */
    adcGroupConf.master = adcGroupConf.groupId;                         /* Set the same group as master group       */

    /* Enable the background scan source and the background auto scan functionality */
    adcGroupConf.arbiter.requestSlotBackgroundScanEnabled = TRUE;
    adcGroupConf.backgroundScanRequest.autoBackgroundScanEnabled = TRUE;

    /* Enable the gate in "always" mode (no edge detection) */
    adcGroupConf.backgroundScanRequest.triggerConfig.gatingMode = IfxVadc_GatingMode_always;

    IfxVadc_Adc_initGroup(&g_vadcGroup, &adcGroupConf);                 /* Apply the configuration                  */
}

/* Function to initialize the VADC used channels */
void initVADCChannels(void)
{
    IfxVadc_Adc_ChannelConfig adcChannelConf[CHANNELS_NUM];             /* Array of configuration structures        */

    uint16 chn;
    for(chn = 0; chn < CHANNELS_NUM; chn++)                             /* Initialize all the channels in a loop    */
    {
        /* Fill the configuration with default values */
        IfxVadc_Adc_initChannelConfig(&adcChannelConf[chn], &g_vadcGroup);

        /* Set the channel ID and the corresponding result register */
        adcChannelConf[chn].channelId = g_vadcChannelIDs[chn];          /* The channels 4..7 are initialized        */
        adcChannelConf[chn].resultRegister = (IfxVadc_ChannelResult)(chn);
        adcChannelConf[chn].backgroundChannel = TRUE;                   /* Enable background scan for the channel   */

        /* Apply the channel configuration */
        IfxVadc_Adc_initChannel(&g_vadcChannel[chn], &adcChannelConf[chn]);

        /* Add the channel to background scan */
        unsigned chnEnableBit = (1 << adcChannelConf[chn].channelId);   /* Set the the corresponding input channel  */
        unsigned mask = chnEnableBit;                                   /* of the respective group to be added in   */
        IfxVadc_Adc_setBackgroundScan(&g_vadc, &g_vadcGroup, chnEnableBit, mask); /* the background scan sequence.  */
    }
}

/* Function to read the VADC measurement */
uint16 readADCValue(uint8 channel)
{
    Ifx_VADC_RES conversionResult;
    do
    {
        conversionResult = IfxVadc_Adc_getResult(&g_vadcChannel[channel]);
    } while(!conversionResult.B.VF);

    return conversionResult.B.RESULT;
}
