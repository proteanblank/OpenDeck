#!/bin/bash

MCU_DEF_FILE=$1
GEN_DIR=$2
YAML_PARSER="dasel -n -p yaml --plain -f"
OUT_FILE_HEADER="$GEN_DIR"/MCU.h
OUT_FILE_SOURCE="$GEN_DIR"/MCU.cpp.include
OUT_FILE_MAKEFILE="$GEN_DIR"/MCU.mk

mkdir -p "$GEN_DIR"

mcu=$(basename "$MCU_DEF_FILE" .yml)
arch=$($YAML_PARSER "$MCU_DEF_FILE" arch)
mcu_family=$($YAML_PARSER "$MCU_DEF_FILE" mcuFamily)
cpu=$($YAML_PARSER "$MCU_DEF_FILE" cpu)
fpu=$($YAML_PARSER "$MCU_DEF_FILE" fpu)
float_abi=$($YAML_PARSER "$MCU_DEF_FILE" float-abi)
define_symbol=$($YAML_PARSER "$MCU_DEF_FILE" define-symbol)
app_start_address=$($YAML_PARSER "$MCU_DEF_FILE" flash.app-start)
boot_start_address=$($YAML_PARSER "$MCU_DEF_FILE" flash.boot-start)
metadata_start_address=$($YAML_PARSER "$MCU_DEF_FILE" flash.metadata-start)

{
    printf "%s\n\n" "#pragma once"

    if [[ $mcu == *"stm32"* ]]
    then
        printf "%s\n\n" "#include \"stm32f4xx_hal.h\""
    fi
} > "$OUT_FILE_HEADER"

{
    printf "%s\n" "#include \"MCU.h\""
    printf "%s\n" "#include \"board/Board.h\""
    printf "%s\n\n" "#include \"board/Internal.h\""
} > "$OUT_FILE_SOURCE"

{
    printf "%s\n" "ARCH := $arch"
    printf "%s\n" "MCU_FAMILY := $mcu_family"
    #base mcu without the variant-specific letters at the end
    printf "%s\n" "MCU_BASE := $(echo "$mcu" | rev | cut -c3- | rev)"
    printf "%s\n" "MCU := $mcu"
    printf "%s\n" "MCU_DIR := board/arch/$arch/variants/$mcu_family/$mcu"
    printf "%s\n" "CPU := $cpu"
    printf "%s\n" "FPU := $fpu"
    printf "%s\n" "FLOAT-ABI := $float_abi"
    printf "%s\n" "DEFINES += $define_symbol"
    printf "%s%x\n" "APP_START_ADDR := 0x" "$app_start_address"
    printf "%s%x\n" "BOOT_START_ADDR := 0x" "$boot_start_address"
    printf "%s%x\n" "FW_METADATA_LOCATION := 0x" "$metadata_start_address"
} >> "$OUT_FILE_MAKEFILE"

