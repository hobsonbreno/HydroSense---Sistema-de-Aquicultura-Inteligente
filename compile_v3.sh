#!/bin/bash
# ============================================================
# HydroSense v3.0 - Script de Compilação
# ============================================================

set -e

echo ""
echo "╔══════════════════════════════════════════════════════════════╗"
echo "║   🐟 HydroSense v3.0 - Compilando...                         ║"
echo "╚══════════════════════════════════════════════════════════════╝"
echo ""

# Define diretórios
PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_DIR}/build_v3"

# Cria diretório de build se não existir
if [ ! -d "$BUILD_DIR" ]; then
    echo "📁 Criando diretório de build..."
    mkdir -p "$BUILD_DIR"
fi

cd "$BUILD_DIR"

# Copia CMakeLists_v3.txt como CMakeLists.txt
echo "📋 Configurando arquivos de build..."
cp "${PROJECT_DIR}/CMakeLists_v3.txt" "${BUILD_DIR}/CMakeLists.txt"
cp "${PROJECT_DIR}/pico_sdk_import.cmake" "${BUILD_DIR}/pico_sdk_import.cmake"
cp "${PROJECT_DIR}/FreeRTOSConfig.h" "${BUILD_DIR}/FreeRTOSConfig.h"
cp "${PROJECT_DIR}/lwipopts.h" "${BUILD_DIR}/lwipopts.h"

# Cria link simbólico para FreeRTOS-Kernel
if [ ! -d "${BUILD_DIR}/FreeRTOS-Kernel" ]; then
    ln -sf "${PROJECT_DIR}/FreeRTOS-Kernel" "${BUILD_DIR}/FreeRTOS-Kernel"
fi

# Cria link simbólico para src e include
if [ ! -d "${BUILD_DIR}/src" ]; then
    ln -sf "${PROJECT_DIR}/src" "${BUILD_DIR}/src"
fi
if [ ! -d "${BUILD_DIR}/include" ]; then
    ln -sf "${PROJECT_DIR}/include" "${BUILD_DIR}/include"
fi
if [ ! -d "${BUILD_DIR}/arch" ]; then
    ln -sf "${PROJECT_DIR}/arch" "${BUILD_DIR}/arch"
fi

# Executa CMake
echo "⚙️ Executando CMake..."
cmake -G "Unix Makefiles" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DPICO_BOARD=pico_w \
    .

# Compila
echo ""
echo "🔨 Compilando..."
make -j$(nproc)

# Verifica se o arquivo UF2 foi gerado
if [ -f "HydroSense_v3.uf2" ]; then
    echo ""
    echo "╔══════════════════════════════════════════════════════════════╗"
    echo "║   ✅ Compilação concluída com sucesso!                       ║"
    echo "╠══════════════════════════════════════════════════════════════╣"
    echo "║   📦 Arquivo: build_v3/HydroSense_v3.uf2                     ║"
    echo "║   📊 Tamanho: $(du -h HydroSense_v3.uf2 | cut -f1)                                          ║"
    echo "║                                                              ║"
    echo "║   🔌 Para instalar:                                          ║"
    echo "║   1. Conecte o Pico W com BOOTSEL pressionado                ║"
    echo "║   2. Copie HydroSense_v3.uf2 para o drive RPI-RP2            ║"
    echo "╚══════════════════════════════════════════════════════════════╝"
    echo ""
else
    echo ""
    echo "❌ ERRO: Arquivo UF2 não foi gerado!"
    exit 1
fi
