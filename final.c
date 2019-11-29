#define F_CPU 1000000UL // El MCU opera a 1 MHz
#include "LCD.c" // Funciones del LCD
#include <avr/io.h>
#include <util/delay.h> // Para los retardos
#include <avr/interrupt.h> // Para el manejo de interrupciones
#include <avr/eeprom.h> // Para la EEPROM

EEMEM unsigned char clave_inicial[4] = { 1, 2, 3, 4}; // Arreglo de 4 bytes

volatile unsigned char tiempo; // Para los intervalos de tiempo
volatile unsigned char FIN_TIEMPO;// Bandera de fin de periodo

ISR (TIMER1_COMPA_vect) { // ISR del temporizador 1
	tiempo --;
	if( tiempo == 0)
	FIN_TIEMPO = 1;
}

char teclado() {
	unsigned char secuencia[] = {0xFE, 0xFD, 0xFB, 0xF7 };
	unsigned char i, renglon, dato;
	for(renglon = 0, i = 0; i < 4; i++) {
		PORTB = secuencia[i]; // Ubica la salida
		asm("nop"); // Espera que las seales se
		// estabilicen
		dato = PINB & 0xF0; // Lee la entrada (anula la
		// parte baja)
		if( dato != 0xF0 ) { // Si se presion una tecla
			_delay_ms(200); // Evita rebotes
			switch(dato) { // Revisa las columnas
				case 0xE0: return renglon;
				case 0xD0: return renglon + 1;
				case 0xB0: return renglon + 2;
				case 0x70: return renglon + 3;
			}
		}
		renglon += 4; // Revisa el siguiente
		// rengln
	}
	return 0xFF; // No hubo tecla presionada
}

