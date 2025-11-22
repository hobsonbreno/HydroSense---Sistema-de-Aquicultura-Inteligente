cmake_minimum_required(VERSION 3.13)

# Inclui o SDK do Pico
include(pico_sdk_import.cmake)

project(test_orientacao_final C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# Inicializa o SDK do Pico
pico_sdk_init()

# Cria o executável APENAS com o arquivo de teste
add_executable(test_orientacao_final
    ../src/test_orientacao_final.c
)

# Link com as bibliotecas essenciais do Pico
target_link_libraries(test_orientacao_final
    pico_stdlib
    hardware_i2c
    hardware_gpio
)

# Habilita USB para stdout
pico_enable_stdio_usb(test_orientacao_final 1)
pico_enable_stdio_uart(test_orientacao_final 0)

# Cria arquivos UF2
pico_add_extra_outputs(test_orientacao_final)