import { Injectable, BadRequestException } from '@nestjs/common';
import { InjectModel } from '@nestjs/mongoose';
import { Model } from 'mongoose';
import { RelayControl, RelayControlDocument } from '../../schemas/relay-control.schema';
import { CreateRelayCommandDto } from './dto/create-relay-command.dto';
import axios from 'axios';

@Injectable()
export class RelaysService {
  constructor(
    @InjectModel(RelayControl.name) private relayControlModel: Model<RelayControlDocument>
  ) {}

  private readonly PICO_IP = process.env.PICO_IP || '10.0.0.181';
  private readonly PICO_PORT = process.env.PICO_PORT || '80';

  async controlRelay(createRelayCommand: CreateRelayCommandDto): Promise<RelayControl> {
    const { tipo, estado, origem = 'manual', observacao } = createRelayCommand;

    try {
      // Validar tipo de relé
      const validTypes = ['LN1', 'LN2', 'LN3'];
      if (!validTypes.includes(tipo)) {
        throw new BadRequestException(`Tipo de relé inválido. Use: ${validTypes.join(', ')}`);
      }

      // Definir mapeamento de relés
      const relayMappings = {
        'LN1': { gpio: 14, description: 'Motor/Ventilador' },
        'LN2': { gpio: 15, description: 'Bomba 01 (Esvaziar)' },
        'LN3': { gpio: 16, description: 'Bomba 02 (Encher)' }
      };

      const relay = relayMappings[tipo];

      // Enviar comando para o Pico W
      const picoUrl = `http://${this.PICO_IP}:${this.PICO_PORT}/relay`;
      const payload = {
        pin: relay.gpio,
        state: estado ? 1 : 0,
        type: tipo,
        description: relay.description
      };

      console.log(`Enviando comando para Pico W: ${picoUrl}`, payload);

      try {
        const response = await axios.post(picoUrl, payload, {
          timeout: 5000,
          headers: { 'Content-Type': 'application/json' }
        });
        
        console.log('Resposta do Pico W:', response.data);
      } catch (picoError) {
        console.warn('Erro ao comunicar com Pico W:', picoError.message);
        // Continuar mesmo se Pico W não responder (para testing)
      }

      // Salvar comando no banco
      const relayControl = new this.relayControlModel({
        tipo,
        estado,
        origem,
        observacao: observacao || `${relay.description} ${estado ? 'LIGADO' : 'DESLIGADO'}`,
        timestamp: new Date()
      });

      return await relayControl.save();
    } catch (error) {
      console.error('Erro no controle de relé:', error);
      throw error;
    }
  }

  async getRelayHistory(
    tipo?: string, 
    limit: number = 50, 
    page: number = 1
  ): Promise<{ data: RelayControl[], total: number, page: number, pages: number }> {
    const filter = tipo ? { tipo } : {};
    const skip = (page - 1) * limit;

    const [data, total] = await Promise.all([
      this.relayControlModel
        .find(filter)
        .sort({ timestamp: -1 })
        .limit(limit)
        .skip(skip)
        .exec(),
      this.relayControlModel.countDocuments(filter)
    ]);

    return {
      data,
      total,
      page,
      pages: Math.ceil(total / limit)
    };
  }

  async getRelayStatus(): Promise<any> {
    try {
      // Buscar último estado de cada relé
      const relayTypes = ['LN1', 'LN2', 'LN3'];
      const statuses = {};

      for (const tipo of relayTypes) {
        const lastCommand = await this.relayControlModel
          .findOne({ tipo })
          .sort({ timestamp: -1 })
          .exec();

        statuses[tipo] = {
          estado: lastCommand?.estado || false,
          ultimaAlteracao: lastCommand?.timestamp || null,
          observacao: lastCommand?.observacao || 'Nunca utilizado'
        };
      }

      return {
        relays: statuses,
        picoStatus: await this.checkPicoConnection()
      };
    } catch (error) {
      console.error('Erro ao buscar status dos relés:', error);
      throw error;
    }
  }

  private async checkPicoConnection(): Promise<{ connected: boolean, ip: string, lastCheck: Date }> {
    try {
      const response = await axios.get(`http://${this.PICO_IP}:${this.PICO_PORT}/status`, {
        timeout: 3000
      });
      
      return {
        connected: true,
        ip: this.PICO_IP,
        lastCheck: new Date()
      };
    } catch (error) {
      return {
        connected: false,
        ip: this.PICO_IP,
        lastCheck: new Date()
      };
    }
  }

  // Métodos específicos para automação
  async controlVentilator(turnOn: boolean, reason: string): Promise<RelayControl> {
    return this.controlRelay({
      tipo: 'LN1',
      estado: turnOn,
      origem: 'automatico',
      observacao: `Ventilador ${turnOn ? 'LIGADO' : 'DESLIGADO'} - ${reason}`
    });
  }

  async controlDrainPump(turnOn: boolean, reason: string): Promise<RelayControl> {
    return this.controlRelay({
      tipo: 'LN2',
      estado: turnOn,
      origem: 'automatico',
      observacao: `Bomba Esvaziar ${turnOn ? 'LIGADA' : 'DESLIGADA'} - ${reason}`
    });
  }

  async controlFillPump(turnOn: boolean, reason: string): Promise<RelayControl> {
    return this.controlRelay({
      tipo: 'LN3',
      estado: turnOn,
      origem: 'automatico',
      observacao: `Bomba Encher ${turnOn ? 'LIGADA' : 'DESLIGADA'} - ${reason}`
    });
  }
}