import { Injectable, Logger } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Cron, CronExpression } from '@nestjs/schedule';
import { Model } from 'mongoose';
import { SensorData, SensorDataDocument } from '../../schemas/sensor-data.schema';
import { RelayControl, RelayControlDocument } from '../../schemas/relay-control.schema';
import { RelaysService } from '../relays/relays.service';

@Injectable()
export class AutomationService {
  private readonly logger = new Logger(AutomationService.name);
  private lastWaterQualityCheck: Date = new Date();
  private temperatureControlActive = true;
  private waterQualityControlActive = true;
  private waterLevelControlActive = true;

  constructor(
    @InjectModel(SensorData.name) private sensorDataModel: Model<SensorDataDocument>,
    @InjectModel(RelayControl.name) private relayControlModel: Model<RelayControlDocument>,
    private relaysService: RelaysService
  ) {}

  // Controle de temperatura - verifica a cada minuto
  @Cron('0 * * * * *') // A cada minuto
  async temperatureControl() {
    if (!this.temperatureControlActive) return;

    try {
      // Buscar últimos dados de temperatura
      const lastSensorData = await this.sensorDataModel
        .findOne()
        .sort({ timestamp: -1 })
        .exec();

      if (!lastSensorData) {
        this.logger.warn('Nenhum dado de sensor encontrado para controle de temperatura');
        return;
      }

      const currentTemp = lastSensorData.temperatura;
      const TEMP_THRESHOLD = 29; // °C

      // Buscar último comando do ventilador
      const lastVentilatorCommand = await this.relayControlModel
        .findOne({ tipo: 'LN1' })
        .sort({ timestamp: -1 })
        .exec();

      const ventilatorOn = lastVentilatorCommand?.estado || false;

      if (currentTemp > TEMP_THRESHOLD && !ventilatorOn) {
        await this.relaysService.controlVentilator(
          true,
          `Temperatura alta detectada: ${currentTemp}°C (limite: ${TEMP_THRESHOLD}°C)`
        );
        this.logger.log(`Ventilador LIGADO - Temperatura: ${currentTemp}°C`);
      } else if (currentTemp <= TEMP_THRESHOLD && ventilatorOn) {
        await this.relaysService.controlVentilator(
          false,
          `Temperatura normalizada: ${currentTemp}°C (limite: ${TEMP_THRESHOLD}°C)`
        );
        this.logger.log(`Ventilador DESLIGADO - Temperatura: ${currentTemp}°C`);
      }
    } catch (error) {
      this.logger.error('Erro no controle automático de temperatura:', error);
    }
  }

  // Controle de qualidade da água - verifica a cada 5 minutos
  @Cron('0 */5 * * * *') // A cada 5 minutos
  async waterQualityControl() {
    if (!this.waterQualityControlActive) return;

    try {
      // Buscar últimos dados de qualidade da água
      const lastSensorData = await this.sensorDataModel
        .findOne()
        .sort({ timestamp: -1 })
        .exec();

      if (!lastSensorData) {
        this.logger.warn('Nenhum dado de sensor encontrado para controle de qualidade');
        return;
      }

      // Verificar se água não está cristalina
      if (lastSensorData.corAgua !== 'cristalino') {
        this.logger.log(`Água não cristalina detectada: ${lastSensorData.corAgua}`);
        await this.performWaterCycle(lastSensorData.corAgua);
      }
    } catch (error) {
      this.logger.error('Erro no controle automático de qualidade da água:', error);
    }
  }

  // Controle de nível da água - verifica a cada 15 minutos
  @Cron('0 */15 * * * *') // A cada 15 minutos
  async waterLevelControl() {
    if (!this.waterLevelControlActive) return;

    try {
      // Buscar últimos dados de nível
      const lastSensorData = await this.sensorDataModel
        .findOne()
        .sort({ timestamp: -1 })
        .exec();

      if (!lastSensorData) {
        this.logger.warn('Nenhum dado de sensor encontrado para controle de nível');
        return;
      }

      const currentLevel = lastSensorData.nivel;
      const MIN_LEVEL = 80; // % - Se abaixo de 80%, completar

      if (currentLevel < MIN_LEVEL) {
        this.logger.log(`Nível baixo detectado: ${currentLevel}% (mínimo: ${MIN_LEVEL}%)`);
        
        // Completar até 90%
        await this.relaysService.controlFillPump(
          true,
          `Completando nível de água: ${currentLevel}% → 90%`
        );

        // Esperar tempo estimado para chegar a 90% (simulação)
        const timeToFill = this.calculateFillTime(currentLevel, 90);
        
        setTimeout(async () => {
          await this.relaysService.controlFillPump(
            false,
            'Nível de água completado para 90%'
          );
          this.logger.log('Bomba de enchimento DESLIGADA - Nível normalizado');
        }, timeToFill);
      }
    } catch (error) {
      this.logger.error('Erro no controle automático de nível da água:', error);
    }
  }

