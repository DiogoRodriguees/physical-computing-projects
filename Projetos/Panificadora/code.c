/******************************************************************
* 						    Bibliotecas 						  *
*******************************************************************/
#include <stdio.h>
#include <LiquidCrystal.h>


/******************************************************************
* 						      Defines 						       *
*******************************************************************/				
// Estado da Panificadora
#define DESLIGADA 0
#define ESCOLHENDO_TEMPO_MISTURA 1
#define ESCOLHENDO_TEMPO_DESCANSO  2
#define ESCOLHENDO_TEMPO_ASSADURA 3
#define MISTURANDO 4
#define DESCANSANDO 5
#define ASSANDO 6
#define PAO_ASSADO 7
#define LIGADA 8

// Para controle do tempo
#define TEMPO 60
#define TEMPO_INICIAL 0

// Para configurar o motor e resistencia
#define LIGAR_MOTOR PORTD |= (1 << PD3)
#define DESLIGAR_MOTOR PORTD &= ~(1 << PD3)
#define DESLIGAR_RESISTENCIA PORTD &= ~(1 << PD2)
#define LIGAR_RESISTENCIA PORTD |= (1 << PD2)

// Para configurar o LCD
#define DADOS_LCD PORTD   	// 4 bits de dados do LCD no PORTD 
#define nibble_dados  1  
#define CONTR_LCD PORTB   	// PORT com os pinos de controle do LCD (pino R/W em 0).
#define E         PB1     	// pino de habilitação do LCD (enable)
#define RS        PB0     	// pino para informar se o dado é uma instrução ou caractere
#define pulso_enable()  _delay_us(1); CONTR_LCD|=(1<<E); _delay_us(1); CONTR_LCD&=~(1<<E); _delay_us(45)

// Tempos padrões
#define TEMPO_MISTURA 1500
#define TEMPO_DESCANSO 5400
#define TEMPO_ASSANDO 2400


/******************************************************************
* 						     Variaveis 						      *
*******************************************************************/
long click_aumentar = 0;          
long click_diminuir = 0;         
long click_concluir = 0; 

char last_aumentar = (1 << PB2); 
char last_diminuir = (1 << PB3); 
char last_concluir = (1 << PB4); 

long ultima_mistura = 0;
long ultimo_descanso = 0;
long assando = 0;

char btn_aumentar;
char btn_diminuir;
char btn_concluir;

int tempo_de_mistura = TEMPO_MISTURA;
int tempo_de_descanso = TEMPO_DESCANSO;
int tempo_assando = TEMPO_ASSANDO;
int temperatura = 152;

void cmd_LCD(unsigned char c, char cd); 
volatile long long last_click = 0;
void mostrar_tempo(int tempo, bool exibir_graus);

/******************************************************************
* 						Struct Panificadora 					   *
*******************************************************************/
typedef struct Panificadora {
	int estado_atual;
}Panificadora;

Panificadora panificadora; // definida globalmente pra não precisar passar por parametro



/******************************************************************
 					  FUNÇÕES DA PANIFICADORA
*******************************************************************/
void misturar(){
  if((millis() - ultima_mistura) > 1000){
    ultima_mistura = millis();
    mostrar_tempo(tempo_de_mistura, true);
  	tempo_de_mistura -= TEMPO; 
    if(tempo_de_mistura < 0) atualizarEstado(DESCANSANDO);
  }
}

void descansar(){
  if((millis() - ultimo_descanso) > 1000){
    ultimo_descanso = millis();
    mostrar_tempo(tempo_de_descanso, true);
    tempo_de_descanso -= TEMPO; 
    if(tempo_de_descanso < 0)atualizarEstado(ASSANDO);
  	
  }
}

void assar(){
  if((millis() - assando) > 1000){
    assando = millis();
    mostrar_tempo(tempo_assando, true);
    tempo_assando -= TEMPO;
    if(tempo_assando < 0) atualizarEstado(PAO_ASSADO);
  }
}

void aguardarIntrucao() {
  bool btn_concluir_clicado = btn_concluir != last_concluir && btn_concluir == 0 && (millis() - click_concluir) > 10;
  
  if(btn_concluir_clicado){
    click_concluir = millis();
    atualizarEstado(ESCOLHENDO_TEMPO_MISTURA);
    mostrar_tempo(tempo_de_mistura, false);
  } 
}

