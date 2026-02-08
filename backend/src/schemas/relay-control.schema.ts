import { Prop, Schema, SchemaFactory } from '@nestjs/mongoose';
import { Document } from 'mongoose';
import { ApiProperty } from '@nestjs/swagger';

export type RelayControlDocument = RelayControl & Document;

export enum RelayType {
  MOTOR_VENTILADOR = 'LN1', // Motor ventilador
  BOMBA_ESVAZIAR = 'LN2',   // Bomba de esvaziamento
  BOMBA_ENCHER = 'LN3',     // Bomba de enchimento
}

@Schema({ timestamps: true })
export class RelayControl {
  @ApiProperty({ description: 'Tipo do relé', enum: RelayType })
  @Prop({ required: true, enum: RelayType })
  relayType: RelayType;

  @ApiProperty({ description: 'Estado atual do relé', example: true })
  @Prop({ required: true, type: Boolean })
  estado: boolean;

  @ApiProperty({ description: 'Motivo da ativação', example: 'Temperatura acima de 29°C' })
  @Prop({ required: true, type: String })
  motivo: string;

  @ApiProperty({ description: 'Duração em segundos (null para indefinido)', example: 300 })
  @Prop({ type: Number, default: null })
  duracao: number;

  @ApiProperty({ description: 'Timestamp da ação' })
  @Prop({ type: Date, default: Date.now })
  timestamp: Date;

  @ApiProperty({ description: 'Ativado automaticamente pelo sistema', example: true })
  @Prop({ type: Boolean, default: false })
  automatico: boolean;
}

export const RelayControlSchema = SchemaFactory.createForClass(RelayControl);