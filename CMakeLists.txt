# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

# Importa o Pico SDK
include(pico_sdk_import.cmake)

# Define a placa como Pico W
set(PICO_BOARD pico_w)

project(HydroSense)

# Inicializa o Pico SDK
pico_sdk_init()

# Configura o FreeRTOS para RP2040
set(FREERTOS_CONFIG_FILE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} CACHE STRING "")
set(FREERTOS_PORT GCC_RP2040 CACHE STRING "")

# Adiciona o FreeRTOS
add_subdirectory(FreeRTOS-Kernel)

# Arquivos fonte do projeto HydroSense completo
set(SOURCES
    src/hydrosense_main.c
    src/hydrosense_servo.c
    src/hydrosense_botoes.c
    src/hydrosense_oled.c
    src/hydrosense_utils.c
    src/tasks/feeding_task.c
    src/tasks/monitoring_task.c
    src/tasks/automation_task.c
    src/config/system_config.c
)

# Adiciona o executável
add_executable(HydroSense ${SOURCES})

# Inclui diretórios de header
target_include_directories(HydroSense PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    include/
    include/actuators/
    include/display/
    include/sensors/
    include/config/
    include/tasks/
    FreeRTOS-Kernel/include/
    FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/include/
)

# Bibliotecas necessárias
target_link_libraries(HydroSense
    pico_stdlib
    hardware_pwm
    hardware_adc
    hardware_i2c
    hardware_gpio
    pico_multicore
    hardware_uart
    hardware_rtc
    hardware_timer
    freertos_kernel
)

# Habilita USB e UART
pico_enable_stdio_usb(HydroSense 1)
pico_enable_stdio_uart(HydroSense 0)

# Cria arquivo UF2
pico_add_extra_outputs(HydroSense)