  // Ciclo completo de troca de água
  private async performWaterCycle(waterColor: string): Promise<void> {
    try {
      this.logger.log(`Iniciando ciclo de troca de água - Cor detectada: ${waterColor}`);

      // Fase 1: Esvaziar 25% (5L de 20L)
      await this.relaysService.controlDrainPump(
        true,
        `Esvaziando 25% do aquário - Água ${waterColor} detectada`
      );

      // Calcular tempo para esvaziar 25% (assumindo vazão de bomba)
      const drainTime = this.calculateDrainTime(25); // 25%
      
      setTimeout(async () => {
        await this.relaysService.controlDrainPump(
          false,
          'Esvaziamento de 25% concluído'
        );
        
        this.logger.log('Bomba de esvaziamento DESLIGADA - 25% removido');

        // Aguardar 30 segundos entre bombas
        setTimeout(async () => {
          // Fase 2: Encher até 90% (18L)
          await this.relaysService.controlFillPump(
            true,
            'Enchendo aquário até 90% após troca de água'
          );

          const fillTime = this.calculateFillTime(75, 90); // De 75% para 90%

          setTimeout(async () => {
            await this.relaysService.controlFillPump(
              false,
              'Aquário preenchido até 90% - Ciclo de troca concluído'
            );
            
            this.logger.log('Ciclo de troca de água CONCLUÍDO');
            this.lastWaterQualityCheck = new Date();
          }, fillTime);

        }, 30000); // 30 segundos entre bombas

      }, drainTime);

    } catch (error) {
      this.logger.error('Erro durante ciclo de troca de água:', error);
    }
  }

  // Cálculos de tempo estimado (baseados na capacidade e vazão das bombas)
  private calculateDrainTime(percentage: number): number {
    // Assumindo vazão de esvaziamento de 1L/min
    const volumeToRemove = (20 * percentage) / 100; // 20L é capacidade total
    return volumeToRemove * 60 * 1000; // minutos convertidos para millisegundos
  }

  private calculateFillTime(fromPercentage: number, toPercentage: number): number {
    // Assumindo vazão de enchimento de 1.5L/min
    const volumeToAdd = (20 * (toPercentage - fromPercentage)) / 100;
    return (volumeToAdd / 1.5) * 60 * 1000; // minutos convertidos para millisegundos
  }

  // Métodos para controle manual das automações
  async setTemperatureControl(active: boolean): Promise<{ success: boolean; message: string }> {
    this.temperatureControlActive = active;
    return {
      success: true,
      message: `Controle automático de temperatura ${active ? 'ATIVADO' : 'DESATIVADO'}`
    };
  }

  async setWaterQualityControl(active: boolean): Promise<{ success: boolean; message: string }> {
    this.waterQualityControlActive = active;
    return {
      success: true,
      message: `Controle automático de qualidade da água ${active ? 'ATIVADO' : 'DESATIVADO'}`
    };
  }

  async setWaterLevelControl(active: boolean): Promise<{ success: boolean; message: string }> {
    this.waterLevelControlActive = active;
    return {
      success: true,
      message: `Controle automático de nível da água ${active ? 'ATIVADO' : 'DESATIVADO'}`
    };
  }

  async getAutomationStatus(): Promise<{
    temperatureControl: boolean;
    waterQualityControl: boolean;
    waterLevelControl: boolean;
    lastWaterQualityCheck: Date;
  }> {
    return {
      temperatureControl: this.temperatureControlActive,
      waterQualityControl: this.waterQualityControlActive,
      waterLevelControl: this.waterLevelControlActive,
      lastWaterQualityCheck: this.lastWaterQualityCheck
    };
  }

  // Executar ciclo de água manualmente
  async manualWaterCycle(): Promise<{ success: boolean; message: string }> {
    try {
      await this.performWaterCycle('manual');
      return {
        success: true,
        message: 'Ciclo manual de troca de água iniciado com sucesso'
      };
    } catch (error) {
      this.logger.error('Erro no ciclo manual de água:', error);
      return {
        success: false,
        message: `Erro ao iniciar ciclo manual: ${error.message}`
      };
    }
  }
}