int main() {
   // Configuracion de entradas y salidas
   DDRB = 0b00001111; // Salidas: AM, PM, Zumbador y 2 puntos
   DDRC = 0xFF; // Salida a los 7 segmentos
   DDRD = 0xF0; // Botones e interruptor y salidas a
   
   //transistores
   PORTD = 0x0F; // Pull-Up en las entradas
   
   //Configuracion del temporizador 2
   TCCR2 = 0x04; // Para que desborde cada medio segundo
   TIMSK = 0x40; // Interrupcion por desbordamiento
   ASSR = 0x08; // Se usa al oscilador externo
   sei(); // Habilitador global de interrupciones
   
   // Configura interrupciones externas
   MCUCR = 0B00001010; // INT1/INT0 por flanco de bajada (sin
   // habilitar)
   
   // Estado inicial del sistema: variables, banderas y salidas
   h_act = 12; m_act = 0; s_act = 0; // La hora actual inicia con 12:00
   h_alrm = 12; m_alrm = 0; // La hora de la alarma es 12:00
   modo = 1; // Modo normal, muestra la hora
   AM_F = 1; AM_F_A = 1; // Antes del medio doa
   ALARM = 0; CAMBIO = 0; // Alarma apagada
   PORTB = 0x02; // Solo activa la salida AM

   while(1) { // Lazo infinito
      mostrar(); // Exhibe, segon el modo

      // Modo es una variable global
      if( ALARM == 1 && (PIND & 0x01) ) {
         ALARM = 0; // Apaga la alarma si esto encendida e
         PORTB = PORTB & 0b11110111;// inhabilitada
      }

      if( modo_presionado() ) {
         modo++;
         if( modo == 3 )
            GICR = 0B11000000; // Habilita INT1 e INT0
         else if( modo == 5 ) {
            if( AM_F_A == 1) {
               PORTB = PORTB & 0b11111011; // Ajusta AM y PM segon la
               PORTB = PORTB | 0b00000010;// bandera AM_F_A de la alarma
            } else {
               PORTB = PORTB & 0b11111101;
               PORTB = PORTB | 0b00000100;
            }
         } else if( modo == 7 ) {
            modo = 1;
            GICR = 0x00; // Inhabilita INT1 e INT0
            if( AM_F == 1) {
               PORTB = PORTB & 0b11111011; // Ajusta AM y PM segon la
               PORTB = PORTB | 0b00000010; // bandera AM_F de la hora actual
            } else {
               PORTB = PORTB & 0b11111101;
               PORTB = PORTB | 0b00000100;
            }
         }
      } // Cierre del if de modo presionado
   } // Cierre del lazo infinito
} // Fin de la funcion principal

void mostrar() { // Funcion para el despliegue de datos
   unsigned char i;
   ubica(); // Coloca la informacion a mostrar
   for( i = 0; i < 4; i++) {
      PORTC = Disp[i]; // Envoa el dato al puerto
      PORTD = pgm_read_byte(&habs[i]); // Habilita para su despliegue
      _delay_ms(5); // Espera se vea adecuadamente
      PORTD = 0xFF; // Inhabilita para no introducir ruido
   } // en otro display
}

