#!/bin/bash

echo "==================================="
echo "    HydroSense v10 - Start System"
echo "==================================="

# Verificar se Docker está rodando
if ! docker info > /dev/null 2>&1; then
    echo "❌ Docker não está rodando. Por favor, inicie o Docker primeiro."
    exit 1
fi

# Verificar se Docker Compose está disponível
if ! command -v docker-compose &> /dev/null; then
    echo "❌ Docker Compose não encontrado. Instalando..."
    # Tentar instalar docker-compose
    pip3 install docker-compose 2>/dev/null || {
        echo "❌ Falha ao instalar Docker Compose. Instale manualmente:"
        echo "   pip3 install docker-compose"
        echo "   ou"
        echo "   sudo apt install docker-compose"
        exit 1
    }
fi

echo "🚀 Iniciando sistema HydroSense..."
echo

# Navegar para o diretório do projeto
cd "$(dirname "$0")"

echo "📂 Diretório atual: $(pwd)"
echo

# Parar containers existentes (se houver)
echo "🛑 Parando containers existentes..."
docker-compose down 2>/dev/null || true
echo

# Remover volumes órfãos
echo "🧹 Limpando volumes órfãos..."
docker volume prune -f 2>/dev/null || true
echo

# Construir e iniciar os serviços
echo "🔨 Construindo e iniciando serviços..."
docker-compose up -d --build

# Aguardar alguns segundos para os serviços iniciarem
echo "⏳ Aguardando serviços iniciarem..."
sleep 10

# Verificar status dos containers
echo "📋 Status dos containers:"
docker-compose ps

echo
echo "✅ Sistema HydroSense iniciado!"
echo
echo "🌐 Serviços disponíveis:"
echo "   • Frontend:        http://localhost:3001"
echo "   • Backend API:     http://localhost:3000"
echo "   • Swagger API:     http://localhost:3000/api"
echo "   • MongoDB:         mongodb://localhost:27017/hydrosense"
echo "   • Pico W Device:   http://10.0.0.181"
echo
echo "📱 Acesse a interface web em: http://localhost:3001"
echo "📚 Documentação da API em: http://localhost:3000/api"
echo
echo "🔧 Comandos úteis:"
echo "   • Ver logs:        docker-compose logs -f"
echo "   • Parar sistema:   docker-compose down"
echo "   • Reiniciar:       docker-compose restart"
echo
echo "📊 Para monitorar os logs em tempo real, execute:"
echo "   docker-compose logs -f"
echo

# Verificar se o Pico W está acessível
echo "🔍 Verificando conectividade com Pico W..."
if ping -c 1 10.0.0.181 &> /dev/null; then
    echo "✅ Pico W (10.0.0.181) está acessível!"
else
    echo "⚠️  Pico W (10.0.0.181) não está acessível."
    echo "   Verifique se o dispositivo está ligado e conectado à rede HydroSense."
fi

echo
echo "🎉 Sistema pronto para uso!"
echo "   Abra http://localhost:3001 no seu navegador para acessar a interface."