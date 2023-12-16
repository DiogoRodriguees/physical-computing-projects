#include <avr/pgmspace.h>	//biblioteca para poder gravar dados na memória flash 


unsigned long clique = 0;		
unsigned long lastupdate = 0;
unsigned long aux = 0;
unsigned long time = 0;

int start = 0;

char last_state = (1 << PB0);

int d0 = 0;
int d1 = 0;
int d2 = 0;
int d3 = 0;

char d = 0;
int saida;

char leitura;
char clickStart;
char lastStartState;

// valor total do timer de acordo com os "giros" do encoder
long int count = 0;

//variável gravada na memória flash
const unsigned char Tabela[] PROGMEM = {0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78, 0x00, 0x18, 0x08, 0x03, 0x46, 0x21, 0x06, 0x0E};

/**********************************************************
*		   		 	 Funções Auxiliares		   		 	  *
***********************************************************/
// Atualiza o número/valor que será exibido no display
void updateDisplay()
{
    d0 = count % 10;
    d1 = (count / 10) % 6;

    d2 = (count / 60) % 10;
    d3 = count / 600;
}


// Indica qual display deve ser atualizado (da direita para esquerda)
void updateOutput()
{
  // Atualiza o primeiro display
  if (d == 0)
    {
      saida = d0;			// Define o índice para Tabela com base em d0
      PORTB |= (1 << PB2);	// Liga o segmento 'b' do dígito
      PORTB |= (1 << PB3);	// Liga o segmento 'c' do dígito
      PORTB |= (1 << PB4);	// Liga o segmento 'd' do dígito
      PORTB &= ~(1 << PB1);	// Desliga o segmento 'a' do dígito
    }
  	// Atualiza o segundo display
    else if (d == 1)
    {
      saida = d1;
      PORTB |= (1 << PB1);
      PORTB |= (1 << PB3);
      PORTB |= (1 << PB4);
      PORTB &= ~(1 << PB2);
    }
  	// Atualiza o terceiro display
    else if (d == 2)
    {
      saida = d2;
      PORTB |= (1 << PB1);
      PORTB |= (1 << PB2);
      PORTB |= (1 << PB4);
      PORTB &= ~(1 << PB3);
    }
  	// Atualiza o quarto display
    else if (d == 3)
    {
      saida = d3;
      PORTB |= (1 << PB1);
      PORTB |= (1 << PB2);
      PORTB |= (1 << PB3);
      PORTB &= ~(1 << PB4);
    }
 
}


/**********************************************************
*		   		 	 Funções Principais		   		 	  *
***********************************************************/
void setup()
{
	DDRB = 0b11111110;			// PB0 como pino de entrada, os demais pinos como saída
	PORTB= 0x01;				// habilita o pull-up do PB0
	DDRD = 0xFF;				// PORTD como saída (display)
  	DDRC = 0b11111111;
	PORTD= 0xFF;				// desliga o display
	UCSR0B = 0x00;				// PD0 e PD1 como I/O genérico, para uso no Arduino
	pinMode(A4, INPUT_PULLUP);	// configura A0 como entrada
  	pinMode(A5, INPUT_PULLUP);	// configura A1 como entrada
}


void loop()
{
  leitura = PINB & (1<<PB0);	// Leitura do pino PB0 (botão incremento/decremento do timer count)
  clickStart = analogRead(PC4);	// Leitura do pino PC4 (botão início contagem regressiva)
  
  
  // Atualiza o display
  if ((millis()-lastupdate) >= 20) {
      lastupdate = millis();
      d = (d + 1) % 4;

      updateOutput();

      PORTC = ~(pgm_read_byte(&Tabela[saida]) >> 4);
      PORTD = ~(pgm_read_byte(&Tabela[saida]) << 4);
  }

  // Verifica se a contagem regressiva começou
  if(start)
  {
        // Se passou mais de 500 milis desde a última vez que o led e o buzzer foram desligados
    	if((millis() - aux) > 500)
        {
          digitalWrite(PD3, 0); // Desliga o led
          digitalWrite(PD0, 1); // Desliga o buzzer
        }
        
      	// Atualiza o tempo que será exibido no display
    	if((millis()-time)>=1000 && count > 0)
        {
            time = millis();
            count--;
            updateDisplay();	
          	digitalWrite(PD3, 0); // pisca o led
          	aux = millis();
      	} 
    	// Se a contagem regressiva chegou em 00:00
    	else if(count == 0)
        {
      		start = 0;
          	digitalWrite(PD0, 0); // Aciona o buzzer
      		aux = millis();
      	}
  
  // Contagem regressiva ainda não foi iniciada
  } else {
    
    	// Incrementa ou decrementa mais rapidamente
        if (leitura!=last_state && leitura == 0 && (millis()-clique) < 500)
        {
        	clique = millis();
          
           	// Leitura do switch que simula a "direção do giro" para alterar contador
           	count = analogRead(PC5) == 0 ? count + 30 : count - 30;
          
          	// Volta o timer para 00:00 quando em 59:59 se incrementado e vice-versa
            count = count < 0 ? 3599 : count;
            count = count > 3599 ? 0 : count;

            updateDisplay();
    
          
    	// Incrementa ou decrementa o tempo unitariamente (de 1 em 1)
        } else if (leitura!=last_state && leitura == 0 && (millis()-clique)>1)
        {
            clique = millis();

            // Leitura do switch que simula a "direção do giro" para alterar contador
			count = analogRead(PC5) == 0 ? count + 1 : count - 1;            

          	// Volta o timer para 00:00 quando em 59:59 se incrementado e vice-versa
            count = count < 0 ? 3599 : count;
            count = count > 3599 ? 0 : count;

            updateDisplay();
        }

    	last_state = leitura;
    
    	// Inicia a contagem regressiva (debounce botão no PD3)
        if(clickStart!= lastStartState && (millis() - clique)> 1){
            clique = millis();
           
            if(clickStart == 0){
                start = 1;
                digitalWrite(PD3, 0);
    
            }
        }
  }

  lastStartState = clickStart;
  _delay_ms(1);
}
