// TAREA 2 - SISTEMAS EMBEBIDOS
// ESTUDIANTE: KAREN DANIELA MIRANDA BRITO
// TEMA: INICIO DE JUEGO SNAKE

#include <avr/io.h>
#include <util/delay.h>

#define TAMANO_SNAKE 48

// ARREGLO PALABRA SNAKE
unsigned char SECUENCIA_SNAKE[TAMANO_SNAKE] = {
  0x00, 0x46, 0x4A, 0x4A, 0x4A, 0x32, 0x00, 0x00, // S
  0x00, 0x7E, 0x0C, 0x10, 0x20, 0x7E, 0x00, 0x00, // N
  0x00, 0x7E, 0x12, 0x12, 0x12, 0x7E, 0x00, 0x00, // A
  0x00, 0x7E, 0x18, 0x24, 0x42, 0x42, 0x00, 0x00, // K
  0x00, 0x7E, 0x52, 0x52, 0x52, 0x52, 0x00, 0x00, // E
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // ESPACIO
};

// COLUMNAS
unsigned char COLUMNAS[8] = {1, 2, 4, 8, 16, 32, 64, 128};

int main(void) {
    // VARIABLES DE CONTROL
    int estado = 0; 
    int offset_x = 0;
    int contador = 0;
    
    //setup
    DDRB = 0xFF; // PUERTO B SALIDA
    DDRD = 0xFF; // PUERTO D SALIDA 
    
    // PUERTO C ENTRADA PARA PULSADOR
    DDRC &= ~(1<<PC0); 
    PORTC |= (1<<PC0); // PULL-UP INTERNO
    
    while(1) {
        //loop
        
        if (estado == 0) {
            // MUESTRA SNAKE SCROLLING
            for (int velocidad = 0; velocidad < 5; velocidad++) { 
                for (int j = 0; j < 8; j++) {
                    PORTD = COLUMNAS[j];
                    PORTB = ~SECUENCIA_SNAKE[(offset_x + j) % TAMANO_SNAKE];
                    _delay_ms(1); 
                    PORTB = 0xFF; // BORRADO DE FILA
                }
            }
            
            // VELOCIDAD DEL TEXTO
            contador++;
            if (contador > 2) { 
                contador = 0;
                offset_x++;
                if (offset_x >= TAMANO_SNAKE) { offset_x = 0; }
            }
            
            // LECTURA DEL PULSADOR PARA EMPEZAR
            if (!(PINC & (1<<PC0))) {
                _delay_ms(200);
                estado = 1; 
            }
        }
        
        else if (estado == 1) {
            // MATRIZ EN BLANCO - AQUI LES TOCA PROGRAMAR EL JUEGO A LOS DEMAS
            PORTB = 0xFF; 
        }
    }
    return 0;
}
