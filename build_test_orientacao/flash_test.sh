#!/bin/bash

echo "🚀 === CARREGANDO TESTE DE ORIENTAÇÃO NO PICO ===";
echo "================================================";

# Verifica se o arquivo UF2 existe
if [ ! -f "test_orientacao_final.uf2" ]; then
    echo "❌ Arquivo test_orientacao_final.uf2 não encontrado!";
    echo "📍 Certifique-se de estar no diretório build_test_orientacao/";
    exit 1;
fi

echo "📱 INSTRUÇÕES PARA CARREGAR:";
echo "";
echo "1. 🔌 Desconecte o Raspberry Pi Pico W do USB";
echo "2. ⏸️  Segure o botão BOOTSEL no Pico W";
echo "3. 🔌 Conecte o Pico W ao USB (ainda segurando BOOTSEL)";
echo "4. ▶️  Solte o botão BOOTSEL";
echo "5. 📁 Uma unidade 'RPI-RP2' aparecerá no sistema";
echo "";

# Aguarda o usuário pressionar Enter
read -p "▶️  Pressione ENTER quando a unidade RPI-RP2 estiver visível..."

echo "";
echo "🔍 Procurando unidade RPI-RP2...";

# Tenta encontrar a unidade RPI-RP2
RPI_PATH=""
for mount_point in /media/$USER/RPI-RP2 /mnt/RPI-RP2 /run/media/$USER/RPI-RP2; do
    if [ -d "$mount_point" ]; then
        RPI_PATH="$mount_point"
        break
    fi
done

# Se não encontrou automaticamente, pergunta o caminho
if [ -z "$RPI_PATH" ]; then
    echo "❓ Não consegui encontrar a unidade RPI-RP2 automaticamente.";
    echo "🔍 Verifique onde ela foi montada com: lsblk | grep RPI-RP2";
    echo "";
    read -p "📍 Digite o caminho completo para a unidade RPI-RP2: " RPI_PATH
fi

# Verifica se o caminho existe
if [ ! -d "$RPI_PATH" ]; then
    echo "❌ Caminho não encontrado: $RPI_PATH";
    echo "💡 Dica: Execute 'lsblk' para ver as unidades montadas";
    exit 1;
fi

echo "✅ Unidade RPI-RP2 encontrada em: $RPI_PATH";
echo "";
echo "📋 Copiando test_orientacao_final.uf2...";

# Copia o arquivo
if cp test_orientacao_final.uf2 "$RPI_PATH/"; then
    echo "✅ Arquivo copiado com sucesso!";
    echo "⏱️  O Pico W será reiniciado automaticamente...";
    echo "";
    echo "🎯 OBSERVE O DISPLAY OLED AGORA!";
    echo "================================";
    echo "Você deve ver:";
    echo "  ✓ Texto 'HYDROSENSE 2024' horizontal (não deitado)";
    echo "  ✓ Seta apontando para CIMA ⬆️";  
    echo "  ✓ Bordas formando retângulo normal";
    echo "  ✓ Mensagem final: 'HYDROSENSE DISPLAY OK SISTEMA ATIVO'";
    echo "";
    echo "📊 RESULTADO:";
    echo "  🟢 Se o texto estiver HORIZONTAL = PROBLEMA RESOLVIDO!";
    echo "  🔴 Se ainda estiver 'deitado' = Precisa mais ajustes";
    echo "";
    echo "📱 Para logs detalhados, abra um monitor serial (115200 baud)";
else
    echo "❌ Erro ao copiar arquivo!";
    echo "🔧 Verifique se tem permissão de escrita na unidade";
    exit 1;
fi