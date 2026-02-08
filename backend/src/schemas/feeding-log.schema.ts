import { Prop, Schema, SchemaFactory } from '@nestjs/mongoose';
import { Document } from 'mongoose';
import { ApiProperty } from '@nestjs/swagger';

export type FeedingLogDocument = FeedingLog & Document;

export enum FeedingType {
  AUTOMATICO = 'automatico',
  MANUAL = 'manual',
}

@Schema({ timestamps: true })
export class FeedingLog {
  @ApiProperty({ description: 'Tipo de alimentação', enum: FeedingType })
  @Prop({ required: true, enum: FeedingType })
  tipo: FeedingType;

  @ApiProperty({ description: 'Horário programado (08:00 ou 16:00)', example: '08:00' })
  @Prop({ type: String })
  horarioProgramado?: string;

  @ApiProperty({ description: 'Duração da alimentação em segundos', example: 5 })
  @Prop({ required: true, type: Number })
  duracao: number;

  @ApiProperty({ description: 'Sucesso da operação', example: true })
  @Prop({ required: true, type: Boolean })
  sucesso: boolean;

  @ApiProperty({ description: 'Observações ou erros' })
  @Prop({ type: String })
  observacoes?: string;

  @ApiProperty({ description: 'Timestamp da alimentação' })
  @Prop({ type: Date, default: Date.now })
  timestamp: Date;
}

export const FeedingLogSchema = SchemaFactory.createForClass(FeedingLog);