import { Injectable } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Model } from 'mongoose';
import { SensorData, SensorDataDocument } from '../../schemas/sensor-data.schema';
import { CreateSensorDataDto } from './dto/create-sensor-data.dto';

@Injectable()
export class SensorsService {
  constructor(
    @InjectModel(SensorData.name)
    private sensorDataModel: Model<SensorDataDocument>,
  ) {}

  async create(createSensorDataDto: CreateSensorDataDto): Promise<SensorData> {
    const sensorData = new this.sensorDataModel(createSensorDataDto);
    return sensorData.save();
  }

  async findAll(limit: number = 100): Promise<SensorData[]> {
    return this.sensorDataModel
      .find()
      .sort({ timestamp: -1 })
      .limit(limit)
      .exec();
  }

  async findLatest(): Promise<SensorData> {
    return this.sensorDataModel
      .findOne()
      .sort({ timestamp: -1 })
      .exec();
  }

  async findByTimeRange(startDate: Date, endDate: Date): Promise<SensorData[]> {
    return this.sensorDataModel
      .find({
        timestamp: {
          $gte: startDate,
          $lte: endDate,
        },
      })
      .sort({ timestamp: -1 })
      .exec();
  }

  async getTemperatureStats(hours: number = 24) {
    const startDate = new Date(Date.now() - hours * 60 * 60 * 1000);
    
    const stats = await this.sensorDataModel.aggregate([
      { $match: { timestamp: { $gte: startDate } } },
      {
        $group: {
          _id: null,
          avgTemp: { $avg: '$temperatura' },
          minTemp: { $min: '$temperatura' },
          maxTemp: { $max: '$temperatura' },
          count: { $sum: 1 },
        },
      },
    ]);

    return stats[0] || null;
  }

  async getWaterLevelHistory(hours: number = 24): Promise<any[]> {
    const startDate = new Date(Date.now() - hours * 60 * 60 * 1000);
    
    return this.sensorDataModel
      .find(
        { timestamp: { $gte: startDate } },
        { timestamp: 1, nivel: 1, volume: 1, _id: 0 }
      )
      .sort({ timestamp: 1 })
      .exec();
  }

  async checkWaterQuality(): Promise<{ clean: boolean; lastColorChange: Date }> {
    const latestData = await this.findLatest();
    const lastCleanData = await this.sensorDataModel
      .findOne({ corAgua: 'cristalino' })
      .sort({ timestamp: -1 })
      .exec();

    return {
      clean: latestData?.corAgua === 'cristalino',
      lastColorChange: latestData?.timestamp || new Date(),
    };
  }

  async deleteOldData(days: number = 30): Promise<number> {
    const cutoffDate = new Date(Date.now() - days * 24 * 60 * 60 * 1000);
    const result = await this.sensorDataModel
      .deleteMany({ timestamp: { $lt: cutoffDate } })
      .exec();
    
    return result.deletedCount;
  }
}