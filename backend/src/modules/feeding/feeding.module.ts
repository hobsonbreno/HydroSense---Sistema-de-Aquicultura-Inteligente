import { Module } from '@nestjs/common';
import { MongooseModule } from '@nestjs/mongoose';
import { ScheduleModule } from '@nestjs/schedule';
import { FeedingController } from './feeding.controller';
import { FeedingService } from './feeding.service';
import { FeedingLog, FeedingLogSchema } from '../../schemas/feeding-log.schema';
import axios from 'axios';

@Module({
  imports: [
    MongooseModule.forFeature([
      { name: FeedingLog.name, schema: FeedingLogSchema }
    ]),
    ScheduleModule.forRoot()
  ],
  controllers: [FeedingController],
  providers: [FeedingService],
  exports: [FeedingService]
})
export class FeedingModule {}