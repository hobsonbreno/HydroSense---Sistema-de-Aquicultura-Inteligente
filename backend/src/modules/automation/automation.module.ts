import { Module } from '@nestjs/common';
import { MongooseModule } from '@nestjs/mongoose';
import { ScheduleModule } from '@nestjs/schedule';
import { AutomationController } from './automation.controller';
import { AutomationService } from './automation.service';
import { SensorData, SensorDataSchema } from '../../schemas/sensor-data.schema';
import { RelayControl, RelayControlSchema } from '../../schemas/relay-control.schema';
import { RelaysModule } from '../relays/relays.module';

@Module({
  imports: [
    MongooseModule.forFeature([
      { name: SensorData.name, schema: SensorDataSchema },
      { name: RelayControl.name, schema: RelayControlSchema }
    ]),
    ScheduleModule.forRoot(),
    RelaysModule
  ],
  controllers: [AutomationController],
  providers: [AutomationService],
  exports: [AutomationService]
})
export class AutomationModule {}