char* converterEstadoAtualParaString(){
	switch(panificadora.estado_atual){
  	case ESCOLHENDO_TEMPO_MISTURA:
    	return "Tempo Mistura";
    break;
    
    case ESCOLHENDO_TEMPO_DESCANSO:
    	return "Tempo Descanso"; 
    break;
      
    case ESCOLHENDO_TEMPO_ASSADURA:
    	return "Tempo Assadura"; 
    break;
    
    case MISTURANDO: return "Misturando";
    break;
    
    case DESCANSANDO:return "Crescendo";
    break;
    
    case ASSANDO:return "Assando";
    break;
    
    case PAO_ASSADO: return "Pao assado";
    break;
    
    default:
    break;
  }
}


void atualizarEstado(int novo_estado) {
  panificadora.estado_atual = novo_estado;
  if( novo_estado == MISTURANDO ) {
  	LIGAR_MOTOR;
    DESLIGAR_RESISTENCIA;
    temperatura = 0;
  }
  if( novo_estado == ASSANDO ) {
  	DESLIGAR_MOTOR;
    LIGAR_RESISTENCIA;
    temperatura = 127;
  }
  if(  novo_estado == DESCANSANDO ) {
  	DESLIGAR_MOTOR;
    DESLIGAR_RESISTENCIA;
    temperatura = 0;
  }
  if( novo_estado == DESLIGADA ) {
  	DESLIGAR_MOTOR;
    DESLIGAR_RESISTENCIA;
  }
}

/******************************************************************
 					     FUNÇÕES GENERICA
*******************************************************************/
int setTempo(int tempo, int tempo_prox_etapa, int proxima_etapa) {
  bool btn_aumentar_clicado = btn_aumentar != last_aumentar && btn_aumentar == 0 && (millis() - click_aumentar) > 10; 
  bool btn_diminuir_clicado = btn_diminuir != last_diminuir && btn_diminuir == 0 && (millis() - click_diminuir) > 10;
  bool btn_concluir_clicado = btn_concluir != last_concluir && btn_concluir == 0 && (millis() - click_concluir) > 10;
  
  // Aumenta o tempo
  if(btn_aumentar_clicado){
    click_aumentar = millis();
    tempo += TEMPO;
    cmd_LCD(0x01,0);
    cmd_LCD(0xC0,0);
    mostrar_tempo(tempo, false);
  }

  // Diminui o tempo
  if(btn_diminuir_clicado){
    click_diminuir = millis();
    if( tempo > 0) tempo -= TEMPO;
    cmd_LCD(0x01,0);
	cmd_LCD(0xC0,0);
    mostrar_tempo(tempo, false);
  }

  // Conclui etapa de escolher tempo de mistura]
  if(btn_concluir_clicado){
    click_concluir = millis();
    atualizarEstado(proxima_etapa);
  	mostrar_tempo(tempo_prox_etapa, false);    
  	return tempo;
  }
  
  return tempo;
}

/******************************************************************
 					   FUNÇÕES DO DISPLAY LCD
*******************************************************************/
void cmd_LCD(unsigned char c, char cd){
  if(cd==0)
    CONTR_LCD&=~(1<<RS);
  else
    CONTR_LCD|=(1<<RS);

  //primeiro nibble de dados - 4 MSB
  #if (nibble_dados)                //compila código para os pinos de dados do LCD nos 4 MSB do PORT
    DADOS_LCD = (DADOS_LCD & 0b00001111) | (0b11110000 & c);
  #else                     //compila código para os pinos de dados do LCD nos 4 LSB do PORT
    DADOS_LCD = (DADOS_LCD & 0xF0)|(c>>4);  
  #endif
  
  pulso_enable();

  //segundo nibble de dados - 4 LSB
  #if (nibble_dados)                //compila código para os pinos de dados do LCD nos 4 MSB do PORT 
    DADOS_LCD = (DADOS_LCD & 0b00001111) | (0b11110000 & (c<<4));
  #else                     //compila código para os pinos de dados do LCD nos 4 LSB do PORT
    DADOS_LCD = (DADOS_LCD & 0xF0) | (0x0F & c);
  #endif
  
  pulso_enable();
  
  if((cd==0) && (c<4))        //se for instrução de retorno ou limpeza espera LCD estar pronto
    _delay_ms(2);
}

