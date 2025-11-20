#!/bin/bash

# Script de Compilação do HydroSense
# Uso: ./compile.sh [clean|help]

echo "🐟 Compilando HydroSense - Sistema de Aquicultura Inteligente"
echo "=============================================================="

PROJECT_DIR="/home/hobson007breno/Downloads/projeto final/HydroSense"
BUILD_DIR="$PROJECT_DIR/build"

# Função de ajuda
show_help() {
    echo "Uso: $0 [opção]"
    echo ""
    echo "Opções:"
    echo "  clean    Limpa build anterior e recompila do zero"
    echo "  help     Mostra esta mensagem de ajuda"
    echo "  (sem parâmetro) Compilação incremental rápida"
    echo ""
    echo "Exemplos:"
    echo "  ./compile.sh         # Compilação rápida"
    echo "  ./compile.sh clean   # Compilação completa"
}

# Verifica argumentos
if [ "$1" = "help" ]; then
    show_help
    exit 0
fi

# Verifica se o diretório do projeto existe
if [ ! -d "$PROJECT_DIR" ]; then
    echo "❌ Erro: Diretório do projeto não encontrado!"
    echo "   Esperado: $PROJECT_DIR"
    exit 1
fi

cd "$PROJECT_DIR"

# Limpa build anterior se solicitado
if [ "$1" = "clean" ]; then
    echo "🧹 Limpando build anterior..."
    rm -rf "$BUILD_DIR"/*
    echo "✅ Build limpo!"
fi

# Cria diretório build se não existir
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# Configura CMake se necessário
if [ ! -f "Makefile" ] || [ "$1" = "clean" ]; then
    echo "⚙️ Configurando CMake..."
    PICO_BOARD=pico_w cmake .. || {
        echo "❌ Erro na configuração do CMake!"
        echo "   Verifique se o Pico SDK está configurado corretamente"
        exit 1
    }
fi

echo "🔨 Compilando projeto..."
start_time=$(date +%s)

make -j4 || {
    echo "❌ Erro na compilação!"
    echo "   Verifique os logs acima para detalhes"
    exit 1
}

end_time=$(date +%s)
duration=$((end_time - start_time))

echo ""
echo "🎉 COMPILAÇÃO CONCLUÍDA COM SUCESSO!"
echo "⏱️ Tempo de compilação: ${duration}s"
echo ""
echo "📁 Arquivos gerados:"
if [ -f "HydroSense.uf2" ]; then
    ls -lh HydroSense.uf2 HydroSense.elf HydroSense.bin 2>/dev/null
else
    echo "❌ Arquivos de saída não encontrados!"
    exit 1
fi

echo ""
echo "🚀 COMO USAR:"
echo "   1. Conecte o Raspberry Pi Pico W via USB segurando BOOTSEL"
echo "   2. Copie HydroSense.uf2 para a unidade RPI-RP2 que aparecer"
echo "   3. O sistema será carregado automaticamente"
echo ""
echo "📊 MONITORAMENTO:"
echo "   - Conecte via Serial Monitor (115200 baud) para ver os logs"
echo "   - O sistema iniciará automaticamente após o boot"
echo ""
echo "✅ Pronto para uso no seu sistema de aquicultura!"