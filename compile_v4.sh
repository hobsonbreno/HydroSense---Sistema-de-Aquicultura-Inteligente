#!/bin/bash
PROJECT_DIR="/home/hobson007breno/Downloads/projeto final/HydroSense"

rm -rf "$PROJECT_DIR/build_v4"
mkdir "$PROJECT_DIR/build_v4"
cd "$PROJECT_DIR/build_v4"

# Copia arquivos necessários
cp "$PROJECT_DIR/CMakeLists_v4.txt" "$PROJECT_DIR/build_v4/CMakeLists.txt"
cp "$PROJECT_DIR/pico_sdk_import.cmake" "$PROJECT_DIR/build_v4/"
mkdir -p src
cp "$PROJECT_DIR/src/hydrosense_v4.c" "$PROJECT_DIR/build_v4/src/"

# Compila
cmake -DCMAKE_BUILD_TYPE=Release .
make -j4

echo ""
if [ -f hydrosense_v4.uf2 ]; then
    echo "✅ Compilado: build_v4/hydrosense_v4.uf2"
else
    echo "❌ Erro na compilação"
fi
