#!/bin/bash

# Script de Gravação do HydroSense v2.1
echo "🐟 HydroSense v2.1 - Script de Gravação Automática"
echo "=================================================="

PROJECT_DIR="/home/hobson007breno/Downloads/projeto final/HydroSense"
BUILD_DIR="$PROJECT_DIR/build"
UF2_FILE="$BUILD_DIR/HydroSense.uf2"

# Função para mostrar ajuda
show_help() {
    echo "USO: $0 [opção]"
    echo ""
    echo "OPÇÕES:"
    echo "  auto       Gravação automática via montagem USB (padrão)"
    echo "  picotool   Gravação via picotool (mais rápido)"
    echo "  force      Força regravação mesmo se já conectado"
    echo "  monitor    Grava e abre monitor serial"
    echo "  help       Mostra esta ajuda"
    echo ""
    echo "EXEMPLOS:"
    echo "  $0            # Gravação automática"
    echo "  $0 picotool   # Usa picotool"
    echo "  $0 monitor    # Grava + monitor serial"
}

# Verifica argumentos
METHOD="auto"
OPEN_MONITOR=false

case "${1:-auto}" in
    "help"|"-h"|"--help")
        show_help
        exit 0
        ;;
    "picotool")
        METHOD="picotool"
        ;;
    "force")
        METHOD="force"
        ;;
    "monitor")
        METHOD="auto"
        OPEN_MONITOR=true
        ;;
    "auto"|"")
        METHOD="auto"
        ;;
    *)
        echo "❌ Opção inválida: $1"
        show_help
        exit 1
        ;;
esac

# Verifica se o arquivo .uf2 existe
if [ ! -f "$UF2_FILE" ]; then
    echo "❌ Arquivo HydroSense.uf2 não encontrado!"
    echo "   Compilando automaticamente..."
    echo ""
    
    cd "$PROJECT_DIR"
    if ./compile.sh; then
        echo "✅ Compilação concluída!"
    else
        echo "❌ Erro na compilação!"
        exit 1
    fi
fi

echo "📁 Arquivo: $UF2_FILE"
echo "📊 Tamanho: $(ls -lh "$UF2_FILE" | awk '{print $5}')"
echo "🔧 Método: $METHOD"
echo ""

# Método via picotool (mais rápido e confiável)
if [ "$METHOD" = "picotool" ]; then
    echo "🛠️  Usando picotool para gravação..."
    echo ""
    echo "🔌 INSTRUÇÕES:"
    echo "1. Segure BOOTSEL e conecte o Pico W via USB"
    echo "2. Solte BOOTSEL e pressione ENTER"
    echo ""
    read -p "Pressione ENTER quando o Pico estiver conectado em modo bootloader..."
    
    # Verifica se picotool está disponível
    if command -v picotool >/dev/null 2>&1; then
        echo "📤 Gravando com picotool..."
        if picotool load "$UF2_FILE" -fx; then
            echo "🎉 GRAVAÇÃO CONCLUÍDA COM PICOTOOL!"
        else
            echo "❌ Erro ao gravar com picotool"
            exit 1
        fi
    else
        echo "❌ picotool não encontrado! Instalando..."
        echo "   sudo apt install picotool"
        echo "   Ou baixe de: https://github.com/raspberrypi/picotool"
        exit 1
    fi