int main(void) // Programa Principal
{
	unsigned char tecla, i; //valor de la tecla presionada y contador
	unsigned char clave[4]; // Clave de la chapa, en SRAM
	unsigned char clave_in[4]; // Clave de entrada
	unsigned char CLAVE_LISTA, CAMBIO_CLAVE; // Banderas
	// Configuracion de los puertos
	DDRB = 0x0F; // Entrada y salida, para el teclado
	PORTB = 0xF0; // Resistores de Pull-Up en las entradas
	DDRC = 0xFF; // Salida para el LCD
	DDRD = 0x0F; // Salidas para los LEDs de estado y el electroimn
	// Inicializa al LCD
	LCD_reset();
	// Obtiene la clave de EEPROM y la ubica en SRAM
	for( i = 0; i < 4; i++)
	clave[i] = eeprom_read_byte(i);
	// Configuracin parcial del temporizador 1
	TIMSK = 0x10; // El temporizador 1 interrumpe
	OCR1A = 62499; // cada medio segundo
	TCCR1A = 0x00; // pero an no arranca (TCCR1B an tiene 0x00)
	sei(); // Habilitador global de interrupciones
	while( 1 ) { // Inicia el lazo infinito
		// Estado inicial
		TCCR1B = 0x00; // Temporizador 1 detenido
		PORTD = 0x00; // Salidas apagadas
		LCD_clear();
		LCD_write_cad("Indique la Clave", 16); // Mensaje inicial
		// Espera tecla numrica
		do {
			tecla = teclado();
		} while( tecla < 0 || tecla > 9 );
		// Se presion una tecla numrica
		clave_in[0] = tecla; // Primer dgito de la clave recibido
		i = 1;
		LCD_clear();
		LCD_cursor(0x04); // Ubica al cursor e
		LCD_write_data('*'); // imprime un asterisco
		PORTD = 0x01; // Enciende LED naranja (hay actividad)
		// Recibe la clave
		TCNT1 = 0; // Asegura que el temporizador 1 est en 0
		TCCR1B = 0x0A; // Arranca al temporizador 1
		tiempo = 20; // con un periodo de 10 segundos
		FIN_TIEMPO = 0; // Banderas apagadas
		CLAVE_LISTA = 0;
		do {
			tecla = teclado(); // Sondea al teclado
			if( tecla != 0xFF) {
				if( tecla >= 0 && tecla <= 9 ) { // Tecla numrica
					clave_in[i] = tecla; // guarda el valor de la tecla
					LCD_write_data('*');
					i++;
					if( i == 4 )
					CLAVE_LISTA = 1; // Se han introducido 4 dgitos
				}
				else if(tecla == 0x0A) { // Tecla de borrado
					if( i > 0 ) {
						LCD_cursor(0x03 + i); // Si hay datos, se
						// borra un *
						LCD_write_data(' ');
						LCD_cursor(0x03 + i);
						i--;
					}
				}
			}
		} while( ! ( FIN_TIEMPO || CLAVE_LISTA));
		// El ciclo termina si la clave est completa o si termin el tiempo disponible
		TCCR1B = 0x00; // Detiene al temporizador 1
		if(CLAVE_LISTA) { // Contina si se introdujo la clave completa
			// Compara la clave
			if( clave[0]==clave_in[0] && clave[1]==clave_in[1] &&
			clave[2]==clave_in[2] && clave[3]==clave_in[3] ){
				PORTD = 0x0A; // Clave correcta: Abre la chapa
				LCD_clear(); // (LED verde y electroimn encendidos)
				LCD_write_cad("<< Bienvenido >>", 16);
				TCNT1 = 0; // Por un tiempo de 3 segundos muestra el
				TCCR1B = 0x0A; // estado de abierto y sondea
				tiempo = 6; // si se pide el cambio de clave
				FIN_TIEMPO = 0;
				CAMBIO_CLAVE = 0;
				do {
					tecla = teclado();
					if(tecla == 0x0B) { // Se solicit el Cambio de clave
						PORTD = 0x02; // LED verde encendido
						LCD_clear();
						LCD_write_cad("Cambio de Clave", 15);
						CAMBIO_CLAVE = 1;
					}
				} while( ! ( FIN_TIEMPO || CAMBIO_CLAVE));
				TCCR1B = 0x00; // Detiene al temporizador 1
				if( CAMBIO_CLAVE ) {
					i = 0;
					TCNT1 = 0; // Se tienen 10 segundos para
					TCCR1B = 0x0A; // introducir la nueva clave
					tiempo = 20;
					FIN_TIEMPO = 0;
					CLAVE_LISTA = 0;
					do { // En este ciclo se lee la nueva
						tecla = teclado(); // clave o espera por 10 segundos
						if( tecla != 0xFF) {
							if( tecla >= 0 && tecla <= 9 ) { // Tecla numrica
								clave_in[i] = tecla;
								if ( i == 0 ) { // Con el primer
									//dgito se ubica
									LCD_clear(); // al cursor
									LCD_cursor(0x04);
									PORTD = 0x01; // Enciende LED naranja
								} // (hay actividad)
								LCD_write_data(tecla + 0x30); // Escribe al
								// dgito
								i++; // lo cuenta
								if( i == 4 )// Con 4 dgitos, la clave est lista
								CLAVE_LISTA = 1;
							}
							else if(tecla == 0x0A) { // Tecla de borrado
								if( i > 0 ) { // Si hay datos, no considera
									LCD_cursor(0x03 + i);// al ltimo dgito
									LCD_write_data(' ');
									LCD_cursor(0x03 + i);
									i--;
								}
							}
						}
					} while( ! ( FIN_TIEMPO || CLAVE_LISTA));
					TCCR1B = 0x00; // Detiene al temporizador 1
					if(CLAVE_LISTA) {
						for( i = 0; i < 4; i++) {
							// Respalda en EEPROM
							eeprom_write_byte(i, clave_in[i]);
							clave[i] = clave_in[i]; // y la deja en RAM
						} // para su evaluacin
						TCNT1 = 0; // Durante 3 segundos
						// se muestra
						TCCR1B = 0x0A; // el mensaje de que se
						// ha aceptado
						tiempo = 6; // la nueva clave
						FIN_TIEMPO = 0;
						PORTD = 0x02; // con el LED verde encendido
						LCD_clear();
						LCD_write_cad(" CLAVE ACEPTADA", 16);
						while( !FIN_TIEMPO );
						TCCR1B = 0x00;
					} // Fin if de nueva clave
				} // Fin if de cambio de clave
			} // Fin if de clave correcta
			else {
				TCNT1 = 0; // Durante 3 segundos se muestra el mensaje
				TCCR1B = 0x0A; // de que la clave es incorrecta
				tiempo = 6; // con el LED rojo encendido
				FIN_TIEMPO = 0;
				PORTD = 0x04;
				LCD_clear();
				LCD_write_cad("CLAVE INCORRECTA", 16);
				while( !FIN_TIEMPO );
				TCCR1B = 0x00;
			} // Fin else de clave incorrecta
		} // Fin if de introduccin de clave completa
	} // Fin del lazo infinito
} // Fin del programa principal
