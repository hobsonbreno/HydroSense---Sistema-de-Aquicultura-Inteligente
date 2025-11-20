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

# Adiciona o executável
add_executable(HydroSense
    src/main.c
    src/sensors/temperature_sensor.c
    src/sensors/water_level_sensor.c
    src/sensors/ph_sensor.c
    src/sensors/turbidity_sensor.c
    src/actuators/pump_controller.c
    src/actuators/servo_feeder.c
    src/communication/mqtt_client.c
    src/tasks/monitoring_task.c
    src/tasks/automation_task.c
    src/tasks/feeding_task.c
    src/config/system_config.c
    arch/sys_arch.c
)

# Inclui diretórios de header
target_include_directories(HydroSense PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    include/
    include/sensors/
    include/actuators/
    include/communication/
    include/utils/
    include/tasks/
    include/config/
    FreeRTOS-Kernel/include/
    FreeRTOS-Kernel/portable/ThirdParty/GCC/RP2040/include/
)

# Bibliotecas necessárias
target_link_libraries(HydroSense
    pico_stdlib
    hardware_pwm
    hardware_adc
    hardware_i2c
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