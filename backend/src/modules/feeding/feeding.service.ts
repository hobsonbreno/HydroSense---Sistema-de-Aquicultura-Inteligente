import { Injectable, Logger } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Cron, CronExpression } from '@nestjs/schedule';
import { Model } from 'mongoose';
import { FeedingLog, FeedingLogDocument } from '../../schemas/feeding-log.schema';
import axios from 'axios';

@Injectable()
export class FeedingService {
  private readonly logger = new Logger(FeedingService.name);
  private feedingActive = true;

  private readonly PICO_IP = process.env.PICO_IP || '10.0.0.181';
  private readonly PICO_PORT = process.env.PICO_PORT || '80';

  constructor(
    @InjectModel(FeedingLog.name) private feedingLogModel: Model<FeedingLogDocument>
  ) {}

  // Alimentação automática às 08:00
  @Cron('0 0 8 * * *') // 08:00 todos os dias
  async morningFeeding() {
    if (!this.feedingActive) return;
    
    await this.performFeeding('automatico', 'Alimentação matinal automática - 08:00');
  }

  // Alimentação automática às 16:00
  @Cron('0 0 16 * * *') // 16:00 todos os dias
  async eveningFeeding() {
    if (!this.feedingActive) return;
    
    await this.performFeeding('automatico', 'Alimentação vespertina automática - 16:00');
  }

  async performFeeding(tipo: 'automatico' | 'manual', observacao: string): Promise<FeedingLog> {
    try {
      this.logger.log(`Iniciando alimentação: ${observacao}`);

      // Comandar servo motor no Pico W
      const servoCommand = {
        servo: {
          pin: 2, // GPIO do servo motor
          action: 'feed',
          angle: 360, // Rotação completa
          duration: 2000 // 2 segundos
        }
      };

      try {
        const response = await axios.post(
          `http://${this.PICO_IP}:${this.PICO_PORT}/servo`,
          servoCommand,
          {
            timeout: 10000,
            headers: { 'Content-Type': 'application/json' }
          }
        );
        
        this.logger.log('Comando de alimentação enviado ao Pico W:', response.data);
      } catch (picoError) {
        this.logger.warn('Erro ao comunicar com Pico W para alimentação:', picoError.message);
        // Continuar para registrar no log mesmo se Pico W não responder
      }

      // Registrar no banco de dados
      const feedingLog = new this.feedingLogModel({
        tipo,
        observacao,
        sucesso: true,
        timestamp: new Date()
      });

      const savedLog = await feedingLog.save();
      this.logger.log(`Alimentação registrada com sucesso: ${savedLog._id}`);
      
      return savedLog;

    } catch (error) {
      this.logger.error('Erro durante alimentação:', error);
      
      // Registrar erro no banco
      const errorLog = new this.feedingLogModel({
        tipo,
        observacao: `ERRO: ${observacao} - ${error.message}`,
        sucesso: false,
        timestamp: new Date()
      });

      return await errorLog.save();
    }
  }

  async getFeedingHistory(
    limit: number = 50,
    page: number = 1,
    startDate?: string,
    endDate?: string
  ): Promise<{
    data: FeedingLog[];
    total: number;
    page: number;
    pages: number;
    stats: {
      totalFeedings: number;
      successfulFeedings: number;
      failedFeedings: number;
      automaticFeedings: number;
      manualFeedings: number;
    };
  }> {
    const skip = (page - 1) * limit;

    // Construir filtro de data
    let dateFilter = {};
    if (startDate || endDate) {
      dateFilter['timestamp'] = {};
      if (startDate) {
        dateFilter['timestamp']['$gte'] = new Date(startDate);
      }
      if (endDate) {
        dateFilter['timestamp']['$lte'] = new Date(endDate);
      }
    }

    // Buscar dados paginados
    const [data, total] = await Promise.all([
      this.feedingLogModel
        .find(dateFilter)
        .sort({ timestamp: -1 })
        .limit(limit)
        .skip(skip)
        .exec(),
      this.feedingLogModel.countDocuments(dateFilter)
    ]);

    // Calcular estatísticas
    const stats = await this.calculateFeedingStats(dateFilter);

    return {
      data,
      total,
      page,
      pages: Math.ceil(total / limit),
      stats
    };
  }