if [[ $($YAML_PARSER "$MCU_DEF_FILE" flash) != "null" ]]
then
    printf "%s\n\n" "namespace {" >> "$OUT_FILE_SOURCE"

    declare -i number_of_flash_pages

    if [[ $($YAML_PARSER "$MCU_DEF_FILE" flash.pages) != "null" ]]
    then
        number_of_flash_pages=$($YAML_PARSER "$MCU_DEF_FILE" flash.pages --length)
        printf "%s\n\n" "#define TOTAL_FLASH_PAGES $number_of_flash_pages" >> "$OUT_FILE_HEADER"

        {
            printf "%s\n" "Board::detail::map::flashPage_t pageDescriptor[TOTAL_FLASH_PAGES] = {"
        } >> "$OUT_FILE_SOURCE"

        for ((i=0; i<number_of_flash_pages; i++))
        do
            addressStart=$($YAML_PARSER "$MCU_DEF_FILE" flash.pages.["$i"].address)
            page_size=$($YAML_PARSER "$MCU_DEF_FILE" flash.pages.["$i"].size)
            addressEnd=$((addressStart+page_size-1))
            app_size=$($YAML_PARSER "$MCU_DEF_FILE" flash.pages.["$i"].app-size)

            #based on provided app start page address, find its index and create a new symbol in header
            #this could be done in application as well, but it's done here to avoid extra processing and flash usage
            if [[ ($addressStart -le $app_start_address) && ($app_start_address -le $addressEnd) ]]
            then
                printf "%s\n" "#define FLASH_PAGE_APP_START $i" >> "$OUT_FILE_HEADER"
            fi

            if [[ $app_size == "null" ]]
            then
                app_size=$page_size
            fi

            {
                printf "%s\n" "#define FLASH_PAGE_ADDRESS_${i} $addressStart"
                printf "%s\n" "#ifdef FW_BOOT"
                printf "%s\n" "#define FLASH_PAGE_SIZE_${i} $page_size"
                printf "%s\n" "#else"
                printf "%s\n" "#define FLASH_PAGE_SIZE_${i} $app_size"
                printf "%s\n" "#endif"
            } >> "$OUT_FILE_HEADER"

            {
                printf "%s\n" "{"
                printf "%s\n" ".address = $addressStart",
                printf "%s\n" "#ifdef FW_BOOT"
                printf "%s\n" ".size = $page_size",
                printf "%s\n" "#else"
                printf "%s\n" ".size = $app_size",
                printf "%s\n" "#endif"
                printf "%s\n" "},"
            } >> "$OUT_FILE_SOURCE"
        done
    else
        page_size=$($YAML_PARSER "$MCU_DEF_FILE" flash.page-size)
        flash_size=$($YAML_PARSER "$MCU_DEF_FILE" flash.size)
        number_of_flash_pages=$((flash_size/page_size))

        {
            printf "%s\n" "#define TOTAL_FLASH_PAGES $number_of_flash_pages"
            printf "%s\n" "#define FLASH_PAGE_SIZE_COMMON $page_size"
            printf "%s\n" "#define FLASH_END $((flash_size-1))"
        } >> "$OUT_FILE_HEADER"

        addressStart=0

        for ((i=0; i<number_of_flash_pages; i++))
        do
            addressEnd=$((addressStart+page_size-1))

            {
                printf "%s\n" "#define FLASH_PAGE_ADDRESS_${i} $addressStart"
                printf "%s\n" "#define FLASH_PAGE_SIZE_${i} $page_size"
            } >> "$OUT_FILE_HEADER"

            if [[ ($addressStart -le $app_start_address) && ($app_start_address -le $addressEnd) ]]
            then
                printf "%s\n" "#define FLASH_PAGE_APP_START $i" >> "$OUT_FILE_HEADER"
            fi

            ((addressStart+=page_size))
        done

        {
            printf "%s\n" "Board::detail::map::flashPage_t pageDescriptor = {"
            printf "%s\n" ".address = 0,"
            printf "%s\n" ".size = FLASH_PAGE_SIZE_COMMON,"
        } >> "$OUT_FILE_SOURCE"
    fi

    {
        printf "%s\n" "};"
        printf "%s\n\n" "}"
    } >> "$OUT_FILE_SOURCE"
fi

if [[ $($YAML_PARSER "$MCU_DEF_FILE" eeprom) != "null" ]]
then
    if [[ $($YAML_PARSER "$MCU_DEF_FILE" eeprom.emulated) != "null" ]]
    then
        factory_flash_page=$($YAML_PARSER "$MCU_DEF_FILE"  eeprom.emulated.factory-flash-page)
        eeprom_flash_page_1=$($YAML_PARSER "$MCU_DEF_FILE" eeprom.emulated.eeprom-flash-page1)
        eeprom_flash_page_2=$($YAML_PARSER "$MCU_DEF_FILE" eeprom.emulated.eeprom-flash-page2)

        {
            printf "%s\n" "#define FLASH_PAGE_FACTORY   $factory_flash_page"
            printf "%s\n" "#define FLASH_PAGE_EEPROM_1  $eeprom_flash_page_1"
            printf "%s\n" "#define FLASH_PAGE_EEPROM_2  $eeprom_flash_page_2"

            printf "%s\n" "#define _FLASH_PAGE_ADDRESS_GEN(x) FLASH_PAGE_ADDRESS_##x"
            printf "%s\n" "#define FLASH_PAGE_ADDRESS(x)      _FLASH_PAGE_ADDRESS_GEN(x)"

            printf "%s\n" "#define _FLASH_PAGE_SIZE_GEN(x) FLASH_PAGE_SIZE_##x"
            printf "%s\n" "#define FLASH_PAGE_SIZE(x)      _FLASH_PAGE_SIZE_GEN(x)"
        } >> "$OUT_FILE_HEADER"
    else
        eeprom_size=$($YAML_PARSER "$MCU_DEF_FILE" eeprom.size)
        printf "%s\n" "#define EEPROM_END $((eeprom_size-1))" >> "$OUT_FILE_HEADER"
    fi
fi

