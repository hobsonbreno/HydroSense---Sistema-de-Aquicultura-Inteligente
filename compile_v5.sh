#!/bin/bash
PROJECT_DIR="/home/hobson007breno/Downloads/projeto final/HydroSense"

rm -rf "$PROJECT_DIR/build_v5"
mkdir "$PROJECT_DIR/build_v5"
cd "$PROJECT_DIR/build_v5"

# Copia arquivos necessários
cp "$PROJECT_DIR/CMakeLists_v5.txt" "$PROJECT_DIR/build_v5/CMakeLists.txt"
cp "$PROJECT_DIR/pico_sdk_import.cmake" "$PROJECT_DIR/build_v5/"
cp "$PROJECT_DIR/lwipopts.h" "$PROJECT_DIR/build_v5/"
mkdir -p src
cp "$PROJECT_DIR/src/hydrosense_v5.c" "$PROJECT_DIR/build_v5/src/"

# Compila
cmake -DCMAKE_BUILD_TYPE=Release .
make -j4

echo ""
if [ -f hydrosense_v5.uf2 ]; then
    echo "✅ Compilado: build_v5/hydrosense_v5.uf2"
    echo ""
    echo "⚠️  IMPORTANTE: Edite src/hydrosense_v5.c e configure:"
    echo "    - WIFI_SSID     = Nome da sua rede WiFi"
    echo "    - WIFI_PASSWORD = Senha da sua rede WiFi"
else
    echo "❌ Erro na compilação"
fi
