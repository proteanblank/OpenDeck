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

#include "board/Board.h"
#include "board/Internal.h"
#include "core/src/general/Reset.h"
#include "core/src/general/Timing.h"

#ifndef USB_SUPPORTED
#include "board/common/comm/USBOverSerial/USBOverSerial.h"
#include "usb-link/Commands.h"
#endif

// holds total flash size - inserted in the binary by build process
// address where this variable is stored contains total firmware length
// after the last firmware address, crc of firmware is stored
// this is used by the bootloader to verify the crc of application
uint32_t _flashSize __attribute__((section(".fwMetadata"))) __attribute__((used)) = 0;

namespace core
{
    namespace timing
    {
        namespace detail
        {
            /// Implementation of core variable used to keep track of run time in milliseconds.
            volatile uint32_t rTime_ms;
        }    // namespace detail
    }        // namespace timing
}    // namespace core

namespace Board
{
    void init()
    {
#if defined(FW_APP)
        detail::setup::application();
#elif defined(FW_BOOT)
        detail::setup::bootloader();
#endif
    }

    void reboot()
    {
#ifndef USB_SUPPORTED
        // signal to usb link to reboot as well

        uint8_t data[2] = {
            static_cast<uint8_t>(USBLink::internalCMD_t::rebootBTLDR),
            Board::bootloader::magicBootValue()
        };

        USBOverSerial::USBWritePacket packet(USBOverSerial::packetType_t::internal,
                                             data,
                                             2,
                                             USB_OVER_SERIAL_BUFFER_SIZE);
        USBOverSerial::write(UART_CHANNEL_USB_LINK, packet);

        while (!Board::UART::isTxEmpty(UART_CHANNEL_USB_LINK))
            ;

        // give some time to usb link to properly re-initialize so that everything is in sync
        core::timing::waitMs(50);
#endif

        core::reset::mcuReset();
    }

    namespace detail
    {
        void errorHandler()
        {
            while (true)
            {
            }
        }

        namespace isrHandling
        {
            void mainTimer()
            {
                core::timing::detail::rTime_ms++;
                Board::detail::io::checkIndicators();

#ifdef FW_APP
                Board::detail::io::checkDigitalInputs();
#endif
            }
        }    // namespace isrHandling
    }        // namespace detail
}    // namespace Board
