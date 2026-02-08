#!/bin/bash
# Compila teste de WiFi

cd "$(dirname "$0")"

# Cria diretório de build
mkdir -p build_wifi_test
cd build_wifi_test

# CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include($ENV{HOME}/.pico-sdk/sdk/2.1.1/pico_sdk_init.cmake)

project(wifi_test C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(wifi_test ../src/wifi_test.c)

target_include_directories(wifi_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../include)

target_link_libraries(wifi_test
    pico_stdlib
    pico_cyw43_arch_lwip_poll
)

pico_enable_stdio_usb(wifi_test 1)
pico_enable_stdio_uart(wifi_test 0)

pico_add_extra_outputs(wifi_test)
EOF

# Compila
cmake . > /dev/null 2>&1
make -j4 2>&1 | tail -5

echo ""
if [ -f wifi_test.uf2 ]; then
    echo "✅ Compilado: build_wifi_test/wifi_test.uf2"
else
    echo "❌ Erro na compilação"
fi
