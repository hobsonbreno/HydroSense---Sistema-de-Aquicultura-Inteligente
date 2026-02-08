#!/bin/bash
# Compila HydroSense v6 com servidor web

cd "$(dirname "$0")"

# Cria diretório build
rm -rf build_v6
mkdir -p build_v6
cd build_v6

# Copia pico_sdk_import.cmake
cp ../pico_sdk_import.cmake .

# Cria CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)

include(pico_sdk_import.cmake)

project(hydrosense_v6 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(hydrosense_v6
    ../src/hydrosense_v6.c
)

target_include_directories(hydrosense_v6 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_link_libraries(hydrosense_v6
    pico_stdlib
    pico_cyw43_arch_lwip_poll
    hardware_i2c
    hardware_pwm
    hardware_gpio
)

pico_enable_stdio_usb(hydrosense_v6 1)
pico_enable_stdio_uart(hydrosense_v6 0)

pico_add_extra_outputs(hydrosense_v6)
EOF

echo "Configurando CMake..."
cmake . -DPICO_BOARD=pico_w

echo ""
echo "Compilando..."
make -j4

if [ -f hydrosense_v6.uf2 ]; then
    echo ""
    echo "=========================================="
    echo "✅ COMPILADO COM SUCESSO!"
    echo ""
    echo "Arquivo: build_v6/hydrosense_v6.uf2"
    echo ""
    echo "Para gravar no Pico W:"
    echo "  1. Pressione BOOTSEL e conecte o USB"
    echo "  2. Execute: cp build_v6/hydrosense_v6.uf2 /media/$USER/RPI-RP2/"
    echo "=========================================="
else
    echo "❌ ERRO na compilação!"
fi
