#!/bin/bash
# Compila HydroSense v7 com servidor web IoT
# Execute: ./compile_v7.sh

set -e
cd "$(dirname "$0")"

echo "╔════════════════════════════════════════╗"
echo "║   Compilando HydroSense v7 - IoT       ║"
echo "╚════════════════════════════════════════╝"
echo ""

# Limpa e cria diretório build
rm -rf build_v7
mkdir -p build_v7
cd build_v7

# Copia arquivos necessários
cp ../pico_sdk_import.cmake .

# Cria CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)

include(pico_sdk_import.cmake)

project(hydrosense_v7 C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(hydrosense_v7
    ../src/hydrosense_v7.c
)

target_include_directories(hydrosense_v7 PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/../include
)

target_link_libraries(hydrosense_v7
    pico_stdlib
    pico_cyw43_arch_lwip_poll
    hardware_i2c
    hardware_pwm
    hardware_gpio
    hardware_adc
)

pico_enable_stdio_usb(hydrosense_v7 1)
pico_enable_stdio_uart(hydrosense_v7 0)

pico_add_extra_outputs(hydrosense_v7)
EOF

echo "📦 Configurando CMake..."
cmake . -DPICO_BOARD=pico_w 2>&1 | grep -E "(PICO|Target|Configuring|Generating|Build)" || true

echo ""
echo "🔨 Compilando..."
make -j4 2>&1 | tail -20

if [ -f hydrosense_v7.uf2 ]; then
    echo ""
    echo "╔════════════════════════════════════════╗"
    echo "║   ✅ COMPILADO COM SUCESSO!            ║"
    echo "╚════════════════════════════════════════╝"
    echo ""
    echo "📁 Arquivo: build_v7/hydrosense_v7.uf2"
    echo ""
    echo "📋 Para gravar no Pico W:"
    echo "   1. Pressione BOOTSEL e conecte o USB"
    echo "   2. Execute: ./flash_v7.sh"
    echo ""
    
    # Cria script de flash
    cat > ../flash_v7.sh << 'FLASH'
#!/bin/bash
UF2="$(dirname "$0")/build_v7/hydrosense_v7.uf2"
DEST="/media/$USER/RPI-RP2"

if [ -d "$DEST" ]; then
    cp "$UF2" "$DEST/"
    echo "✅ Firmware gravado! Aguarde o Pico reiniciar..."
else
    # Tenta variações
    for d in /media/$USER/RPI-RP2*; do
        if [ -d "$d" ]; then
            cp "$UF2" "$d/"
            echo "✅ Firmware gravado em $d! Aguarde o Pico reiniciar..."
            exit 0
        fi
    done
    echo "⚠️ Conecte o Pico W segurando BOOTSEL e tente novamente"
fi
FLASH
    chmod +x ../flash_v7.sh
    
else
    echo ""
    echo "❌ ERRO na compilação!"
    exit 1
fi
