vpath application/%.cpp ../src
vpath common/%.cpp ../src
vpath modules/%.cpp ../
vpath modules/%.c ../

SOURCES_COMMON := \
modules/unity/src/unity.c

#common for all targets
ifeq (,$(findstring USB_LINK_MCU,$(DEFINES)))
    SOURCES_COMMON += \
    modules/dbms/src/LESSDB.cpp \
    modules/midi/src/MIDI.cpp \
    modules/u8g2/csrc/u8x8_string.c \
    modules/u8g2/csrc/u8x8_setup.c \
    modules/u8g2/csrc/u8x8_u8toa.c \
    modules/u8g2/csrc/u8x8_8x8.c \
    modules/u8g2/csrc/u8x8_u16toa.c \
    modules/u8g2/csrc/u8x8_display.c \
    modules/u8g2/csrc/u8x8_fonts.c \
    modules/u8g2/csrc/u8x8_byte.c \
    modules/u8g2/csrc/u8x8_cad.c \
    modules/u8g2/csrc/u8x8_gpio.c \
    modules/u8g2/csrc/u8x8_d_ssd1306_128x64_noname.c \
    modules/u8g2/csrc/u8x8_d_ssd1306_128x32.c \
    $(BOARD_TARGET_DIR)/$(TARGET).cpp \
    stubs/Board.cpp \
    stubs/Core.cpp \
    application/util/messaging/Messaging.cpp \
    application/util/scheduler/Scheduler.cpp
endif

ifeq ($(ARCH),stm32)
    SOURCES_COMMON += modules/EmuEEPROM/src/EmuEEPROM.cpp
endif

#common include dirs
INCLUDE_DIRS_COMMON := \
-I"./" \
-I"./stubs" \
-I"./unity" \
-I"../modules/" \
-I"../src/" \
-I"../src/bootloader/" \
-I"../src/application/" \
-I"../src/board/common" \
-I"../src/board/arch/$(ARCH)/variants/$(MCU_FAMILY)" \
-I"../src/$(MCU_DIR)" \
-I"../src/$(BOARD_MCU_BASE_DIR)/$(MCU)" \
-I"../src/$(BOARD_TARGET_DIR)/"
