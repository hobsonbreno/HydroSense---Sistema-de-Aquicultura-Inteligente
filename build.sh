#!/bin/bash

# Script de build para HydroSense
# Automatiza o processo de compilação

set -e

PROJECT_DIR="/home/hobson007breno/Downloads/projeto final/HydroSense"
BUILD_DIR="$PROJECT_DIR/build"

echo "🔧 HydroSense Build Script"
echo "=========================="

# Navegar para o diretório do projeto
cd "$PROJECT_DIR"
echo "📁 Diretório: $(pwd)"

# Verificar se PICO_SDK_PATH está definido
if [ -z "$PICO_SDK_PATH" ]; then
    echo "⚠️  PICO_SDK_PATH não definido, tentando localizar..."
    
    # Procurar pelo SDK em locais comuns
    POSSIBLE_PATHS=(
        "/usr/share/pico-sdk"
        "/opt/pico-sdk"
        "$HOME/pico/pico-sdk"
        "$(pwd)/../pico-sdk"
    )
    
    for path in "${POSSIBLE_PATHS[@]}"; do
        if [ -d "$path" ]; then
            export PICO_SDK_PATH="$path"
            echo "✅ PICO_SDK encontrado: $PICO_SDK_PATH"
            break
        fi
    done
    
    if [ -z "$PICO_SDK_PATH" ]; then
        echo "❌ PICO_SDK_PATH não encontrado!"
        echo "💡 Instale o Pico SDK ou defina a variável PICO_SDK_PATH"
        exit 1
    fi
fi

# Criar diretório build se não existir
if [ ! -d "$BUILD_DIR" ]; then
    echo "📁 Criando diretório build..."
    mkdir -p "$BUILD_DIR"
fi

# Entrar no diretório build
cd "$BUILD_DIR"
echo "📁 Build dir: $(pwd)"

# Configurar CMake
echo "🔧 Configurando CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# Compilar
echo "🔨 Compilando projeto..."
make -j$(nproc)

# Verificar se o arquivo UF2 foi gerado
UF2_FILE="HydroSense.uf2"
if [ -f "$UF2_FILE" ]; then
    echo "✅ Compilação concluída com sucesso!"
    echo "📦 Arquivo gerado: $BUILD_DIR/$UF2_FILE"
    echo "📏 Tamanho: $(du -h "$UF2_FILE" | cut -f1)"
    echo ""
    echo "🚀 Para fazer upload:"
    echo "   cd .."
    echo "   ./flash.sh picotool"
else
    echo "❌ Erro: Arquivo UF2 não foi gerado"
    exit 1
fi