/*

Copyright 2015-2022 Igor Petrovic

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

#include "Touchscreen.h"
#include "core/src/general/Helpers.h"
#include "core/src/general/Timing.h"
#include "util/conversion/Conversion.h"
#include "util/configurable/Configurable.h"

using namespace IO;

std::array<Touchscreen::Model*, static_cast<size_t>(Touchscreen::model_t::AMOUNT)> Touchscreen::_models;

Touchscreen::Touchscreen(HWA&                hwa,
                         Database::Instance& database,
                         CDCPassthrough&     cdcPassthrough)
    : _hwa(hwa)
    , _database(database)
    , _cdcPassthrough(cdcPassthrough)
{
    MIDIDispatcher.listen(Messaging::eventSource_t::leds,
                          Messaging::listenType_t::fwd,
                          [this](const Messaging::event_t& event) {
                              setIconState(event.componentIndex, event.midiValue);
                          });

    MIDIDispatcher.listen(Messaging::eventSource_t::preset,
                          Messaging::listenType_t::all,
                          [this](const Messaging::event_t& event) {
                              if (!init(mode_t::normal))
                              {
                                  deInit(mode_t::normal);
                              }
                          });

    ConfigHandler.registerConfig(
        System::Config::block_t::touchscreen,
        // read
        [this](uint8_t section, size_t index, uint16_t& value) {
            return sysConfigGet(static_cast<System::Config::Section::touchscreen_t>(section), index, value);
        },

        // write
        [this](uint8_t section, size_t index, uint16_t value) {
            return sysConfigSet(static_cast<System::Config::Section::touchscreen_t>(section), index, value);
        });
}

bool Touchscreen::init()
{
    return init(mode_t::normal);
}

bool Touchscreen::init(mode_t mode)
{
    switch (mode)
    {
    case mode_t::normal:
    {
        if (_database.read(Database::Config::Section::touchscreen_t::setting, Touchscreen::setting_t::enable))
        {
            auto dbModel = static_cast<model_t>(_database.read(Database::Config::Section::touchscreen_t::setting, Touchscreen::setting_t::model));

            if (_initialized)
            {
                if (dbModel == _activeModel)
                {
                    // nothing to do, same model already _initialized
                    return true;
                }

                if (!deInit(mode))
                {
                    return false;
                }
            }

            if (_cdcPassthrough.deInit())
            {
                _activeModel  = dbModel;
                auto instance = modelInstance(_activeModel);

                if (instance != nullptr)
                {
                    _initialized = instance->init();
                }

                if (_initialized)
                {
                    // add slight delay before display becomes ready on power on
                    core::timing::waitMs(1000);

                    setScreen(_database.read(Database::Config::Section::touchscreen_t::setting, Touchscreen::setting_t::initialScreen));
                    setBrightness(static_cast<brightness_t>(_database.read(Database::Config::Section::touchscreen_t::setting, Touchscreen::setting_t::brightness)));

                    _mode = mode;

                    return true;
                }

                return false;
            }

            return false;
        }
    }
    break;

    case mode_t::cdcPassthrough:
    {
        if (_cdcPassthrough.init())
        {
            _mode = mode;
            return true;
        }
    }
    break;

    default:
        return false;
    }

    return false;
}

bool Touchscreen::deInit(mode_t mode)
{
    switch (mode)
    {
    case mode_t::normal:
    {
        if (!_initialized)
        {
            return true;    // nothing to do
        }

        if (_cdcPassthrough.deInit())
        {
            if (modelInstance(_activeModel)->deInit())
            {
                _mode        = mode_t::normal;
                _initialized = false;

                return true;
            }
        }
    }
    break;

    case mode_t::cdcPassthrough:
    {
        if (_cdcPassthrough.deInit())
        {
            _mode = mode_t::normal;
            return true;
        }
    }
    break;

    default:
        return false;
    }

    return false;
}

void Touchscreen::updateSingle(size_t index, bool forceRefresh)
{
    // ignore index here - not applicable
    updateAll(forceRefresh);
}

void Touchscreen::updateAll(bool forceRefresh)
{
    if (isInitialized(mode_t::normal))
    {
        tsData_t  tsData;
        tsEvent_t event = modelInstance(_activeModel)->update(tsData);

        switch (event)
        {
        case tsEvent_t::button:
        {
            processButton(tsData.buttonID, tsData.buttonState);
        }
        break;

        default:
            break;
        }
    }
    else if (isInitialized(mode_t::cdcPassthrough))
    {
        auto touchscreenToUSB = [&]() {
            uint32_t size = 0;

            for (size = 0; size < TSCREEN_CDC_PASSTHROUGH_BUFFER_SIZE; size++)
            {
                uint8_t value;

                if (_cdcPassthrough.uartRead(value))
                {
                    _txBuffer[size] = value;
                }
                else
                {
                    break;
                }
            }

            if (size)
            {
                _cdcPassthrough.cdcWrite(_txBuffer, size);
            }
        };

        auto usbToTouchscreen = [&]() {
            size_t size = 0;

            while (_cdcPassthrough.cdcRead(_rxBuffer, size, TSCREEN_CDC_PASSTHROUGH_BUFFER_SIZE))
            {
                for (size_t i = 0; i < size; i++)
                {
                    _cdcPassthrough.uartWrite(_rxBuffer[i]);
                }
            }
        };

        touchscreenToUSB();
        usbToTouchscreen();
    }
}

size_t Touchscreen::maxComponentUpdateIndex()
{
    return 0;
}

void Touchscreen::registerModel(model_t model, Model* instance)
{
    _models[static_cast<size_t>(model)] = instance;
}

/// Switches to requested screen on display
/// param [in]: screenID  Index of screen to display.
void Touchscreen::setScreen(size_t screenID)
{
    if (!isInitialized(mode_t::normal))
    {
        return;
    }

    modelInstance(_activeModel)->setScreen(screenID);
    _activeScreenID = screenID;
    screenChangeHandler(screenID);
}

/// Used to retrieve currently active screen on display.
/// returns: Active display screen.
size_t Touchscreen::activeScreen()
{
    return _activeScreenID;
}

void Touchscreen::setIconState(size_t index, bool state)
{
    if (!isInitialized(mode_t::normal))
    {
        return;
    }

    if (index >= Collection::size())
    {
        return;
    }

    icon_t icon;

    icon.onScreen  = _database.read(Database::Config::Section::touchscreen_t::onScreen, index);
    icon.offScreen = _database.read(Database::Config::Section::touchscreen_t::offScreen, index);

    if (icon.onScreen == icon.offScreen)
    {
        return;    // invalid screen indexes
    }

    if ((_activeScreenID != icon.onScreen) && (_activeScreenID != icon.offScreen))
    {
        return;    // don't allow setting icon on wrong screen
    }

    icon.xPos   = _database.read(Database::Config::Section::touchscreen_t::xPos, index);
    icon.yPos   = _database.read(Database::Config::Section::touchscreen_t::yPos, index);
    icon.width  = _database.read(Database::Config::Section::touchscreen_t::width, index);
    icon.height = _database.read(Database::Config::Section::touchscreen_t::height, index);

    modelInstance(_activeModel)->setIconState(icon, state);
}

void Touchscreen::processButton(const size_t buttonID, const bool state)
{
    bool   changeScreen = false;
    size_t newScreen    = 0;

    if (_database.read(Database::Config::Section::touchscreen_t::pageSwitchEnabled, buttonID))
    {
        changeScreen = true;
        newScreen    = _database.read(Database::Config::Section::touchscreen_t::pageSwitchIndex, buttonID);
    }

    buttonHandler(buttonID, state);

    // if the button should change screen, change it immediately
    // this will result in button never sending off state so do it manually first
    if (changeScreen)
    {
        buttonHandler(buttonID, false);
        setScreen(newScreen);
    }
}

bool Touchscreen::setBrightness(brightness_t brightness)
{
    if (!isInitialized(mode_t::normal))
    {
        return false;
    }

    return modelInstance(_activeModel)->setBrightness(brightness);
}

bool Touchscreen::isInitialized() const
{
    return _initialized;
}

bool Touchscreen::isInitialized(mode_t mode) const
{
    switch (mode)
    {
    case mode_t::normal:
        return _initialized && (_mode == mode);

    case mode_t::cdcPassthrough:
        return _mode == mode;

    default:
        return false;
    }
}

Touchscreen::Model* Touchscreen::modelInstance(model_t model)
{
    return _models[static_cast<size_t>(model)];
}

void Touchscreen::buttonHandler(size_t index, bool state)
{
    Messaging::event_t event;

    event.componentIndex = index;
    event.midiValue      = state;

    // mark this as forwarding message type - further action/processing is required
    MIDIDispatcher.notify(Messaging::eventSource_t::touchscreenButton,
                          event,
                          Messaging::listenType_t::fwd);
}

void Touchscreen::screenChangeHandler(size_t screenID)
{
    Messaging::event_t event;

    event.componentIndex = screenID;

    // mark this as forwarding message type - further action/processing is required
    MIDIDispatcher.notify(Messaging::eventSource_t::touchscreenScreen,
                          event,
                          Messaging::listenType_t::fwd);
}

std::optional<uint8_t> Touchscreen::sysConfigGet(System::Config::Section::touchscreen_t section, size_t index, uint16_t& value)
{
    if (!isInitialized() && _hwa.allocated(IO::Common::interface_t::uart))
    {
        return System::Config::status_t::serialPeripheralAllocatedError;
    }

    switch (section)
    {
    case System::Config::Section::touchscreen_t::setting:
    {
        switch (index)
        {
        case static_cast<size_t>(setting_t::cdcPassthrough):
        {
            if (_cdcPassthrough.allocated(IO::Common::interface_t::cdc))
            {
                return System::Config::status_t::cdcAllocatedError;
            }
        }
        break;

        default:
            break;
        }
    }
    break;

    default:
        break;
    }

    int32_t readValue;
    auto    result = _database.read(Util::Conversion::sys2DBsection(section), index, readValue) ? System::Config::status_t::ack : System::Config::status_t::errorRead;

    value = readValue;
    return result;
}

std::optional<uint8_t> Touchscreen::sysConfigSet(System::Config::Section::touchscreen_t section, size_t index, uint16_t value)
{
    if (!isInitialized() && _hwa.allocated(IO::Common::interface_t::uart))
    {
        return System::Config::status_t::serialPeripheralAllocatedError;
    }

    auto initAction = Common::initAction_t::asIs;
    auto mode       = mode_t::normal;
    bool writeToDb  = true;

    switch (section)
    {
    case System::Config::Section::touchscreen_t::setting:
    {
        switch (index)
        {
        case static_cast<size_t>(setting_t::enable):
        {
            if (value)
            {
                initAction = Common::initAction_t::init;
            }
            else
            {
                initAction = Common::initAction_t::deInit;
            }
        }
        break;

        case static_cast<size_t>(setting_t::model):
        {
            if (value >= static_cast<size_t>(model_t::AMOUNT))
            {
                return System::Config::status_t::errorNewValue;
            }

            initAction = Common::initAction_t::init;
        }
        break;

        case static_cast<size_t>(setting_t::brightness):
        {
            if (isInitialized())
            {
                if (!setBrightness(static_cast<brightness_t>(value)))
                {
                    return System::Config::status_t::errorWrite;
                }
            }
        }
        break;

        case static_cast<size_t>(setting_t::initialScreen):
        {
            if (isInitialized())
            {
                setScreen(value);
            }
        }
        break;

        case static_cast<size_t>(IO::Touchscreen::setting_t::cdcPassthrough):
        {
            if (_cdcPassthrough.allocated(IO::Common::interface_t::cdc))
            {
                return System::Config::status_t::cdcAllocatedError;
            }

            mode       = mode_t::cdcPassthrough;
            initAction = value ? Common::initAction_t::init : Common::initAction_t::deInit;
            writeToDb  = false;
        }
        break;

        default:
            break;
        }
    }
    break;

    default:
        break;
    }

    bool result = true;

    if (writeToDb)
    {
        result = _database.update(Util::Conversion::sys2DBsection(section), index, value);
    }

    if (result)
    {
        if (initAction == Common::initAction_t::init)
        {
            init(mode);
        }
        else if (initAction == Common::initAction_t::deInit)
        {
            deInit(mode);
        }

        return System::Config::status_t::ack;
    }

    return System::Config::status_t::errorWrite;
}
