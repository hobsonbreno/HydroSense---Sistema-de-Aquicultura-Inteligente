#!/bin/bash
# Compila HydroSense v9 - WiFi Simplificado

cd "$(dirname "$0")"

# Cria diretório de build
mkdir -p build_v9
cd build_v9

# CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include($ENV{HOME}/.pico-sdk/sdk/2.1.1/pico_sdk_init.cmake)

project(hydrosense_v9 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(hydrosense_v9 ../src/hydrosense_v9.c)

target_include_directories(hydrosense_v9 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_link_libraries(hydrosense_v9
    pico_stdlib
    hardware_i2c
    hardware_gpio
    hardware_pwm
    pico_cyw43_arch_lwip_poll
)

pico_enable_stdio_usb(hydrosense_v9 1)
pico_enable_stdio_uart(hydrosense_v9 0)

pico_add_extra_outputs(hydrosense_v9)
EOF

# Compila
echo "🔨 Compilando HydroSense v9..."
cmake . > /dev/null 2>&1
make -j4 2>&1 | tail -5

echo ""
if [ -f hydrosense_v9.uf2 ]; then
    echo "✅ Compilado: build_v9/hydrosense_v9.uf2"
    echo ""
    echo "📋 DIFERENÇAS DA V9:"
    echo "   • WiFi SIMPLIFICADO (sem scan que travava)"
    echo "   • Conecta DIRETO à rede (como MicroPython)" 
    echo "   • Sensores SIMULADOS para demonstração"
    echo "   • Interface web responsiva"
    echo "   • Timeout configurável (30s)"
    echo ""
    echo "🔗 Após conectar, acesse: http://[IP-DO-PICO]"
else
    echo "❌ Erro na compilação"
fi