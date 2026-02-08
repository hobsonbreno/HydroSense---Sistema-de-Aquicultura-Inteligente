import { Module } from '@nestjs/common';
import { MongooseModule } from '@nestjs/mongoose';
import { RelaysController } from './relays.controller';
import { RelaysService } from './relays.service';
import { RelayControl, RelayControlSchema } from '../../schemas/relay-control.schema';

@Module({
  imports: [
    MongooseModule.forFeature([
      { name: RelayControl.name, schema: RelayControlSchema }
    ])
  ],
  controllers: [RelaysController],
  providers: [RelaysService],
  exports: [RelaysService]
})
export class RelaysModule {}