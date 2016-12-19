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
#include <stdint.h>

namespace ubirch {

    /*!
     * WS2812B flexio configuration structure
     */
    struct WS2812bFlexioConfig {
        uint8_t shifter;        //!< the FlexIO shifter to use
        uint8_t clockTimer;     //!< the FlexIO timer used for base signal
        uint8_t timerT0;        //!< the FlexIO timer to use for the 0-signal
        uint8_t timerT1;        //!< the FlexIO timer to use for the 1-signal
        uint32_t shifterPin;    //!< the FlexIO shifter pin
        uint32_t clockTimerPin; //!< the FlexIO clock timer pin
        uint32_t dataPin;       //!< the FlexIO data pin for the target signal

        WS2812bFlexioConfig() {
            shifter = 0;
            clockTimer = 0;
            timerT0 = 1;
            timerT1 = 2;
            shifterPin = 2;
            clockTimerPin = 3;
            dataPin = 20;
        };
    };


    class WS2812b {
    public:
        WS2812b(WS2812bFlexioConfig config = WS2812bFlexioConfig());

        virtual ~WS2812b();

        void show(unsigned int *pixels, int size);

    private:
        unsigned int shifterIndex, clockTimerIndex;
    };

}