void ubica() { // Ubica los datos a mostrar en un
   arreglo global
   switch(modo) { // Segon sea el modo
   case 1: Disp[3] = pgm_read_byte(&Sg7[h_act/10]);// Hora Actual
           Disp[2] = pgm_read_byte(&Sg7[h_act%10]);
           Disp[1] = pgm_read_byte(&Sg7[m_act/10]);
           Disp[0] = pgm_read_byte(&Sg7[m_act%10]);
           if( CAMBIO ) // Dos puntos parpadeando
              PORTB = PORTB | 0x01;
           else
              PORTB = PORTB & 0xFE;
   break;
   case 2: Disp[3] = 0xFF; // Segundos
           Disp[2] = 0xFF;
           Disp[1] = pgm_read_byte(&Sg7[s_act/10]);
           Disp[0] = pgm_read_byte(&Sg7[s_act%10]);
   break;
   case 3: if( CAMBIO ) { // Hora actual
              Disp[3] = pgm_read_byte(&Sg7[h_act/10]);// parpadeando
              Disp[2] = pgm_read_byte(&Sg7[h_act%10]);
           } else {
              Disp[3] = 0xFF;
              Disp[2] = 0xFF;
           }
           Disp[1] = pgm_read_byte(&Sg7[m_act/10]);// Minutos actuales
           Disp[0] = pgm_read_byte(&Sg7[m_act%10]); // normal
   break;
   case 4: Disp[3] = pgm_read_byte(&Sg7[h_act/10]); //Hora actual normal
           Disp[2] = pgm_read_byte(&Sg7[h_act%10]);
           if( CAMBIO ) { // Minutos actuales
              Disp[1] = pgm_read_byte(&Sg7[m_act/10]);// parpadeando
              Disp[0] = pgm_read_byte(&Sg7[m_act%10]);
           } else {
              Disp[1] = 0xFF;
              Disp[0] = 0xFF;
           }
           break;
   case 5: if( CAMBIO ) { // Hora de la alarma
              Disp[3] = pgm_read_byte(&Sg7[h_alrm/10]);//parpadeando
              Disp[2] = pgm_read_byte(&Sg7[h_alrm%10]);
           }
           else {
              Disp[3] = 0xFF;
              Disp[2] = 0xFF;
           }
           Disp[1] = pgm_read_byte(&Sg7[m_alrm/10]);// Minutos de la
           Disp[0] = pgm_read_byte(&Sg7[m_alrm%10]);// alarma normal
           break;
   case 6: Disp[3] = pgm_read_byte(&Sg7[h_alrm/10]);//Hora de la alarma
           Disp[2] = pgm_read_byte(&Sg7[h_alrm%10]); // normal
           if( CAMBIO ) {
              Disp[1] = pgm_read_byte(&Sg7[m_alrm/10]);//Minutos de la
              Disp[0] = pgm_read_byte(&Sg7[m_alrm%10]);//alarma parpadeando
           } else {
              Disp[1] = 0xFF;
              Disp[0] = 0xFF;
           }
   break;
   }
}

ISR(TIMER2_OVF_vect) { // Se ejecuta cada medio segundo
   if( CAMBIO ) {
      CAMBIO = 0;
      s_act++; // Incrementa los segundos
      if( s_act == 60 ) { // Si son 60 incrementa los minutos
         s_act = 0;
         m_act++;
         if( ALARM ) {
            ALARM = 0; // Si la alarma esto activa, la apaga
            PORTB = PORTB & 0b11110111; // por ser otro minuto
         }
         if(m_act == 60) { // Si son 60 minutos incrementa las horas
            m_act = 0;
            h_act++;
            if( h_act == 13) // Con la hora 13 se regresa a 1
            h_act = 1;
            if( h_act == 12 ) { // Con la hora 12 se ajusta la
               bandera
               if( AM_F == 1 ) { // AM_F y las salidas AM y FM
                  AM_F = 0;
                  PORTB = PORTB & 0b11111101;
                  PORTB = PORTB | 0b00000100;
               } else {
                  AM_F = 1;
                  PORTB = PORTB & 0b11111011;
                  PORTB = PORTB | 0b00000010;
               }
            }
         }
         // En cada nuevo minuto tambion revisa si se debe activar la alarma
         if( !(PIND&0x01) && AM_F==AM_F_A && h_act==h_alrm && m_act==m_alrm ) {
            ALARM = 1;
            PORTB = PORTB | 0b00001000;
         }
      }
   } else // Modifica la bandera si no es el
   // segundo
   CAMBIO = 1; // completo
}

ISR(INT0_vect) { // Atiende al boton UP
switch( modo ) {
   case 3: h_act++; // Incrementa hora actual
      if( h_act == 13 ) // De 13 pasa a 1
         h_act = 1;
      if( h_act == 12 ) { // En 12 ajusta bandera AM_F
         if( AM_F ) { // y salidas AM y PM
            AM_F = 0;
            PORTB = PORTB & 0b11111101;
            PORTB = PORTB | 0b00000100;
         } else {
            AM_F = 1;
            PORTB = PORTB & 0b11111011;
            PORTB = PORTB | 0b00000010;
         }
      }
   break;
   case 4: m_act++; // Incrementa minutos actuales
      if( m_act == 60 ) // De 60 reinicia en 0
         m_act = 0;
      break;
   case 5: h_alrm++; // Incrementa la hora de la alarma
      if( h_alrm == 13 ) // De 13 pasa a 1
         h_alrm = 1;
      if( h_alrm == 12 ) { // En 12 ajusta bandera AM_F_A
         if( AM_F_A ) { // y salidas AM y PM
            AM_F_A = 0;
            PORTB = PORTB & 0b11111101;
            PORTB = PORTB | 0b00000100;
         } else {
            AM_F_A = 1;
            PORTB = PORTB & 0b11111011;
            PORTB = PORTB | 0b00000010;
         }
      }
   break;
   case 6: m_alrm++; // Incrementa minutos de la alarma
      if( m_alrm == 60 ) // De 60 reinicia en 0
         m_alrm = 0;
   break;
   }
}
