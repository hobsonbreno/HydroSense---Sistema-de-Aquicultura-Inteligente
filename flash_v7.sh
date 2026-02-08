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
