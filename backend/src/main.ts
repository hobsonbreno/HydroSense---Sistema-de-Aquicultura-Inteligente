import { NestFactory } from '@nestjs/core';
import { SwaggerModule, DocumentBuilder } from '@nestjs/swagger';
import { ValidationPipe } from '@nestjs/common';
import { AppModule } from './app.module';

async function bootstrap() {
  const app = await NestFactory.create(AppModule);

  // Enable CORS
  app.enableCors({
    origin: ['http://localhost:3001', 'http://10.0.0.181'],
    methods: 'GET,HEAD,PUT,PATCH,POST,DELETE,OPTIONS',
    credentials: true,
  });

  // Global validation pipe
  app.useGlobalPipes(new ValidationPipe({
    whitelist: true,
    forbidNonWhitelisted: true,
    transform: true,
  }));

  // Swagger documentation
  const config = new DocumentBuilder()
    .setTitle('HydroSense API')
    .setDescription('Sistema de Aquicultura Inteligente - API REST para controle total')
    .setVersion('1.0.0')
    .addTag('sensors', 'Controle e leitura de sensores')
    .addTag('relays', 'Controle de relés (motor, bombas)')
    .addTag('automation', 'Automação e regras de negócio')
    .addTag('monitoring', 'Monitoramento em tempo real')
    .addTag('feeding', 'Sistema de alimentação automática')
    .addBearerAuth()
    .build();

  const document = SwaggerModule.createDocument(app, config);
  SwaggerModule.setup('api/docs', app, document);

  const port = process.env.PORT || 3000;
  await app.listen(port);
  
  console.log(`🚀 HydroSense Backend running on: http://localhost:${port}`);
  console.log(`📚 Swagger Documentation: http://localhost:${port}/api/docs`);
}

bootstrap();