void iniciar_display(){             
  CONTR_LCD&=~(1<<RS); 
  CONTR_LCD&=~(1<<E); 
  _delay_ms(20);       
  
  cmd_LCD(0x30,0);
              
  pulso_enable();     //habilitação respeitando os tempos de resposta do LCD
  _delay_ms(5);   
  pulso_enable();
  _delay_us(200);
  pulso_enable(); 
  
  cmd_LCD(0x20,0);
  pulso_enable();   
  cmd_LCD(0x28,0);    
  cmd_LCD(0x08,0);    //desliga o display
  cmd_LCD(0x01,0);    //limpa todo o display
  cmd_LCD(0x0C,0);    //mensagem aparente cursor inativo não piscando   
  cmd_LCD(0x80,0);    //inicializa cursor na primeira posição a esquerda - 1a linha
}

void escreve_LCD(char *c){
   for (; *c!=0;c++) cmd_LCD(*c,1);
}

void mostrarGraus(int temperatura) {
  int centena = temperatura / 100;
  int dezena = (temperatura % 100) / 10;
  int unidade = temperatura % 10;
  cmd_LCD(0x20, 1);
  
  if( temperatura > 99 ) cmd_LCD((0x30 + centena), 1);
  if( temperatura > 10 ) cmd_LCD((0x30 + dezena), 1);
  cmd_LCD((0x30 + unidade), 1);
  cmd_LCD('°', 1);
  cmd_LCD(0x43, 1);
}

void mostrar_tempo(int tempo, bool exibir_graus){
  int segundos = (tempo  % 3600) % 60;
  int minutos = (tempo  % 3600)/60;
  int horas = tempo / 3600;
  
  cmd_LCD(0x01, 0);
  cmd_LCD(0x80, 0); 
  escreve_LCD(converterEstadoAtualParaString());
  if(exibir_graus){
	mostrarGraus(temperatura);
  }
  
  cmd_LCD(0xC0, 0);
  cmd_LCD((0x30 + (horas / 10)), 1);  // Horas
  cmd_LCD((0x30 + (horas % 10)), 1);  			
  
  cmd_LCD(0x3a, 1);
  cmd_LCD((0x30 + (minutos / 10)), 1); // Minutos
  cmd_LCD((0x30 + (minutos % 10)), 1);
  
  cmd_LCD(0x3a, 1);
  cmd_LCD((0x30 + (segundos / 10)), 1);
  cmd_LCD((0x30 + (segundos % 10)), 1);// Segundos
}

/******************************************************************
 							  SETUP 							  
*******************************************************************/
void setup(){
  Serial.begin(57600);
	
  DDRC |= 0b00000000;
  DDRD |= 0b11111100;
  DDRB |= 0b00000011;		
  PORTB |= (1<<PB2);
  PORTB |= (1<<PB3);
  PORTB |= (1<<PB4);
  
  iniciar_display();
  atualizarEstado(LIGADA);
  
  cmd_LCD(0x80,0);
  escreve_LCD("Panificadora"); 
  cmd_LCD(0xC0, 0);
  escreve_LCD("Pressione OK");
}

/******************************************************************
 							   LOOP 							  
*******************************************************************/
void loop(){
  btn_aumentar = PINB & (1 << PB2);
  btn_diminuir = PINB & (1 << PB3);
  btn_concluir = PINB & (1 << PB4);
  
  switch(panificadora.estado_atual){
    case LIGADA: aguardarIntrucao();
    break;
    
  	case ESCOLHENDO_TEMPO_MISTURA:
    	tempo_de_mistura = setTempo(tempo_de_mistura, tempo_de_descanso, ESCOLHENDO_TEMPO_DESCANSO);
    break;
    
    case ESCOLHENDO_TEMPO_DESCANSO:
    	tempo_de_descanso = setTempo(tempo_de_descanso, tempo_assando, ESCOLHENDO_TEMPO_ASSADURA);
    break;
   
   case ESCOLHENDO_TEMPO_ASSADURA:
    	tempo_assando = setTempo(tempo_assando, tempo_de_mistura, MISTURANDO);
    break;
    
    case MISTURANDO: misturar();
    break;
    
    case DESCANSANDO: descansar();
    break;
    
    case ASSANDO: assar();
    break;
    
    case PAO_ASSADO: 
     	cmd_LCD(0x01,0);
    	cmd_LCD(0x80,0);
    	escreve_LCD("PAO ASSADO");
    	atualizarEstado(DESLIGADA);
    break;
    
    default:
    break;
  }
  
  last_aumentar = btn_aumentar;
  last_diminuir = btn_diminuir;
  last_concluir = btn_concluir;
}