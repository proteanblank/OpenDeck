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

#include "board/common/comm/usb/descriptors/types/Helpers.h"

#define CDC_IN_EPADDR           (USB_ENDPOINT_DIR_IN | 1)
#define MIDI_STREAM_IN_EPADDR   (USB_ENDPOINT_DIR_IN | 2)
#define CDC_NOTIFICATION_EPADDR (USB_ENDPOINT_DIR_IN | 3)
#define CDC_OUT_EPADDR          (USB_ENDPOINT_DIR_OUT | 1)
#define MIDI_STREAM_OUT_EPADDR  (USB_ENDPOINT_DIR_OUT | 2)

#define CONTROL_EPSIZE          64
#define CDC_NOTIFICATION_EPSIZE 8
#define CDC_IN_OUT_EPSIZE       32
#define MIDI_IN_OUT_EPSIZE      32