else
    # Método tradicional via montagem USB
    echo "🔌 INSTRUÇÕES:"
    echo "1. Desligue o Raspberry Pi Pico W"
    echo "2. Segure o botão BOOTSEL na placa"
    echo "3. Conecte via USB mantendo BOOTSEL pressionado"
    echo "4. Solte o BOOTSEL após conectar"
    echo ""
    echo "🔍 Aguardando a unidade RPI-RP2 aparecer..."

    # Loop para aguardar a unidade aparecer
    TIMEOUT=45
    COUNT=0
    while [ $COUNT -lt $TIMEOUT ]; do
        # Verifica possíveis locais onde a unidade pode aparecer
        RPI_PATH=""
        
        # Busca mais abrangente
        for path in "/media/$USER/RPI-RP2" "/media/RPI-RP2" "/mnt/RPI-RP2" "/run/media/$USER/RPI-RP2"; do
            if [ -d "$path" ]; then
                RPI_PATH="$path"
                break
            fi
        done
        
        # Busca dinâmica em /media e /mnt
        if [ -z "$RPI_PATH" ]; then
            RPI_PATH=$(find /media /mnt -name "RPI-RP2" -type d 2>/dev/null | head -1)
        fi
        
        if [ -n "$RPI_PATH" ]; then
            echo "✅ Unidade RPI-RP2 encontrada em: $RPI_PATH"
            echo ""
            echo "📤 Copiando firmware HydroSense..."
            
            if cp "$UF2_FILE" "$RPI_PATH/"; then
                echo "🎉 GRAVAÇÃO CONCLUÍDA COM SUCESSO!"
                break
            else
                echo "❌ Erro ao copiar arquivo para a placa!"
                exit 1
            fi
        fi
        
        if [ $((COUNT % 5)) -eq 0 ]; then
            echo "   Aguardando... (${COUNT}s/${TIMEOUT}s)"
        fi
        sleep 1
        COUNT=$((COUNT + 1))
    done

    if [ $COUNT -ge $TIMEOUT ]; then
        echo ""
        echo "⏰ Timeout: Unidade RPI-RP2 não foi encontrada em ${TIMEOUT}s"
        echo ""
        echo "🔧 SOLUÇÕES:"
        echo "1. Tente o método picotool: $0 picotool"
        echo "2. Verifique se a placa está em modo bootloader"
        echo "3. Use gravação manual via gerenciador de arquivos"
        exit 1
    fi
fi

echo ""
echo "🐟 HydroSense v2.1 gravado com sucesso!"
echo ""
echo "🔌 CONEXÕES IMPORTANTES:"
echo "   Servo SG90:"
echo "   ├── VCC (Vermelho) → 5V externo (importante!)"
echo "   ├── GND (Marrom/Preto) → GND comum"
echo "   └── Signal (Laranja) → GPIO 16"
echo ""
echo "   Display OLED I2C:"
echo "   ├── VCC → 3.3V"
echo "   ├── GND → GND"
echo "   ├── SDA → GPIO 8"
echo "   └── SCL → GPIO 9"
echo ""
echo "   Botões:"
echo "   ├── Botão A → GPIO 5 (pull-up interno)"
echo "   └── Botão B → GPIO 6 (pull-up interno)"
echo ""
echo "🔄 A placa reiniciará automaticamente em ~3 segundos"
echo "📊 Para monitorar: Serial Monitor 115200 baud"

# Abre monitor serial se solicitado
if [ "$OPEN_MONITOR" = true ]; then
    echo ""
    echo "🖥️  Aguardando reinicialização para abrir monitor serial..."
    sleep 5
    
    # Procura por dispositivos serial
    SERIAL_PORT=""
    for port in /dev/ttyACM* /dev/ttyUSB*; do
        if [ -e "$port" ]; then
            SERIAL_PORT="$port"
            break
        fi
    done
    
    if [ -n "$SERIAL_PORT" ]; then
        echo "🔗 Conectando ao monitor serial: $SERIAL_PORT"
        if command -v minicom >/dev/null 2>&1; then
            minicom -D "$SERIAL_PORT" -b 115200
        elif command -v screen >/dev/null 2>&1; then
            screen "$SERIAL_PORT" 115200
        else
            echo "📝 Instale minicom ou screen para monitor serial:"
            echo "   sudo apt install minicom screen"
        fi
    else
        echo "⚠️  Porta serial não encontrada. Reconecte a placa sem BOOTSEL."
    fi
fi

echo ""
echo "✅ HydroSense operacional! Sistema de aquicultura ativo."