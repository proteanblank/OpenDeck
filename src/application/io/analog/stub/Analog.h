/*

Copyright 2015-2021 Igor Petrovic

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

#pragma once

namespace IO
{
    class Analog
    {
        public:
        enum class adcType_t : uint16_t
        {
            adc10bit = 1023,
            adc12bit = 4095
        };

        typedef struct
        {
            const uint16_t adcMinValue;                 ///< Minimum raw ADC value.
            const uint16_t adcMaxValue;                 ///< Maxmimum raw ADC value.
            const uint16_t stepDiff14Bit;               ///< Minimum difference between two raw ADC readings to consider that value has been changed for 14-bit MIDI values.
            const uint16_t fsrMinValue;                 ///< Minimum raw ADC reading for FSR sensors.
            const uint16_t fsrMaxValue;                 ///< Maximum raw ADC reading for FSR sensors.
            const uint16_t aftertouchMaxValue;          ///< Maxmimum raw ADC reading for aftertouch on FSR sensors.
            const uint16_t digitalValueThresholdOn;     ///< Value above which buton connected to analog input is considered pressed.
            const uint16_t digitalValueThresholdOff;    ///< Value below which button connected to analog input is considered released.
        } adcConfig_t;

        enum class type_t : uint8_t
        {
            potentiometerControlChange,
            potentiometerNote,
            fsr,
            button,
            nrpn7bit,
            nrpn14bit,
            pitchBend,
            controlChange14bit,
            AMOUNT
        };

        enum class pressureType_t : uint8_t
        {
            velocity,
            aftertouch
        };

        class HWA
        {
            public:
            virtual bool value(size_t index, uint16_t& value) = 0;
        };

        class Filter
        {
            public:
            virtual Analog::adcType_t adcType()                                                                              = 0;
            virtual bool              isFiltered(size_t index, Analog::type_t type, uint16_t value, uint16_t& filteredValue) = 0;
            virtual void              reset(size_t index)                                                                    = 0;
        };

        using buttonHandler_t = std::function<void(size_t index, bool state)>;

        Analog(HWA&                     hwa,
               Filter&                  filter,
               Database&                database,
               Util::MessageDispatcher& dispatcher)
            : _hwa(hwa)
            , _filter(filter)
            , _database(database)
            , _dispatcher(dispatcher)
        {}

        void update(bool forceResend = false)
        {
        }

        adcType_t adcType()
        {
#ifdef ADC_12_BIT
            return adcType_t::adc12bit;
#else
            return adcType_t::adc10bit;
#endif
        }

        void debounceReset(uint16_t index)
        {
        }

        void registerButtonHandler(buttonHandler_t buttonHandler)
        {
        }

        adcConfig_t& config()
        {
            return adcConfigStub;
        }

        private:
        adcConfig_t adcConfigStub = {
            .adcMaxValue              = 0,
            .stepDiff14Bit            = 0,
            .fsrMinValue              = 0,
            .fsrMaxValue              = 0,
            .aftertouchMaxValue       = 0,
            .digitalValueThresholdOn  = 0,
            .digitalValueThresholdOff = 0,
        };
    };
}    // namespace IO