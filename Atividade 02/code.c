/*
  Descrição:
    1. Ao girar o encoder no sentido horario, os leds se acendem da direita para a esquerda.
    2. Ao girar o econder no sentido anti-horário os leds se apagam, da esquerda para a direita.
    3. Ao segurar no botão central os leds apagados acendem e os acessos se apagam.
    4. Ao clicar no botão central as regras 1 e 2 são invertidas.

  Code:
    C language
*/

#define pinoDT 8
#define pinoCLK 9
#define centerButton 10

int apaga = 0;
int acende = 1;

int count = 0;              // creferencia o led que sera aceso
int clique = 0;             // guarda o tempo de click dos pinos CLK ou DT

int recentlyOff = 0;        // se -- 1, o botão central foi precionado por 1000ms
int clickCenterButton = 0;  // guarda o tempo de clique no botão central

// guarda os stados do pino CLK
int state;
int lastState;

// guarda os estados do botão central
int stateCenterButton;
int lastStateCenterButton;


void setup(){
  // configurando pinos CLK, DT e SW como entradas
  pinMode(pinoCLK, INPUT_PULLUP);
  pinMode(pinoDT, INPUT_PULLUP);
  pinMode(centerButton, INPUT_PULLUP);

  // configurando a PORTD como saída
  DDRD = 0xFF;

  // leitura inicial do pino CLK e SW
  lastState = digitalRead(pinoCLK);
  lastStateCenterButton = digitalRead(centerButton);

}

void loop(){
  state = digitalRead(pinoCLK);
  stateCenterButton = digitalRead(centerButton);

  // se o estado atual de CLK for diferente do anterior, houve um giro.
  if (lastState != state && digitalRead(pinoCLK) == 0 && (millis() - clique) > 2) {

    clique = millis();

    // acende os led da direita para a esquerda e apaga da esquerda para a direita
    if (digitalRead(pinoDT) == state) {
      // girando sentido horário
      digitalWrite(count - 1, apaga);
      if (count > 0) count--;

    } else {
      // girando sentido anti-horário
      digitalWrite(count, acende);
      if (count <= 7) count++;
    }


  }

  // clique no botão central
  if (lastStateCenterButton != stateCenterButton && (millis() - clickCenterButton) > 1) {
    clickCenterButton = millis();

    if (digitalRead(centerButton) == 1 && recentlyOff == 0) {
      PORTD = ~PORTD; // acende os leds apagados e apaga os leds acesos

      apaga = apaga == 0 ? 1 : 0;
      acende = acende == 0 ? 1 : 0;
    }

    recentlyOff = 0;

  }

  // verifica se o botão central está clicado por mais de 1000ms
  if (digitalRead(centerButton) == 0 && lastStateCenterButton == stateCenterButton && (millis() - clickCenterButton) > 1000) {
    PORTD &= 0x00;  // reseta os leds conectados na PORTD
    count = 0;      // reseta o contador de leds ligados
    recentlyOff = 1;

    if (acende == 0) {
      apaga = apaga == 0 ? 1 : 0;
      acende = acende == 0 ? 1 : 0;
    }

    clickCenterButton = millis();
  }


  lastState = state;
  lastStateCenterButton = stateCenterButton;
}