/*
	Academics:
    	Diogo Rodrigues dos Santos - 2380242
        Gustavo Zanzin Guerreiro Martins - 2349370
        
    Description:
    	This project is a simulation of an automatic accelerator.
*/

unsigned char d = 0; // define what display is active
unsigned long lastDispRefresh = 0, lastSerialRefresh = 0;

// Tabela = {0,1,2,3,4,5,6,7,8,9,E,r}
unsigned char Tabela[] = {0x40, 0x79, 0x24, 0x30, 0x19, 0x12, 0x02, 0x78, 0x80, 0x18, 0x06, 0x2F};
unsigned char todisp[3] = {10, 11, 11};

char ch = 1;

int sensorValue[2] = {0,0}; // sensors values read
int sensorValueMaped[2] = {0,0}; // sensor value after maped

int diferenceSensorsValues = 0; 	// storage diference between sensors values
int avaregeSensorsValues = 0;	// avarege between sensors

void setValuesToDisplay(){
  // if res is a diference accepted, then calculate avarege
  if (diferenceSensorsValues < 10 && diferenceSensorsValues > -10)
  {
    // calculate avarege between value1 nd value2
  	avaregeSensorsValues = (sensorValueMaped[0] + sensorValueMaped[1])/ 2; // média nao ta funcionando n sei pq
  	
    // updating the number that will be displayed
    todisp[0] = avaregeSensorsValues / 100;    
    todisp[1] = (avaregeSensorsValues % 100) /10 ;    	 	
  	todisp[2] = avaregeSensorsValues % 10; 	
  }
  else {
  	// if res is unacceptable then error is displayed           	
    todisp[0] =10;    	
    todisp[1] = 11;    	 	
  	todisp[2] = 11; 
   	
    avaregeSensorsValues = 0;
  }
  
}

void setup()
{
  DDRD |= 0b01111111; // define POTD as output
  DDRB |= 0b00000111; // define PB1, PB2 e PB3 as output
  
  DDRC &= ~(1<<PC0); // PC0 as input
  DDRC &= ~(1<<PC1); // PC1 as input
  
  ADMUX &= ~((1<<REFS0)|(1<<REFS1)); // tensao de referencia
  ADMUX |= (1<<REFS0); // AVCC
  
  ADCSRB = 0; // valor padrão
  
  ADCSRA &= 0b11111000;
  ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0); // ADCCLK = CLK/128
  
  ADCSRA |= (1<<ADIE); // habilita interrupção
  ADCSRA |= (1<<ADEN); // habilita o ADC
  
  ADMUX &= 0b11110000;
  ADMUX |= (0b00001111&ch); // seleciona o canal ch no MUX
  DIDR0 = (1<<ch);
  
  // start first convertion
  ADCSRA |= (1<<ADSC); 
}

void loop()
{
  // map it to the range of the analog out:
  sensorValueMaped[0] = map(sensorValue[0], 190, 833, 0, 100);  
  sensorValueMaped[1] = map(sensorValue[1], 87, 413, 0, 100);
  
  // update display
  if (millis()>(lastDispRefresh+0)) {
    lastDispRefresh = millis();
  	d++;
    d%=3;
    
    PORTB = (PORTB&0b11111000)|(1<<d); // ativa o display correspondente ao d
	PORTD = Tabela[todisp[d]];
  }
  
  // diference between sensorValue[0] e sensorValue[1]
  diferenceSensorsValues = sensorValueMaped[0] - sensorValueMaped[1];
	
  setValuesToDisplay();
}

// interrupt handler function
ISR(ADC_vect)
{
  // read sensor value
  sensorValue[ch] = ADC;
  
  // change to the next channel
  ch += 1;
  ch %= 2;
  
  ADMUX &= 0b11110000;
  ADMUX |= (0b00001111&ch); // select the channel ch no MUX
  DIDR0 = (1<<ch);
  
  // start convertion ADC
  ADCSRA |= (1<<ADSC); 
}
