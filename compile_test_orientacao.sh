#!/bin/bash

echo "🎯 === COMPILANDO TESTE FINAL DE ORIENTAÇÃO ===";
echo "===============================================";

# Diretório de build para o teste
BUILD_DIR="build_test_orientacao"

# Limpa build anterior se existir
if [ -d "$BUILD_DIR" ]; then
    echo "🧹 Limpando build anterior...";
    rm -rf "$BUILD_DIR"
fi

# Cria diretório de build
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Copia CMakeLists.txt específico
cp ../CMakeLists_test_orientacao.txt ./CMakeLists.txt
cp ../pico_sdk_import.cmake ./

echo "🔨 Configurando CMake...";
cmake .. -DCMAKE_BUILD_TYPE=Debug

if [ $? -ne 0 ]; then
    echo "❌ Erro na configuração do CMake";
    exit 1
fi

echo "🔨 Compilando projeto...";
make -j$(nproc)

if [ $? -eq 0 ]; then
    echo "";
    echo "✅ COMPILAÇÃO CONCLUÍDA COM SUCESSO!";
    echo "📁 Arquivo gerado:";
    ls -la test_orientacao_final.uf2
    echo "";
    echo "🚀 COMO USAR:";
    echo "   1. Conecte o Pico segurando BOOTSEL";
    echo "   2. Copie test_orientacao_final.uf2 para RPI-RP2";
    echo "   3. Observe o display para verificar orientação";
    echo "   4. Use Serial Monitor (115200) para logs detalhados";
    echo "";
    echo "📊 O QUE VOCÊ DEVE VER NO DISPLAY:";
    echo "   ✓ Texto 'HYDROSENSE 2024' horizontal e legível";
    echo "   ✓ Seta apontando para cima";
    echo "   ✓ Bordas formando retângulo correto";
    echo "   ✓ Tela final: 'HYDROSENSE DISPLAY OK SISTEMA ATIVO'";
else
    echo "❌ Erro na compilação";
    exit 1
fi