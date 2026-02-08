import { Prop, Schema, SchemaFactory } from '@nestjs/mongoose';
import { Document } from 'mongoose';
import { ApiProperty } from '@nestjs/swagger';

export type SensorDataDocument = SensorData & Document;

@Schema({ timestamps: true })
export class SensorData {
  @ApiProperty({ description: 'Temperatura em Celsius', example: 27.5 })
  @Prop({ required: true, type: Number })
  temperatura: number;

  @ApiProperty({ description: 'Umidade relativa em %', example: 65 })
  @Prop({ required: true, type: Number })
  umidade: number;

  @ApiProperty({ description: 'Distância em mm (sensor infravermelho)', example: 120 })
  @Prop({ required: true, type: Number })
  distancia: number;

  @ApiProperty({ description: 'Nível da água em %', example: 75 })
  @Prop({ required: true, type: Number })
  nivel: number;

  @ApiProperty({ description: 'Volume da água em litros', example: 15.0 })
  @Prop({ required: true, type: Number })
  volume: number;

  @ApiProperty({ description: 'Cor da água detectada pelo sensor', example: 'cristalino' })
  @Prop({ required: true, type: String })
  corAgua: string;

  @ApiProperty({ description: 'Status WiFi do dispositivo', example: true })
  @Prop({ required: true, type: Boolean })
  wifiStatus: boolean;

  @ApiProperty({ description: 'Contador de leituras', example: 150 })
  @Prop({ required: true, type: Number })
  contadorLeituras: number;

  @ApiProperty({ description: 'IP do dispositivo Pico W', example: '10.0.0.181' })
  @Prop({ required: true, type: String })
  deviceIp: string;

  @ApiProperty({ description: 'Timestamp da leitura' })
  @Prop({ type: Date, default: Date.now })
  timestamp: Date;
}

export const SensorDataSchema = SchemaFactory.createForClass(SensorData);