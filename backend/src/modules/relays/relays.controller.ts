import { Controller, Post, Get, Body, Query, HttpCode, HttpStatus } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse, ApiQuery } from '@nestjs/swagger';
import { RelaysService } from './relays.service';
import { CreateRelayCommandDto } from './dto/create-relay-command.dto';

@ApiTags('Relays')
@Controller('relays')
export class RelaysController {
  constructor(private readonly relaysService: RelaysService) {}

  @Post('control')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ 
    summary: 'Controlar relé específico',
    description: `
    Controla os relés do sistema HydroSense:
    - LN1: Motor/Ventilador (GPIO 14) - Controle de temperatura
    - LN2: Bomba 01 (GPIO 15) - Esvaziar aquário
    - LN3: Bomba 02 (GPIO 16) - Encher aquário
    `
  })
  @ApiResponse({ status: 200, description: 'Comando enviado com sucesso' })
  @ApiResponse({ status: 400, description: 'Parâmetros inválidos' })
  @ApiResponse({ status: 500, description: 'Erro interno do servidor' })
  async controlRelay(@Body() createRelayCommand: CreateRelayCommandDto) {
    return await this.relaysService.controlRelay(createRelayCommand);
  }

  @Get('history')
  @ApiOperation({ 
    summary: 'Histórico de comandos dos relés',
    description: 'Retorna histórico paginado dos comandos enviados aos relés'
  })
  @ApiQuery({ name: 'tipo', required: false, enum: ['LN1', 'LN2', 'LN3'], description: 'Filtrar por tipo de relé' })
  @ApiQuery({ name: 'limit', required: false, type: Number, description: 'Limite de resultados (padrão: 50)' })
  @ApiQuery({ name: 'page', required: false, type: Number, description: 'Página dos resultados (padrão: 1)' })
  @ApiResponse({ status: 200, description: 'Histórico recuperado com sucesso' })
  async getHistory(
    @Query('tipo') tipo?: string,
    @Query('limit') limit: number = 50,
    @Query('page') page: number = 1
  ) {
    return await this.relaysService.getRelayHistory(tipo, limit, page);
  }

  @Get('status')
  @ApiOperation({ 
    summary: 'Status atual dos relés',
    description: 'Retorna o estado atual de todos os relés e conexão com Pico W'
  })
  @ApiResponse({ status: 200, description: 'Status recuperado com sucesso' })
  async getStatus() {
    return await this.relaysService.getRelayStatus();
  }

  @Post('ventilator/on')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ 
    summary: 'Ligar ventilador (LN1)',
    description: 'Atalho para ligar o ventilador/motor de refrigeração'
  })
  async turnVentilatorOn() {
    return await this.relaysService.controlVentilator(true, 'Comando manual via API');
  }

  @Post('ventilator/off')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ 
    summary: 'Desligar ventilador (LN1)',
    description: 'Atalho para desligar o ventilador/motor de refrigeração'
  })
  async turnVentilatorOff() {
    return await this.relaysService.controlVentilator(false, 'Comando manual via API');
  }

  @Post('drain/start')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ 
    summary: 'Iniciar bomba de esvaziar (LN2)',
    description: 'Atalho para ligar bomba que esvazia o aquário'
  })
  async startDrainPump() {
    return await this.relaysService.controlDrainPump(true, 'Comando manual via API');
  }

  @Post('drain/stop')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ 
    summary: 'Parar bomba de esvaziar (LN2)',
    description: 'Atalho para desligar bomba que esvazia o aquário'
  })
  async stopDrainPump() {
    return await this.relaysService.controlDrainPump(false, 'Comando manual via API');
  }

  @Post('fill/start')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ 
    summary: 'Iniciar bomba de encher (LN3)',
    description: 'Atalho para ligar bomba que enche o aquário'
  })
  async startFillPump() {
    return await this.relaysService.controlFillPump(true, 'Comando manual via API');
  }

  @Post('fill/stop')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ 
    summary: 'Parar bomba de encher (LN3)',
    description: 'Atalho para desligar bomba que enche o aquário'
  })
  async stopFillPump() {
    return await this.relaysService.controlFillPump(false, 'Comando manual via API');
  }
}