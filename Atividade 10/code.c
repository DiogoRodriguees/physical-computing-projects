/*******************************************************************
                              BIBLIOTECAS
********************************************************************/
#include "HX711.h"
#include <avr/io.h>
#include <util/delay.h>

/*******************************************************************
                                DEFINES
********************************************************************/
// Para a leitura da porta serial
#define TAMANHO_ENTRADA 6
#define TAMANHO_BUFFER 10

// Para controle do despejo
#define ABRIR_SAIDA OCR1A = 1096 //Boca virada pra cima
#define FECHAR_SAIDA OCR1A = 4500 //Boca virada pra cima

// Para a leitura dos botões
#define LER_BOTAO_A PIND & (1 << PD4)
#define LER_BOTAO_B PIND & (1 << PD5)
#define LER_BOTAO_C PIND & (1 << PD6)
#define LER_BOTAO_D PIND & (1 << PD7)

// Para a conexão da balança
#define LOADCELL_DOUT_PIN  2
#define LOADCELL_SCK_PIN  3

HX711 scale;

/*******************************************************************
                           VARIAVEIS GLOBAIS
********************************************************************/
long peso = 1000; //Valor da balança

char executar_funcao = 'E';
char buffer[TAMANHO_BUFFER];

bool calibrando = false;
bool tem_novo_valor = false;

int giro = 1092;
int gramas_desejadas = 0;
int gramas_despejadas = 0;
int gramas_por_giro = 0;

float peso_adicionado = 0;
float valor_inicial = 0;
float valor_final = 0;
float conta_gramas = 0;

/*******************************************************************
                       VARIAVEIS PARA OS BOTÕES
********************************************************************/
volatile unsigned long ms = 0;

char leitura_a = 0;
char leitura_b = 0;
char leitura_c = 0;
char leitura_d = 0;

char ultimo_estado_a = (1 << PD4);
char ultimo_estado_b = (1 << PD5);
char ultimo_estado_c = (1 << PD6);
char ultimo_estado_d = (1 << PD7);

unsigned long botao_a = 0;
unsigned long botao_b = 0;
unsigned long botao_c = 0;
unsigned long botao_d = 0;

unsigned long valor_timer = millis();

/*******************************************************************
                         FUNÇÕES SERIAL
********************************************************************/
void lerSerial() {
  static int index = 0;

  while (Serial.available() > 0 && !tem_novo_valor) {
    char caracter_recebido = Serial.read();
    if (caracter_recebido == '\n') {
      buffer[index] = '\0';
      tem_novo_valor = true;
      index = 0;
      return;
    }

    buffer[index] = caracter_recebido;
    index = (index + 1) % TAMANHO_ENTRADA;
  }
}

/*******************************************************************
                        XTRAIR CARACTERES
********************************************************************/
void extrairCaracteres() {
  char caracteres[4] = {'\0', '\0', '\0', '\0'};
  int tamanh_entrada = strlen(buffer);

  if (tamanh_entrada > 6 || tamanh_entrada < 6) {
    strncpy(caracteres, buffer + tamanh_entrada - 3, 3);
    gramas_desejadas = atoi(caracteres);
    Serial.print("Caracteres: ");
    Serial.println(gramas_desejadas);
  } else {
    Serial.println("Formato válido --> s:xxx");
  }

  tem_novo_valor = false;
}

/*******************************************************************
                       FUNÇÕES DE CALIBRAÇÃO
********************************************************************/
void iniciarCalibracao() {
  calibrando = true;
  scale.tare();
  valor_inicial = peso;
  Serial.println("Coloque 100 gramas na balança e precione o botão B.");
}

void finalizarCalibracao() {
  valor_final = peso;
  conta_gramas = ((float)valor_final - valor_inicial) / (float)100;
  scale.set_scale(conta_gramas);

  Serial.println(valor_inicial);
  Serial.println(valor_final);
  Serial.println(conta_gramas);

  calibrando = false;
}

/*******************************************************************
                        FUNÇÕES DE DESPEJO
********************************************************************/
void calibrarDespejo() {
  Serial.println("Calibrando Despejo");
  float peso_inicial = peso_adicionado;
  float peso_final = 0;

  FECHAR_SAIDA;
  delay(1000);
  ABRIR_SAIDA;
  delay(3000);

  // Verifica a diferença com o peso extra adicionado
  peso_adicionado = scale.get_units(5);
  peso_final = peso_adicionado;
  gramas_por_giro = peso_inicial - peso_final;

  FECHAR_SAIDA;
}

