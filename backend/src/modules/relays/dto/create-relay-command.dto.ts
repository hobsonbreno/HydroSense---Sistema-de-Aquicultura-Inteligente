import { IsString, IsBoolean, IsOptional } from 'class-validator';
import { ApiProperty, ApiPropertyOptional } from '@nestjs/swagger';

export class CreateRelayCommandDto {
  @ApiProperty({ 
    description: 'Tipo do relé a ser controlado',
    example: 'LN1',
    enum: ['LN1', 'LN2', 'LN3']
  })
  @IsString()
  tipo: 'LN1' | 'LN2' | 'LN3';

  @ApiProperty({ 
    description: 'Estado desejado do relé (true = ligado, false = desligado)',
    example: true
  })
  @IsBoolean()
  estado: boolean;

  @ApiPropertyOptional({ 
    description: 'Origem do comando (manual ou automatico)',
    example: 'manual',
    enum: ['manual', 'automatico'],
    default: 'manual'
  })
  @IsOptional()
  @IsString()
  origem?: 'manual' | 'automatico';

  @ApiPropertyOptional({ 
    description: 'Observação ou razão do comando',
    example: 'Controle manual via interface web'
  })
  @IsOptional()
  @IsString()
  observacao?: string;
}