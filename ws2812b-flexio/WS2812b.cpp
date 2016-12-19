/*
 * WS2812b LED driver code using the FlexIO component of Kinetis MCUs.
 *
 * @author Matthias L. Jugel
 * @date 2016-12-16
 *
 * @copyright &copy; 2015 ubirch GmbH (https://ubirch.com)
 *
 * ```
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 * ```
 */
 
#include "WS2812b.h"
#include <stdio.h>
#include <fsl_port.h>
#include <fsl_flexio.h>

#define WS2812B_CLOCK_FREQ 800000U
#define WS2812B_RGB_BITS   24

#define FLEXIO_CLOCK_DIVIDER(freq)  ((freq)/WS2812B_CLOCK_FREQ)
#define FLEXIO_SHIFT_BITS_MASK      (0x0000FF00)
#define FLEXIO_SHIFT_BITS(n)        (((n) * 2 - 1) << 8)

// calculate count from nanoseconds
//#define NSEC_TO_COUNT(ns, freq)  (((uint64_t)(ns) * (uint64_t)(freq))/1000000000UL)
#define NSEC_TO_COUNT(ns, freq)  (((uint64_t)(ns) * ((uint64_t)(freq/1000000UL))/1000UL))

namespace ubirch
{
    
WS2812b::WS2812b(WS2812bFlexioConfig config)
{
    // initialize FlexIO
    flexio_config_t flexio_config;
    FLEXIO_GetDefaultConfig(&flexio_config);
    FLEXIO_Init(FLEXIO0, &flexio_config);
    FLEXIO_Reset(FLEXIO0);

    // get frequency of the flexio source clock
    const clock_name_t flexio_src_clk = (clock_name_t) ((SIM->SOPT2 & SIM_SOPT2_FLEXIOSRC_MASK) >> SIM_SOPT2_FLEXIOSRC_SHIFT);
    const uint32_t flexio_src_clk_freq = CLOCK_GetFreq(flexio_src_clk);

    printf("FlexIO freq=%d\r\n", flexio_src_clk_freq);

    // signal 0 and 1 high/low counter values,
    const uint32_t T0H = (uint32_t) NSEC_TO_COUNT(375, flexio_src_clk_freq);
    const uint32_t T1H = (uint32_t) NSEC_TO_COUNT(750, flexio_src_clk_freq);
    const uint32_t T0L = (uint32_t) NSEC_TO_COUNT(800, flexio_src_clk_freq);
    const uint32_t T1L = (uint32_t) NSEC_TO_COUNT(500, flexio_src_clk_freq);

     printf("T0H=%d T1H=%d T0L=%d T1L=%d\r\n", T0H, T1H, T0L, T1L);

    // shifter configuration, take clock timer as input and output in flexio pin
    flexio_shifter_config_t shifter_config;
    shifter_config.inputSource = kFLEXIO_ShifterInputFromPin;
    shifter_config.shifterStop = kFLEXIO_ShifterStopBitDisable;
    shifter_config.shifterStart = kFLEXIO_ShifterStartBitDisabledLoadDataOnShift;
    shifter_config.timerSelect = config.clockTimer;
    shifter_config.timerPolarity = kFLEXIO_ShifterTimerPolarityOnPositive;
    shifter_config.pinConfig = kFLEXIO_PinConfigOutput;
    shifter_config.pinSelect = config.shifterPin;
    shifter_config.pinPolarity = kFLEXIO_PinActiveHigh;
    shifter_config.shifterMode = kFLEXIO_ShifterModeTransmit;
    
    // store a treference to the shifter to access during transmission
    shifterIndex = config.shifter;
    FLEXIO_SetShifterConfig(FLEXIO0, config.shifter, &shifter_config);

    // clock generator, generates the 800kHz clock signal in sync with the shifter, output in flexio timer pin
    flexio_timer_config_t timer_clock_config;
    timer_clock_config.timerCompare = FLEXIO_SHIFT_BITS(WS2812B_RGB_BITS) | ((FLEXIO_CLOCK_DIVIDER(flexio_src_clk_freq) / 2) - 1);
    timer_clock_config.timerOutput = kFLEXIO_TimerOutputZeroNotAffectedByReset;
    timer_clock_config.timerDecrement = kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput;
    timer_clock_config.timerReset = kFLEXIO_TimerResetNever;
    timer_clock_config.timerDisable = kFLEXIO_TimerDisableOnTimerCompare;
    timer_clock_config.timerEnable = kFLEXIO_TimerEnableOnTriggerHigh;
    timer_clock_config.timerStop = kFLEXIO_TimerStopBitDisabled;
    timer_clock_config.timerStart = kFLEXIO_TimerStartBitDisabled;
    timer_clock_config.triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_SHIFTnSTAT(config.shifter);
    timer_clock_config.triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveLow;
    timer_clock_config.triggerSource = kFLEXIO_TimerTriggerSourceInternal;
    timer_clock_config.pinConfig = kFLEXIO_PinConfigOutput;
    timer_clock_config.pinSelect = config.clockTimerPin;
    timer_clock_config.pinPolarity = kFLEXIO_PinActiveHigh;
    timer_clock_config.timerMode = kFLEXIO_TimerModeDual8BitBaudBit;
    
    // store a reference to the TIMCMP index for changing the amount of bits send
    clockTimerIndex = config.clockTimer;
    FLEXIO_SetTimerConfig(FLEXIO0, config.clockTimer, &timer_clock_config);

    // timer to generate the 0 signal (0.35us high, 0.9us low, short pulse), triggered by clock
    flexio_timer_config_t timer_t0_config;
    timer_t0_config.timerCompare = (T0L << 8) | T0H;
    timer_t0_config.timerOutput = kFLEXIO_TimerOutputOneNotAffectedByReset;
    timer_t0_config.timerDecrement = kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput;
    timer_t0_config.timerReset = kFLEXIO_TimerResetNever;
    timer_t0_config.timerDisable = kFLEXIO_TimerDisableOnTimerCompare;
    timer_t0_config.timerEnable = kFLEXIO_TimerEnableOnTriggerRisingEdge;
    timer_t0_config.timerStop = kFLEXIO_TimerStopBitDisabled;
    timer_t0_config.timerStart = kFLEXIO_TimerStartBitDisabled;
    timer_t0_config.triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_PININPUT(config.clockTimerPin);
    timer_t0_config.triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveHigh;
    timer_t0_config.triggerSource = kFLEXIO_TimerTriggerSourceInternal;
    timer_t0_config.pinConfig = kFLEXIO_PinConfigOutput;
    timer_t0_config.pinSelect = config.dataPin;
    timer_t0_config.pinPolarity = kFLEXIO_PinActiveHigh;
    timer_t0_config.timerMode = kFLEXIO_TimerModeDual8BitPWM;

    FLEXIO_SetTimerConfig(FLEXIO0, config.timerT0, &timer_t0_config);

    // timer to generate the 1 signal (0.9us high, 0.35us low, long pulse), triggered by shifter
    flexio_timer_config_t timer_t1_config;
    timer_t1_config.timerCompare = (T1L << 8) | T1H;
    timer_t1_config.timerOutput = kFLEXIO_TimerOutputOneNotAffectedByReset;
    timer_t1_config.timerDecrement = kFLEXIO_TimerDecSrcOnFlexIOClockShiftTimerOutput;
    timer_t1_config.timerReset = kFLEXIO_TimerResetNever;
    timer_t1_config.timerDisable = kFLEXIO_TimerDisableOnTriggerFallingEdge;
    timer_t1_config.timerEnable = kFLEXIO_TimerEnableOnTriggerRisingEdge;
    timer_t1_config.timerStop = kFLEXIO_TimerStopBitDisabled;
    timer_t1_config.timerStart = kFLEXIO_TimerStartBitDisabled;
    timer_t1_config.triggerSelect = FLEXIO_TIMER_TRIGGER_SEL_PININPUT(config.shifterPin);
    timer_t1_config.triggerPolarity = kFLEXIO_TimerTriggerPolarityActiveHigh;
    timer_t1_config.triggerSource = kFLEXIO_TimerTriggerSourceInternal;
    timer_t1_config.pinConfig = kFLEXIO_PinConfigOutput;
    timer_t1_config.pinSelect = config.dataPin;
    timer_t1_config.pinPolarity = kFLEXIO_PinActiveHigh;
    timer_t1_config.timerMode = kFLEXIO_TimerModeDual8BitPWM;

    FLEXIO_SetTimerConfig(FLEXIO0, config.timerT1, &timer_t1_config);

    // ensure flexio is enabled
    FLEXIO_Enable(FLEXIO0, true);
}

WS2812b::~WS2812b() {
    FLEXIO_Enable(FLEXIO0, false);
}

void WS2812b::show(unsigned int *pixels, int size)
{
    // set the shifted bits to the number of bits per LED (24)
    volatile uint32_t *TIMCMP = &(FLEXIO0->TIMCMP[clockTimerIndex]);
    *TIMCMP = (*TIMCMP & ~FLEXIO_SHIFT_BITS_MASK) | FLEXIO_SHIFT_BITS(WS2812B_RGB_BITS);

    while (size--) {
        // wait for the shifter to finish
        while (!(FLEXIO_GetShifterStatusFlags(FLEXIO0) & (1U << shifterIndex))) {}

        // if this is the last junk of data, shift a single bit more than necessary to drive signal low
        if (!size) *TIMCMP = (*TIMCMP & ~FLEXIO_SHIFT_BITS_MASK) | FLEXIO_SHIFT_BITS(WS2812B_RGB_BITS);

        // set shifter operational and put data in the  swapped shift buffer to send
        FLEXIO0->TIMSTAT |= FLEXIO_TIMSTAT_TSF(1U << clockTimerIndex);
        FLEXIO0->SHIFTBUFBIS[0] = (*pixels++) << 8;
    }
}

}