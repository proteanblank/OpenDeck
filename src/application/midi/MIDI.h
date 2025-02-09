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

#include "midi/src/MIDI.h"
#include "util/messaging/Messaging.h"

namespace Protocol
{
    class MIDI : public ::MIDI
    {
        public:
        MIDI(::MIDI::HWA& hwa, Util::MessageDispatcher& dispatcher)
            : ::MIDI(hwa)
        {
            dispatcher.listen(Util::MessageDispatcher::messageSource_t::analog,
                              Util::MessageDispatcher::listenType_t::nonFwd,
                              [this](const Util::MessageDispatcher::message_t& dispatchMessage) {
                                  sendMIDI(dispatchMessage);
                              });

            dispatcher.listen(Util::MessageDispatcher::messageSource_t::buttons,
                              Util::MessageDispatcher::listenType_t::nonFwd,
                              [this](const Util::MessageDispatcher::message_t& dispatchMessage) {
                                  sendMIDI(dispatchMessage);
                              });

            dispatcher.listen(Util::MessageDispatcher::messageSource_t::encoders,
                              Util::MessageDispatcher::listenType_t::nonFwd,
                              [this](const Util::MessageDispatcher::message_t& dispatchMessage) {
                                  sendMIDI(dispatchMessage);
                              });

            dispatcher.listen(Util::MessageDispatcher::messageSource_t::touchscreenButton,
                              Util::MessageDispatcher::listenType_t::nonFwd,
                              [this](const Util::MessageDispatcher::message_t& dispatchMessage) {
                                  sendMIDI(dispatchMessage);
                              });

            dispatcher.listen(Util::MessageDispatcher::messageSource_t::touchscreenAnalog,
                              Util::MessageDispatcher::listenType_t::nonFwd,
                              [this](const Util::MessageDispatcher::message_t& dispatchMessage) {
                                  sendMIDI(dispatchMessage);
                              });
        }

        private:
        void sendMIDI(const Util::MessageDispatcher::message_t& message);
    };
}    // namespace Protocol