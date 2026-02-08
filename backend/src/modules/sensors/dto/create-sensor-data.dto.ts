import { IsNumber, IsString, IsBoolean, IsOptional, Min, Max } from 'class-validator';
import { ApiProperty } from '@nestjs/swagger';

export class CreateSensorDataDto {
  @ApiProperty({ description: 'Temperatura em Celsius', example: 27.5, minimum: -50, maximum: 100 })
  @IsNumber()
  @Min(-50)
  @Max(100)
  temperatura: number;

  @ApiProperty({ description: 'Umidade relativa em %', example: 65, minimum: 0, maximum: 100 })
  @IsNumber()
  @Min(0)
  @Max(100)
  umidade: number;

  @ApiProperty({ description: 'Distância em mm (sensor infravermelho)', example: 120 })
  @IsNumber()
  @Min(0)
  distancia: number;

  @ApiProperty({ description: 'Nível da água em %', example: 75, minimum: 0, maximum: 100 })
  @IsNumber()
  @Min(0)
  @Max(100)
  nivel: number;

  @ApiProperty({ description: 'Volume da água em litros', example: 15.0, minimum: 0, maximum: 20 })
  @IsNumber()
  @Min(0)
  @Max(20)
  volume: number;

  @ApiProperty({ 
    description: 'Cor da água detectada pelo sensor', 
    example: 'cristalino',
    enum: ['cristalino', 'turvo', 'verde', 'marrom', 'outro']
  })
  @IsString()
  corAgua: string;

  @ApiProperty({ description: 'Status WiFi do dispositivo', example: true })
  @IsBoolean()
  wifiStatus: boolean;

  @ApiProperty({ description: 'Contador de leituras', example: 150 })
  @IsNumber()
  @Min(0)
  contadorLeituras: number;

  @ApiProperty({ description: 'IP do dispositivo Pico W', example: '10.0.0.181' })
  @IsString()
  deviceIp: string;

  @ApiProperty({ description: 'Timestamp da leitura (opcional, será usado Date.now() se não informado)' })
  @IsOptional()
  timestamp?: Date;
}