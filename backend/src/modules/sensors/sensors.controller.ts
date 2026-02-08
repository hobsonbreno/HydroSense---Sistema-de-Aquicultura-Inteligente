import { Controller, Get, Post, Body, Query, Param } from '@nestjs/common';
import { ApiTags, ApiOperation, ApiResponse, ApiQuery } from '@nestjs/swagger';
import { SensorsService } from './sensors.service';
import { CreateSensorDataDto } from './dto/create-sensor-data.dto';
import { SensorData } from '../../schemas/sensor-data.schema';

@ApiTags('sensors')
@Controller('sensors')
export class SensorsController {
  constructor(private readonly sensorsService: SensorsService) {}

  @Post('data')
  @ApiOperation({ summary: 'Receber dados dos sensores do Pico W' })
  @ApiResponse({ status: 201, description: 'Dados salvos com sucesso', type: SensorData })
  async create(@Body() createSensorDataDto: CreateSensorDataDto) {
    return this.sensorsService.create(createSensorDataDto);
  }

  @Get('data')
  @ApiOperation({ summary: 'Buscar dados dos sensores' })
  @ApiQuery({ name: 'limit', required: false, description: 'Limite de registros' })
  @ApiResponse({ status: 200, description: 'Lista de dados dos sensores', type: [SensorData] })
  async findAll(@Query('limit') limit?: string) {
    const limitNum = limit ? parseInt(limit) : 100;
    return this.sensorsService.findAll(limitNum);
  }

  @Get('data/latest')
  @ApiOperation({ summary: 'Buscar dados mais recentes' })
  @ApiResponse({ status: 200, description: 'Dados mais recentes', type: SensorData })
  async findLatest() {
    return this.sensorsService.findLatest();
  }

  @Get('temperature/stats')
  @ApiOperation({ summary: 'Estatísticas de temperatura' })
  @ApiQuery({ name: 'hours', required: false, description: 'Período em horas (padrão: 24h)' })
  async getTemperatureStats(@Query('hours') hours?: string) {
    const hoursNum = hours ? parseInt(hours) : 24;
    return this.sensorsService.getTemperatureStats(hoursNum);
  }

  @Get('water/level-history')
  @ApiOperation({ summary: 'Histórico do nível da água' })
  @ApiQuery({ name: 'hours', required: false, description: 'Período em horas (padrão: 24h)' })
  async getWaterLevelHistory(@Query('hours') hours?: string) {
    const hoursNum = hours ? parseInt(hours) : 24;
    return this.sensorsService.getWaterLevelHistory(hoursNum);
  }

  @Get('water/quality')
  @ApiOperation({ summary: 'Verificar qualidade da água (sensor de cor)' })
  async checkWaterQuality() {
    return this.sensorsService.checkWaterQuality();
  }

  @Get('data/range/:start/:end')
  @ApiOperation({ summary: 'Buscar dados por período' })
  async findByTimeRange(
    @Param('start') start: string,
    @Param('end') end: string,
  ) {
    const startDate = new Date(start);
    const endDate = new Date(end);
    return this.sensorsService.findByTimeRange(startDate, endDate);
  }
}