import { Module } from '@nestjs/common';
import { MongooseModule } from '@nestjs/mongoose';
import { ScheduleModule } from '@nestjs/schedule';
import { SensorsModule } from './modules/sensors/sensors.module';
import { RelaysModule } from './modules/relays/relays.module';
import { AutomationModule } from './modules/automation/automation.module';
import { MonitoringModule } from './modules/monitoring/monitoring.module';
import { FeedingModule } from './modules/feeding/feeding.module';
import { PicoWModule } from './modules/pico-w/pico-w.module';

@Module({
  imports: [
    // MongoDB connection
    MongooseModule.forRoot(
      process.env.MONGODB_URI || 'mongodb://localhost:27018/hydrosense'
    ),
    
    // Schedule module for cron jobs
    ScheduleModule.forRoot(),
    
    // Feature modules
    SensorsModule,
    RelaysModule,
    AutomationModule,
    MonitoringModule,
    FeedingModule,
    PicoWModule,
  ],
})
export class AppModule {}