void despejar() {
  float gramas_inicias = abs(scale.get_units(5));
  float gramas_atuais = gramas_inicias;
  Serial.println("Despejar -->");
  Serial.println(gramas_atuais);

  // Despeja enquanto a quantidade desejada não for alcançada
  do {
    FECHAR_SAIDA;
    delay(1500);

    ABRIR_SAIDA;
    delay(1500);

    gramas_atuais = abs(scale.get_units(5));

    FECHAR_SAIDA;
    Serial.println(gramas_atuais);
    Serial.println(gramas_atuais - gramas_inicias);
    Serial.println("-----------");
  } while ((gramas_atuais - gramas_inicias) < gramas_desejadas);

  gramas_despejadas = 0;
}

/*******************************************************************
                      FUNÇÕES DE CADA BOTÃO
********************************************************************/
void executarFuncaoA() {
  iniciarCalibracao();
}

void executarFuncaoB() {
  if (calibrando) finalizarCalibracao();
  else scale.tare();
}

void executarFuncaoC() {
  calibrarDespejo();
  digitalWrite(13, true);
}

void executarFuncaoD() {
  despejar();
}

/*******************************************************************
                   FUNÇÕES DE LEITURA DOS BOTÕES
********************************************************************/
void executarBotaoClicado() {
  // Executa a função de acordo com o botão clicado
  switch (executar_funcao) {
    case 'A': executarFuncaoA();
      break;

    case 'B': executarFuncaoB();

      break;

    case 'C': executarFuncaoC();

      break;

    case 'D': executarFuncaoD();
      break;
  }
  executar_funcao = 'E';

}

// Função genérica para verificar o clique nos botões
void verificarClique(char estado, char ultimo_estado, unsigned long *botao, char identificador) {
  if (estado != ultimo_estado && (millis() - *botao) > 10 && estado == 0) {
    *botao = millis();
    executar_funcao = identificador;
  }

  executarBotaoClicado();
}

/*******************************************************************
                        FUNÇÕES DE MENSAGENS
********************************************************************/
void exibirMensagemPadrao() {
  Serial.println("--------------------------------------");
  Serial.print("* Peso atual          : ");
  Serial.println(peso);
  peso_adicionado = scale.get_units(1);
  Serial.print("* Peso com 100g a mais: ");
  Serial.println(peso_adicionado);
  Serial.print("* Gramas por despejo  : ");
  Serial.println(gramas_por_giro);
  Serial.print("* Gramas desejadas    : ");
  Serial.println(gramas_desejadas);
  Serial.println("--------------------------------------");
}

/*******************************************************************
                                SETUP
********************************************************************/
void setup() {
  Serial.begin(9600);

  // Configurando os PORTB e PORTD
  PORTD = 0b11110000;
  DDRB = 0b00000010;
  DDRD  = 0b00000010;
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);

  // Configurações da balança
  ICR1 = 39999;
  TCCR1A = (1 << WGM11);
  TCCR1B = (1 << WGM12) | (1 << WGM13) | (1 << CS11);
  TCCR1A |= (1 << COM1A1);
  FECHAR_SAIDA;

  // Interrupçção
  TCCR2A |= (1 << WGM01); // MODO CTC
  TCCR2B |= (1 << CS02); // prescaler = 256
  TIMSK2 |= (1 << OCIE0A); // habilita interrupção por compare match
  OCR2A = 62; // T = (62+1)*256/16000000 = 0,000992 ~= 1ms -> f ~= 1008,06 Hz

  // Primeira leitura do botões
  leitura_a = millis();
  leitura_b = millis();
  leitura_c = millis();
  leitura_d = millis();
}

/*******************************************************************
                                  LOOP
********************************************************************/
void loop() {
  lerSerial();

  if (tem_novo_valor) extrairCaracteres();

  // Verifica se deve calibrar novamente (Um novo valor foi lido)
  if ((millis() - valor_timer) >= 1000) {
    valor_timer = millis();

    if (scale.is_ready()) {
      peso = scale.read();
      if (!calibrando) exibirMensagemPadrao();
    }
  }

  leitura_a = LER_BOTAO_A;
  leitura_b = LER_BOTAO_B;
  leitura_c = LER_BOTAO_C;
  leitura_d = LER_BOTAO_D;

  verificarClique(leitura_a, ultimo_estado_a, &botao_a, 'A');
  verificarClique(leitura_b, ultimo_estado_b, &botao_b, 'B');
  verificarClique(leitura_c, ultimo_estado_c, &botao_c, 'C');
  verificarClique(leitura_d, ultimo_estado_d, &botao_d, 'D');

  ultimo_estado_a = leitura_a;
  ultimo_estado_b = leitura_b;
  ultimo_estado_c = leitura_c;
  ultimo_estado_d = leitura_d;
  delay(1);
}


/*******************************************************************
                            INTERRUPÇÃO
********************************************************************/
ISR(TIMER2_COMPA_vect) {
  ms++; // incrementa a cada segundo
}


