/*

Copyright 2015-2018 Igor Petrovic

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#include "pins/Map.h"
#include "interface/digital/output/leds/Helpers.h"
#include "board/common/constants/LEDs.h"
#include "board/common/analog/input/Variables.h"
#include "board/common/digital/input/Variables.h"
#include "board/common/digital/output/Variables.h"
#include "core/src/HAL/avr/PinManipulation.h"
#include "core/src/general/BitManipulation.h"

namespace Board
{
    namespace detail
    {
        ///
        /// Acquires data by reading all inputs from two connected shift registers.
        ///
        inline void storeDigitalIn()
        {
            setLow(SR_DIN_CLK_PORT, SR_DIN_CLK_PIN);
            setLow(SR_DIN_LATCH_PORT, SR_DIN_LATCH_PIN);
            _NOP();

            setHigh(SR_DIN_LATCH_PORT, SR_DIN_LATCH_PIN);

            for (int i=0; i<NUMBER_OF_IN_SR; i++)
            {
                for (int j=0; j<8; j++)
                {
                    setLow(SR_DIN_CLK_PORT, SR_DIN_CLK_PIN);
                    _NOP();
                    BIT_WRITE(digitalInBuffer[dIn_head][i], j, !readPin(SR_DIN_DATA_PORT, SR_DIN_DATA_PIN));
                    setHigh(SR_DIN_CLK_PORT, SR_DIN_CLK_PIN);
                }
            }
        }

        ///
        /// \brief Checks if any LED state has been changed and writes changed state to output shift registers.
        ///
        inline void checkLEDs()
        {
            bool updateSR = false;

            for (int i=0; i<MAX_NUMBER_OF_LEDS; i++)
            {
                uint8_t ledStateSingle = LED_ON(ledState[i]);

                if (ledStateSingle != lastLEDstate[i])
                {
                    lastLEDstate[i] = ledStateSingle;
                    updateSR = true;
                }
            }

            if (updateSR)
            {
                setLow(SR_OUT_LATCH_PORT, SR_OUT_LATCH_PIN);

                for (int i=0; i<MAX_NUMBER_OF_LEDS; i++)
                {
                    LED_ON(ledState[i]) ? EXT_LED_ON(SR_OUT_DATA_PORT, SR_OUT_DATA_PIN) : EXT_LED_OFF(SR_OUT_DATA_PORT, SR_OUT_DATA_PIN);
                    pulseHighToLow(SR_OUT_CLK_PORT, SR_OUT_CLK_PIN);
                }

                setHigh(SR_OUT_LATCH_PORT, SR_OUT_LATCH_PIN);
            }
        }

        ///
        /// \brief Configures one of 16 inputs/outputs on 4067 multiplexer.
        ///
        inline void setMuxInput()
        {
            BIT_READ(activeMuxInput, 0) ? setHigh(MUX_S0_PORT, MUX_S0_PIN) : setLow(MUX_S0_PORT, MUX_S0_PIN);
            BIT_READ(activeMuxInput, 1) ? setHigh(MUX_S1_PORT, MUX_S1_PIN) : setLow(MUX_S1_PORT, MUX_S1_PIN);
            BIT_READ(activeMuxInput, 2) ? setHigh(MUX_S2_PORT, MUX_S2_PIN) : setLow(MUX_S2_PORT, MUX_S2_PIN);
            BIT_READ(activeMuxInput, 3) ? setHigh(MUX_S3_PORT, MUX_S3_PIN) : setLow(MUX_S3_PORT, MUX_S3_PIN);
        }
    }
}