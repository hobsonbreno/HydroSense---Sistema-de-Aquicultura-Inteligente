import { IsNumber, IsString, IsOptional, Min, Max } from 'class-validator';
import { ApiProperty, ApiPropertyOptional } from '@nestjs/swagger';

export class GetSensorsQueryDto {
  @ApiPropertyOptional({ description: 'Página dos resultados (começa em 1)', example: 1, minimum: 1 })
  @IsOptional()
  @IsNumber()
  @Min(1)
  page?: number = 1;

  @ApiPropertyOptional({ description: 'Limite de itens por página', example: 50, minimum: 1, maximum: 1000 })
  @IsOptional()
  @IsNumber()
  @Min(1)
  @Max(1000)
  limit?: number = 50;

  @ApiPropertyOptional({ description: 'Data de início (ISO string)', example: '2026-02-01T00:00:00.000Z' })
  @IsOptional()
  @IsString()
  startDate?: string;

  @ApiPropertyOptional({ description: 'Data de fim (ISO string)', example: '2026-02-08T23:59:59.999Z' })
  @IsOptional()
  @IsString()
  endDate?: string;

  @ApiPropertyOptional({ 
    description: 'Filtrar por cor da água',
    enum: ['cristalino', 'turvo', 'verde', 'marrom', 'outro']
  })
  @IsOptional()
  @IsString()
  corAgua?: string;

  @ApiPropertyOptional({ description: 'Temperatura mínima para filtro', minimum: -50, maximum: 100 })
  @IsOptional()
  @IsNumber()
  @Min(-50)
  @Max(100)
  tempMin?: number;

  @ApiPropertyOptional({ description: 'Temperatura máxima para filtro', minimum: -50, maximum: 100 })
  @IsOptional()
  @IsNumber()
  @Min(-50)
  @Max(100)
  tempMax?: number;

  @ApiPropertyOptional({ description: 'Nível mínimo da água (%)', minimum: 0, maximum: 100 })
  @IsOptional()
  @IsNumber()
  @Min(0)
  @Max(100)
  nivelMin?: number;
}