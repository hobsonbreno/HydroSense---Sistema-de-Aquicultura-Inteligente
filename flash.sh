#!/bin/bash

# Script para fazer flash do firmware HydroSense no Raspberry Pi Pico W
# Uso: ./flash.sh [metodo]
# Métodos disponíveis: picotool, usb, auto

set -e

PROJECT_NAME="HydroSense"
BUILD_DIR="build"
UF2_FILE="$BUILD_DIR/${PROJECT_NAME}.uf2"
ELF_FILE="$BUILD_DIR/${PROJECT_NAME}.elf"

# Cores para output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

echo -e "${BLUE}🚀 HydroSense Flash Tool${NC}"
echo "================================="

# Verifica se o arquivo UF2 existe
if [ ! -f "$UF2_FILE" ]; then
    echo -e "${RED}❌ Erro: Arquivo $UF2_FILE não encontrado!${NC}"
    echo -e "${YELLOW}💡 Execute 'make' no diretório build primeiro.${NC}"
    exit 1
fi

echo -e "${GREEN}✅ Arquivo encontrado: $UF2_FILE${NC}"

# Função para upload via USB (método padrão)
flash_usb() {
    echo -e "${BLUE}📱 Método: Upload via USB${NC}"
    echo ""
    echo -e "${YELLOW}📋 Instruções:${NC}"
    echo "1. Segure o botão BOOTSEL no Pico W"
    echo "2. Conecte o cabo USB (mantenha BOOTSEL pressionado)"
    echo "3. Solte o botão BOOTSEL"
    echo "4. O Pico W aparecerá como drive USB (RPI-RP2)"
    echo ""
    read -p "Pressione ENTER quando o Pico W estiver em modo bootloader..."
    
    # Procura pelo drive do Pico W
    MOUNT_POINTS=$(find /media -name "RPI-RP2" -type d 2>/dev/null || true)
    
    if [ -z "$MOUNT_POINTS" ]; then
        echo -e "${RED}❌ Drive RPI-RP2 não encontrado!${NC}"
        echo -e "${YELLOW}💡 Verifique se o Pico W está em modo bootloader.${NC}"
        exit 1
    fi
    
    MOUNT_POINT=$(echo "$MOUNT_POINTS" | head -n1)
    echo -e "${GREEN}📁 Drive encontrado: $MOUNT_POINT${NC}"
    
    echo -e "${BLUE}📤 Copiando firmware...${NC}"
    cp "$UF2_FILE" "$MOUNT_POINT/"
    
    echo -e "${GREEN}✅ Upload concluído!${NC}"
    echo -e "${BLUE}🔄 O Pico W reiniciará automaticamente.${NC}"
}

# Função para upload via picotool
flash_picotool() {
    echo -e "${BLUE}🔧 Método: picotool${NC}"
    
    # Verifica se picotool está instalado
    if ! command -v picotool &> /dev/null; then
        echo -e "${RED}❌ picotool não encontrado!${NC}"
        echo -e "${YELLOW}💡 Instale com: sudo apt install picotool${NC}"
        exit 1
    fi
    
    echo -e "${BLUE}🔍 Procurando dispositivos...${NC}"
    
    # Verifica se o Pico W está conectado
    if ! picotool info -a &> /dev/null; then
        echo -e "${RED}❌ Pico W não encontrado!${NC}"
        echo -e "${YELLOW}💡 Conecte o Pico W em modo bootloader (BOOTSEL).${NC}"
        exit 1
    fi
    
    echo -e "${GREEN}✅ Pico W detectado!${NC}"
    
    echo -e "${BLUE}📤 Fazendo upload via picotool...${NC}"
    picotool load "$UF2_FILE"
    
    echo -e "${BLUE}🔄 Reiniciando dispositivo...${NC}"
    picotool reboot
    
    echo -e "${GREEN}✅ Upload via picotool concluído!${NC}"
}

# Função para detecção automática
flash_auto() {
    echo -e "${BLUE}🤖 Método: Detecção automática${NC}"
    
    # Tenta picotool primeiro
    if command -v picotool &> /dev/null && picotool info -a &> /dev/null; then
        echo -e "${GREEN}🔍 Pico W detectado via picotool${NC}"
        flash_picotool
    else
        echo -e "${YELLOW}⚠️  picotool não disponível, usando método USB${NC}"
        flash_usb
    fi
}

# Função para mostrar informações do firmware
show_info() {
    echo -e "${BLUE}ℹ️  Informações do Firmware${NC}"
    echo "================================="
    echo "Projeto: $PROJECT_NAME"
    echo "Arquivo: $UF2_FILE"
    
    if [ -f "$ELF_FILE" ]; then
        SIZE=$(stat -f%z "$ELF_FILE" 2>/dev/null || stat -c%s "$ELF_FILE" 2>/dev/null || echo "unknown")
        echo "Tamanho: $SIZE bytes"
    fi
    
    echo "Data: $(date '+%Y-%m-%d %H:%M:%S')"
    echo ""
}

# Parse dos argumentos
METHOD="auto"
if [ $# -gt 0 ]; then
    METHOD="$1"
fi

case "$METHOD" in
    "picotool")
        show_info
        flash_picotool
        ;;
    "usb")
        show_info
        flash_usb
        ;;
    "auto")
        show_info
        flash_auto
        ;;
    "info")
        show_info
        ;;
    "help"|"-h"|"--help")
        echo "Uso: $0 [método]"
        echo ""
        echo "Métodos disponíveis:"
        echo "  auto      - Detecção automática (padrão)"
        echo "  picotool  - Upload via picotool"
        echo "  usb       - Upload manual via USB"
        echo "  info      - Mostrar informações do firmware"
        echo "  help      - Mostrar esta ajuda"
        echo ""
        echo "Exemplos:"
        echo "  $0              # Detecção automática"
        echo "  $0 picotool     # Usar picotool"
        echo "  $0 usb          # Método USB manual"
        ;;
    *)
        echo -e "${RED}❌ Método desconhecido: $METHOD${NC}"
        echo -e "${YELLOW}💡 Use '$0 help' para ver os métodos disponíveis.${NC}"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}🎉 Flash concluído com sucesso!${NC}"
echo -e "${BLUE}📟 Verifique a saída serial para logs do HydroSense.${NC}"