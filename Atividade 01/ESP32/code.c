// Incluir biblioteca do ESP32
#include <Arduino.h>

// Definir o pino do LED
#define LED_PIN  14  
#define LED_PIN2  15

void setup()
{
  // Configurar o pino do LED como saída
  pinMode(LED_PIN, OUTPUT);
  pinMode(LED_PIN2, OUTPUT);
}

void loop()
{
  // acende o lado por 500ms
  digitalWrite(LED_PIN, HIGH);
  digitalWrite(LED_PIN2, HIGH);
  delay(500);  
  
  // mantém o led apagado por 500ms
  digitalWrite(LED_PIN, LOW);
  digitalWrite(LED_PIN2, LOW);
  delay(500); 
}
