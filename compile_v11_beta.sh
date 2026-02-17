#!/bin/bash

# Script para compilar apenas o hydrosense_v11_beta.c
echo "🔨 Compilando HydroSense v11 Beta (Anti-Sifão)"
echo "================================================================="

# Verifica se o PICO_SDK_PATH está definido
if [ -z "$PICO_SDK_PATH" ]; then
    echo "⚠️  PICO_SDK_PATH não está definido. Tentando usar /tmp/pico-sdk..."
    export PICO_SDK_PATH=/tmp/pico-sdk
fi

if [ ! -d "$PICO_SDK_PATH" ]; then
    echo "❌ Erro: Pico SDK não encontrado em $PICO_SDK_PATH"
    echo "   Por favor, defina PICO_SDK_PATH ou instale o Pico SDK"
    exit 1
fi

echo "✅ Usando Pico SDK: $PICO_SDK_PATH"

# Criar diretório de build específico
BUILD_DIR="build_v11_beta"
rm -rf $BUILD_DIR
mkdir -p $BUILD_DIR
cd $BUILD_DIR

# Criar CMakeLists.txt específico para v11_beta
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.13)

# Define PICO_BOARD BEFORE including pico_sdk_import.cmake
set(PICO_BOARD pico_w CACHE STRING "Board type")

include(pico_sdk_import.cmake)

project(hydrosense_v11_beta C CXX ASM)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(hydrosense_v11_beta
    ../src/hydrosense_v11_beta.c
)

target_include_directories(hydrosense_v11_beta PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(hydrosense_v11_beta 
    pico_stdlib 
    pico_cyw43_arch_lwip_threadsafe_background
    hardware_i2c
    hardware_gpio
    hardware_pwm
    hardware_adc
)

pico_add_extra_outputs(hydrosense_v11_beta)
pico_enable_stdio_usb(hydrosense_v11_beta 1)
pico_enable_stdio_uart(hydrosense_v11_beta 0)
EOF

# Copiar arquivos necessários
cp ../pico_sdk_import.cmake .
cp ../lwipopts.h .

echo ""
echo "📁 Configurando CMake (usando arquivo local)..."
cmake . -DPICO_SDK_PATH=$PICO_SDK_PATH

if [ $? -ne 0 ]; then
    echo "❌ Erro ao configurar CMake!"
    exit 1
fi

echo ""
echo "🔨 Compilando..."
make -j$(nproc)

if [ $? -ne 0 ]; then
    echo "❌ Erro na compilação!"
    exit 1
fi

echo ""
if [ -f "hydrosense_v11_beta.uf2" ]; then
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo "🎉 COMPILAÇÃO CONCLUÍDA COM SUCESSO!"
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
    echo ""
    echo "📁 Arquivo gerado: hydrosense_v11_beta.uf2"
    echo "📊 Tamanho: $(ls -lh hydrosense_v11_beta.uf2 | awk '{print $5}')"
    echo ""
    echo "🚀 Para gravar no Pico W:"
    echo "   1. Segure o botão BOOTSEL e conecte o Pico W ao PC"
    echo "   2. Copie o arquivo para o drive RPI-RP2:"
    echo "      cp build_v11_beta/hydrosense_v11_beta.uf2 /media/\$USER/RPI-RP2/"
    echo ""
    echo "📍 Localização completa do arquivo:"
    ls -la hydrosense_v11_beta.uf2
    echo ""
    echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
else
    echo "❌ Erro: Arquivo .uf2 não foi gerado!"
    echo "   Verifique os erros de compilação acima."
    exit 1
fi