if [[ $($YAML_PARSER "$MCU_DEF_FILE" hal) != "null" ]]
then
    adc_instance=$($YAML_PARSER "$MCU_DEF_FILE" hal.adc)
    main_timer_instance=$($YAML_PARSER "$MCU_DEF_FILE" hal.timers.main)
    pwm_timer_instance=$($YAML_PARSER "$MCU_DEF_FILE" hal.timers.pwm)

    {
        printf "%s\n" "#define ADC_INSTANCE         $adc_instance"
        printf "%s\n" "#define MAIN_TIMER_INSTANCE  $main_timer_instance"
        printf "%s\n" "#define PWM_TIMER_INSTANCE   $pwm_timer_instance"
    } >> "$OUT_FILE_HEADER"

    pllm_8mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse8MHz.pllm)
    plln_8mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse8MHz.plln)
    pllq_8mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse8MHz.pllq)
    pllp_8mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse8MHz.pllp)
    ahb_clk_div_8mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse8MHz.ahb_clk_div)
    apb1_clk_div_8mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse8MHz.apb1_clk_div)
    apb2_clk_div_8mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse8MHz.apb2_clk_div)

    pllm_16mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse16MHz.pllm)
    plln_16mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse16MHz.plln)
    pllq_16mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse16MHz.pllq)
    pllp_16mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse16MHz.pllp)
    ahb_clk_div_16mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse16MHz.ahb_clk_div)
    apb1_clk_div_16mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse16MHz.apb1_clk_div)
    apb2_clk_div_16mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse16MHz.apb2_clk_div)

    pllm_25mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse25MHz.pllm)
    plln_25mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse25MHz.plln)
    pllq_25mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse25MHz.pllq)
    pllp_25mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse25MHz.pllp)
    ahb_clk_div_25mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse25MHz.ahb_clk_div)
    apb1_clk_div_25mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse25MHz.apb1_clk_div)
    apb2_clk_div_25mhz=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.hse25MHz.apb2_clk_div)

    vreg_scale=$($YAML_PARSER "$MCU_DEF_FILE" hal.clocks.voltage_scale)

    {
        printf "%s\n" "#if (HSE_VALUE == 8000000)"
        printf "%s\n" "#define HSE_PLLM $pllm_8mhz"
        printf "%s\n" "#define HSE_PLLN $plln_8mhz"
        printf "%s\n" "#define HSE_PLLQ $pllq_8mhz"
        printf "%s\n" "#define HSE_PLLP $pllp_8mhz"
        printf "%s\n" "#define AHB_CLK_DIV $ahb_clk_div_8mhz"
        printf "%s\n" "#define APB1_CLK_DIV $apb1_clk_div_8mhz"
        printf "%s\n" "#define APB2_CLK_DIV $apb2_clk_div_8mhz"
        printf "%s\n" "#elif (HSE_VALUE == 16000000)"
        printf "%s\n" "#define HSE_PLLM $pllm_16mhz"
        printf "%s\n" "#define HSE_PLLN $plln_16mhz"
        printf "%s\n" "#define HSE_PLLQ $pllq_16mhz"
        printf "%s\n" "#define HSE_PLLP $pllp_16mhz"
        printf "%s\n" "#define AHB_CLK_DIV $ahb_clk_div_16mhz"
        printf "%s\n" "#define APB1_CLK_DIV $apb1_clk_div_16mhz"
        printf "%s\n" "#define APB2_CLK_DIV $apb2_clk_div_16mhz"
        printf "%s\n" "#elif (HSE_VALUE == 25000000)"
        printf "%s\n" "#define HSE_PLLM $pllm_25mhz"
        printf "%s\n" "#define HSE_PLLN $plln_25mhz"
        printf "%s\n" "#define HSE_PLLQ $pllq_25mhz"
        printf "%s\n" "#define HSE_PLLP $pllp_25mhz"
        printf "%s\n" "#define AHB_CLK_DIV $ahb_clk_div_25mhz"
        printf "%s\n" "#define APB1_CLK_DIV $apb1_clk_div_25mhz"
        printf "%s\n" "#define APB2_CLK_DIV $apb2_clk_div_25mhz"
        printf "%s\n" "#else"
        printf "%s\n" "#error Invalid clock value"
        printf "%s\n\n" "#endif"
        printf "%s\n\n" "#define PWR_REGULATOR_VOLTAGE_SCALE $vreg_scale"
    } >> "$OUT_FILE_HEADER"
fi

if [[ $($YAML_PARSER "$MCU_DEF_FILE" fuses) != "null" ]]
then
    fuse_unlock=$($YAML_PARSER "$MCU_DEF_FILE" fuses.unlock)
    fuse_lock=$($YAML_PARSER "$MCU_DEF_FILE" fuses.lock)
    fuse_ext=$($YAML_PARSER "$MCU_DEF_FILE" fuses.ext)
    fuse_high=$($YAML_PARSER "$MCU_DEF_FILE" fuses.high)
    fuse_low=$($YAML_PARSER "$MCU_DEF_FILE" fuses.low)

    {
        printf "%s\n" "FUSE_UNLOCK := $fuse_unlock"
        printf "%s\n" "FUSE_LOCK := $fuse_lock"
        printf "%s\n" "FUSE_EXT := $fuse_ext"
        printf "%s\n" "FUSE_HIGH := $fuse_high"
        printf "%s\n" "FUSE_LOW := $fuse_low"
    } >> "$OUT_FILE_MAKEFILE"
fi

number_of_uart_interfaces=$($YAML_PARSER "$MCU_DEF_FILE" interfaces.uart)
number_of_i2c_interfaces=$($YAML_PARSER "$MCU_DEF_FILE" interfaces.i2c)

{
    printf "%s\n" "#define MAX_UART_INTERFACES  $number_of_uart_interfaces"
    printf "%s\n" "#define MAX_I2C_INTERFACES   $number_of_i2c_interfaces"
} >> "$OUT_FILE_HEADER"