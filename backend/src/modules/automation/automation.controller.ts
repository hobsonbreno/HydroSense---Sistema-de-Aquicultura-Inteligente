import { Controller, Post, Get, Body, HttpCode, HttpStatus } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse, ApiBody } from '@nestjs/swagger';
import { AutomationService } from './automation.service';

@ApiTags('Automation')
@Controller('automation')
export class AutomationController {
  constructor(private readonly automationService: AutomationService) {}

  @Get('status')
  @ApiOperation({ 
    summary: 'Status das automações',
    description: 'Retorna o status atual de todas as automações (temperatura, qualidade da água, nível)'
  })
  @ApiResponse({ status: 200, description: 'Status das automações recuperado com sucesso' })
  async getAutomationStatus() {
    return await this.automationService.getAutomationStatus();
  }

  @Post('temperature/toggle')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ 
    summary: 'Ativar/desativar controle de temperatura',
    description: 'Liga ou desliga o controle automático de temperatura (ventilador quando > 29°C)'
  })
  @ApiBody({
    description: 'Estado desejado para controle de temperatura',
    schema: {
      type: 'object',
      properties: {
        active: { type: 'boolean', example: true }
      }
    }
  })
  @ApiResponse({ status: 200, description: 'Controle de temperatura alterado com sucesso' })
  async toggleTemperatureControl(@Body('active') active: boolean) {
    return await this.automationService.setTemperatureControl(active);
  }

  @Post('water-quality/toggle')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ 
    summary: 'Ativar/desativar controle de qualidade da água',
    description: 'Liga ou desliga o controle automático de qualidade da água (troca quando não cristalina)'
  })
  @ApiBody({
    description: 'Estado desejado para controle de qualidade',
    schema: {
      type: 'object',
      properties: {
        active: { type: 'boolean', example: true }
      }
    }
  })
  @ApiResponse({ status: 200, description: 'Controle de qualidade da água alterado com sucesso' })
  async toggleWaterQualityControl(@Body('active') active: boolean) {
    return await this.automationService.setWaterQualityControl(active);
  }

  @Post('water-level/toggle')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ 
    summary: 'Ativar/desativar controle de nível da água',
    description: 'Liga ou desliga o controle automático de nível da água (completa se < 80%)'
  })
  @ApiBody({
    description: 'Estado desejado para controle de nível',
    schema: {
      type: 'object',
      properties: {
        active: { type: 'boolean', example: true }
      }
    }
  })
  @ApiResponse({ status: 200, description: 'Controle de nível da água alterado com sucesso' })
  async toggleWaterLevelControl(@Body('active') active: boolean) {
    return await this.automationService.setWaterLevelControl(active);
  }

  @Post('water-cycle/manual')
  @HttpCode(HttpStatus.OK)
  @ApiOperation({ 
    summary: 'Executar ciclo de troca de água manual',
    description: 'Executa manualmente o ciclo completo: esvaziar 25% → aguardar 30s → encher até 90%'
  })
  @ApiResponse({ status: 200, description: 'Ciclo manual iniciado com sucesso' })
  @ApiResponse({ status: 500, description: 'Erro ao executar ciclo manual' })
  async manualWaterCycle() {
    return await this.automationService.manualWaterCycle();
  }
}