  private async calculateFeedingStats(dateFilter: any): Promise<{
    totalFeedings: number;
    successfulFeedings: number;
    failedFeedings: number;
    automaticFeedings: number;
    manualFeedings: number;
  }> {
    const [
      totalFeedings,
      successfulFeedings,
      failedFeedings,
      automaticFeedings,
      manualFeedings
    ] = await Promise.all([
      this.feedingLogModel.countDocuments(dateFilter),
      this.feedingLogModel.countDocuments({ ...dateFilter, sucesso: true }),
      this.feedingLogModel.countDocuments({ ...dateFilter, sucesso: false }),
      this.feedingLogModel.countDocuments({ ...dateFilter, tipo: 'automatico' }),
      this.feedingLogModel.countDocuments({ ...dateFilter, tipo: 'manual' })
    ]);

    return {
      totalFeedings,
      successfulFeedings,
      failedFeedings,
      automaticFeedings,
      manualFeedings
    };
  }

  async getTodayFeedings(): Promise<FeedingLog[]> {
    const startOfDay = new Date();
    startOfDay.setHours(0, 0, 0, 0);

    const endOfDay = new Date();
    endOfDay.setHours(23, 59, 59, 999);

    return await this.feedingLogModel
      .find({
        timestamp: {
          $gte: startOfDay,
          $lte: endOfDay
        }
      })
      .sort({ timestamp: -1 })
      .exec();
  }

  async getNextScheduledFeeding(): Promise<{
    nextFeeding: string;
    timeUntilNext: string;
    todayFeedings: number;
  }> {
    const now = new Date();
    const today8AM = new Date();
    today8AM.setHours(8, 0, 0, 0);

    const today4PM = new Date();
    today4PM.setHours(16, 0, 0, 0);

    const tomorrow8AM = new Date();
    tomorrow8AM.setDate(tomorrow8AM.getDate() + 1);
    tomorrow8AM.setHours(8, 0, 0, 0);

    let nextFeeding: Date;

    if (now < today8AM) {
      nextFeeding = today8AM;
    } else if (now < today4PM) {
      nextFeeding = today4PM;
    } else {
      nextFeeding = tomorrow8AM;
    }

    const timeUntilNext = this.formatTimeUntil(now, nextFeeding);
    const todayFeedings = await this.getTodayFeedings();

    return {
      nextFeeding: nextFeeding.toLocaleString('pt-BR'),
      timeUntilNext,
      todayFeedings: todayFeedings.length
    };
  }

  private formatTimeUntil(from: Date, to: Date): string {
    const diffMs = to.getTime() - from.getTime();
    const hours = Math.floor(diffMs / (1000 * 60 * 60));
    const minutes = Math.floor((diffMs % (1000 * 60 * 60)) / (1000 * 60));

    return `${hours}h ${minutes}min`;
  }

  async setFeedingActive(active: boolean): Promise<{ success: boolean; message: string }> {
    this.feedingActive = active;
    return {
      success: true,
      message: `Alimentação automática ${active ? 'ATIVADA' : 'DESATIVADA'}`
    };
  }

  async getFeedingStatus(): Promise<{
    active: boolean;
    nextScheduled: any;
    todayFeedings: FeedingLog[];
  }> {
    const nextScheduled = await this.getNextScheduledFeeding();
    const todayFeedings = await this.getTodayFeedings();

    return {
      active: this.feedingActive,
      nextScheduled,
      todayFeedings
    };
  }

  // Alimentação manual
  async manualFeeding(observacao?: string): Promise<FeedingLog> {
    const defaultObservacao = observacao || 'Alimentação manual via API';
    return await this.performFeeding('manual', defaultObservacao);
  }
}