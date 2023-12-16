void setup() {
  DDRB |= (1 << PORTB1); // Configura PB1 como saída
  DDRD |= (1 << PORTD6); // Comfigura PD6 como saída
}

void loop() {
  PORTB |= (1 << PORTB1); // Acende o LED amarelo
  
  delay(50);

  PORTB &= ~(1 << PORTB1); // Apaga o LED amarelo
  PORTD |= (1 << PORTD6);  // Acende o LED verde
  
  delay(50);

  PORTD &= ~(1 << PORTD6);  // Apaga o LED verde

  delay(450);
}
