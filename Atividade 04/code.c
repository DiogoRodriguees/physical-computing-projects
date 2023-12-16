
/**********************************************************************
                                BIBLIOTECAS
***********************************************************************/
#include <util/delay.h>   //biblioteca para o uso das rotinas de _delay_
#include <avr/pgmspace.h> //biblioteca para poder gravar dados na memória flash
#include <avr/io.h> 	 	//definições do componente especificado
#include <util/delay.h>		//biblioteca para o uso das rotinas de _delay_
#include <avr/pgmspace.h>	//biblioteca para poder gravar dados na memória flash 


/**********************************************************************
                                 DEFINES
***********************************************************************/
#define tst_bit(Y, bit_x) (Y & (1 << bit_x)) // testa o bit x da variável Y (retorna 0 ou 1)
#define clr_bit(Y, bit_x) (Y &= ~(1 << bit_x))
#define set_bit(Y, bit_x) (Y |= (1 << bit_x))

#define LINHA PIND   // registrador para a leitura das linhas
#define COLUNA PORTD // registrador para a escrita nas colunas
#define ACENDER_LED PORTB |= (1 << PB5)
#define APAGAR_LED PORTB &= ~(1 << PB5)
#define TOTAL_DE_DISPLAYS 6
#define NENHUMA 0xFF

/**********************************************************************
                                VARIAVEIS
***********************************************************************/
char coluna = 0;
bool cofreAberto = true;

int displayAtivo = 0;	// Define qual display mostrará o número precionado
int totalDeDigitos = 0;	// Define qual display deve ser ligado

unsigned char senha[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};	// password saved
unsigned char digitos[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};	// password incomplete
unsigned char colunaPrecionada = 0xFF;

unsigned long clique = 0;
unsigned long ultimaAtualizacaoDisplay = 0;
unsigned long ultimaVerificacaoDeColuna = 0;

const unsigned char Tabela[] PROGMEM = {0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78, 0x80, 0x18, 0x08, 0x03, 0x46, 0x21, 0x06, 0x0E};
const unsigned char keyboard[4][4] PROGMEM = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
}; 

/**********************************************************************
                          FUNÇÕES DO TECLADO
***********************************************************************/
unsigned char lerTeclado() {
  unsigned char j; 
  unsigned char line;
  unsigned char tecla = 0xFF;	// 0xFF == nenhuma tecla precionada

  clr_bit(COLUNA, coluna); 	
  line = LINHA >> 4;

  for (j = 0; j < 4; j++) {
    if (!tst_bit(line, j)) {		
        if ((millis() - clique) > 10 && (colunaPrecionada == 0b11111111)) {
            clique = millis();	
            colunaPrecionada = PIND;
            tecla = pgm_read_byte(&keyboard[j][coluna]);
        }
    }

    if (((PIND & 0b00001111) == (colunaPrecionada & 0b00001111)) && ((PIND & 0b11110000) == 0b11110000)) {
      colunaPrecionada = 0b11111111;
    }
  }

  set_bit(COLUNA, coluna); 
  return tecla;            	
}

unsigned char converterTeclaPrecionada(unsigned char tecla) {
  return tecla <= '9' ? tecla - '0' : 10 + tecla - 'A';
}  

void adicionarDigito(unsigned char tecla){
  	unsigned char teclaPrecionada = converterTeclaPrecionada(tecla);
	digitos[totalDeDigitos++] = pgm_read_byte(&Tabela[teclaPrecionada]);
}

void mudarColunaDeLeituraDoTeclado(){
  if (millis() > (ultimaVerificacaoDeColuna + 1)) {
    coluna = (coluna + 1) % 4;
    ultimaVerificacaoDeColuna = millis();
  }
}


/**********************************************************************
                          FUNÇÕES DO DISPLAY
***********************************************************************/
void ligarDisplay(int d1, int d2, int d3, int mp = 0){
  if( d1 == 0 ) PORTB |= (1 << PB3);
  if( d2 == 0 ) PORTB |= (1 << PB2);
  if( d3 == 0 ) PORTB |= (1 << PB1);
  if( mp == 2 ) PORTB |= (1 << PB4); // display como catodo comun
  
  if( d1 == 1 ) PORTB &= ~(1 << PB3);
  if( d2 == 1 ) PORTB &= ~(1 << PB2);
  if( d3 == 1 ) PORTB &= ~(1 << PB1);
  if( mp == 1 ) PORTB &= ~(1 << PB4); // display como anodo comum	
}

void atualizarDisplay() {
  if ((millis() - ultimaAtualizacaoDisplay) >= 10) {
    ultimaAtualizacaoDisplay = millis();
    displayAtivo = (displayAtivo + 1) % TOTAL_DE_DISPLAYS; 

    switch(displayAtivo) {
      case 0: ligarDisplay(1, 0, 0, 1);
      break;
      
      case 1: ligarDisplay(0, 1, 0);
      break;
      
      case 2: ligarDisplay(0, 0, 1);
      break;
      
      case 3: ligarDisplay(1, 0, 0, 2);
      break;
      
      case 4: ligarDisplay(0, 1, 0);
      break;
      
      case 5: ligarDisplay(0, 0, 1);
      break;
    }
  }

  PORTC = digitos[displayAtivo]; 	
  // Coloca 0 no pino PB0
  PORTB &= ~(1 << PB0);		
  // verifica se o 6° bit é 0 ou 1 (1 == acende traço do meio)
  PORTB |= 0x01 & (digitos[displayAtivo] >> 6); 
}	

void resetarDisplay() {
    totalDeDigitos = 0;
    for( int i=0 ; i<6 ; i++){
    	digitos[i] = 0xFF;
  	}
}

/**********************************************************************
                          FUNÇÕES DO COFRE
***********************************************************************/
void abrirCofre() {
  ACENDER_LED;
  cofreAberto = true;
}

void fecharCofre() {
  APAGAR_LED;
  cofreAberto = false;
}

/**********************************************************************
                          FUNÇÕES DA SENHA
***********************************************************************/
void resetarSenha(){
  for( int i=0 ; i<6 ; i++){
    senha[1] = 0xFF;
  }
}

bool compararSenhas() {
  return !memcmp(senha, digitos, sizeof(unsigned char) * 6);
}

void salvarSenha() {
  for( int i=0 ; i<6 ; i++){
    senha[i] = digitos[i];
  }
}

/**********************************************************************
                           	   SETUP
***********************************************************************/
void setup() {
  DDRB = 0xFF; // PORTB como saída
  DDRD = 0x0F; // PORTD como entrada
  PORTD= 0xFF; // Habilita os pull-ups do PORTD e coloca colunas em 1

  DDRC = 0xFF; 	// PORTC como saída
  PORTC = 0xFF;	// Habilita os pull-ups do PORTC
  PORTB |= 0x01;// Habilita pull-ups no PB0

  ultimaVerificacaoDeColuna = millis();
  abrirCofre();
}

/**********************************************************************
                           	 	LOOP
***********************************************************************/
void loop(){

  atualizarDisplay();
  unsigned char teclaPrecionada = lerTeclado();

  if (teclaPrecionada != NENHUMA) {
	  adicionarDigito(teclaPrecionada);

      if( totalDeDigitos > 5 ) {
          if( cofreAberto ){
              salvarSenha();
              fecharCofre();

          }else {
              bool senhaEstaCorreta = compararSenhas();

              if( senhaEstaCorreta ){
                  abrirCofre();
                  resetarSenha();
              }
          }

          resetarDisplay();
      }

  }

  mudarColunaDeLeituraDoTeclado();
  delay(1);
} 