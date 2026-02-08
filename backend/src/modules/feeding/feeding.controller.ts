import { Controller, Post, Get, Body, Query, HttpCode, HttpStatus } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse, ApiQuery, ApiBody } from '@nestjs/swagger';
import { FeedingService } from './feeding.service';

@ApiTags('Feeding')
@Controller('feeding')
export class FeedingController {
  constructor(private readonly feedingService: FeedingService) {}

  @Get('status')
  @ApiOperation({ 
    summary: 'Status do sistema de alimentação',
    description: 'Retorna status da alimentação automática, próxima alimentação agendada e alimentações do dia'
  })
  @ApiResponse({ status: 200, description: 'Status recuperado com sucesso' })
  async getFeedingStatus() {
    return await this.feedingService.getFeedingStatus();
  }

  @Get('history')
  @ApiOperation({ 
    summary: 'Histórico de alimentações',
    description: 'Retorna histórico paginado das alimentações com estatísticas'
  })
  @ApiQuery({ name: 'limit', required: false, type: Number, description: 'Limite de resultados (padrão: 50)' })
  @ApiQuery({ name: 'page', required: false, type: Number, description: 'Página dos resultados (padrão: 1)' })
  @ApiQuery({ name: 'startDate', required: false, type: String, description: 'Data de início (ISO string)' })
  @ApiQuery({ name: 'endDate', required: false, type: String, description: 'Data de fim (ISO string)' })
  @ApiResponse({ status: 200, description: 'Histórico recuperado com sucesso' })
  async getFeedingHistory(
    @Query('limit') limit: number = 50,
    @Query('page') page: number = 1,
    @Query('startDate') startDate?: string,
    @Query('endDate') endDate?: string
  ) {
    return await this.feedingService.getFeedingHistory(limit, page, startDate, endDate);
  }

  @Get('today')
  @ApiOperation({ 
    summary: 'Alimentações de hoje',
    description: 'Retorna todas as alimentações realizadas hoje'
  })
  @ApiResponse({ status: 200, description: 'Alimentações de hoje recuperadas com sucesso' })
  async getTodayFeedings() {
    return await this.feedingService.getTodayFeedings();
  }

  @Get('next')
  @ApiOperation({ 
    summary: 'Próxima alimentação agendada',
    description: 'Retorna informações sobre a próxima alimentação automática'
  })
  @ApiResponse({ status: 200, description: 'Informações da próxima alimentação recuperadas com sucesso' })
  async getNextScheduledFeeding() {
    return await this.feedingService.getNextScheduledFeeding();
  }

  @Post('manual')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ 
    summary: 'Executar alimentação manual',
    description: 'Executa uma alimentação manual imediatamente (servo 360° por 2 segundos)'
  })
  @ApiBody({
    description: 'Observação opcional para a alimentação manual',
    schema: {
      type: 'object',
      properties: {
        observacao: { type: 'string', example: 'Alimentação extra solicitada pelo usuário' }
      }
    },
    required: false
  })
  @ApiResponse({ status: 200, description: 'Alimentação manual executada com sucesso' })
  @ApiResponse({ status: 500, description: 'Erro ao executar alimentação manual' })
  async manualFeeding(@Body('observacao') observacao?: string) {
    return await this.feedingService.manualFeeding(observacao);
  }

  @Post('toggle')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ 
    summary: 'Ativar/desativar alimentação automática',
    description: 'Liga ou desliga o sistema de alimentação automática (08:00 e 16:00)'
  })
  @ApiBody({
    description: 'Estado desejado para alimentação automática',
    schema: {
      type: 'object',
      properties: {
        active: { type: 'boolean', example: true }
      }
    }
  })
  @ApiResponse({ status: 200, description: 'Status da alimentação automática alterado com sucesso' })
  async toggleAutomaticFeeding(@Body('active') active: boolean) {
    return await this.feedingService.setFeedingActive(active);
  }
}