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
    class Encoders
    {
        public:
        enum class type_t : uint8_t
        {
            controlChange7Fh01h,
            controlChange3Fh41h,
            programChange,
            controlChange,
            presetChange,
            pitchBend,
            nrpn7bit,
            nrpn14bit,
            controlChange14bit,
            AMOUNT
        };

        enum class position_t : uint8_t
        {
            stopped,
            ccw,
            cw,
        };

        enum class acceleration_t : uint8_t
        {
            disabled,
            slow,
            medium,
            fast,
            AMOUNT
        };

        class HWA
        {
            public:
            virtual bool state(size_t index, uint8_t& numberOfReadings, uint32_t& states) = 0;
        };

        class Filter
        {
            virtual bool isFiltered(size_t                    index,
                                    IO::Encoders::position_t  position,
                                    IO::Encoders::position_t& filteredPosition,
                                    uint32_t                  sampleTakenTime) = 0;

            virtual void     reset(size_t index)            = 0;
            virtual uint32_t lastMovementTime(size_t index) = 0;
        };

        Encoders(HWA&                     hwa,
                 Filter&                  filter,
                 uint32_t                 timeDiffTimeout,
                 Database&                database,
                 Util::MessageDispatcher& dispatcher)
        {}

        void init()
        {
        }

        void update()
        {
        }

        void resetValue(size_t index)
        {
        }

        void setValue(size_t index, uint16_t value)
        {
        }
    };
}    // namespace IO