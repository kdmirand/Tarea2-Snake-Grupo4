// TAREA 2 - SISTEMAS EMBEBIDOS
// ESTUDIANTE: KAREN DANIELA MIRANDA BRITO
// TEMA: INICIO DE JUEGO SNAKE

#include <avr/io.h>
#include <util/delay.h>
//numero de elementos en el arreglo de la palabra SNAKE
#define TAMANO_SNAKE 48
//numero de elementos en el areglo de dificultades (3 dificultades * 8 filas cada una)
#define tamnio_dificultades 24
//direcciones para el movimiento de la serpiente
#define DERECHA 0
#define IZQUIERDA 1
#define ARRIBA 2
#define ABAJO 3
//movimiento incial de la serpiente
unsigned char direccion = DERECHA;

int offset_x = 0;
// ARREGLO PALABRA SNAKE
unsigned char SECUENCIA_SNAKE[TAMANO_SNAKE] = {
  0x00, 0x46, 0x4A, 0x4A, 0x4A, 0x32, 0x00, 0x00, // S
  0x00, 0x7E, 0x0C, 0x10, 0x20, 0x7E, 0x00, 0x00, // N
  0x00, 0x7E, 0x12, 0x12, 0x12, 0x7E, 0x00, 0x00, // A
  0x00, 0x7E, 0x18, 0x24, 0x42, 0x42, 0x00, 0x00, // K
  0x00, 0x7E, 0x52, 0x52, 0x52, 0x52, 0x00, 0x00, // E
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // ESPACIO
};
// ARREGLO DIFICULTADES
unsigned char difultades[tamnio_dificultades] = {
  0x00,	0x40,	0x44,	0x7E,	0x7E,	0x40,	0x40,	0x00, // FACIL(1)
  0x00,	0x66,	0x76,	0x7E,	0x5E,	0x4C,	0x00,	0x00, // NORMAL(2)
  0x00,	0x5A,	0x5A,	0x5A,	0x5A,	0x7E,	0x3C,	0x00 // DIFICIL(3)
};


// COLUMNAS 
unsigned char COLUMNAS[8] = {1, 2, 4, 8, 16, 32, 64, 128};

//pantala de lo q se va a estar mostrando en cada momento(pensado para juego de 1 solo punto)
unsigned char pantalla[8];

//arreglo para la posicion de la serpiente
unsigned char serpiente_x[64];
unsigned char serpiente_y[64];
//posicion de la comida 
unsigned char comida_x = 6;
unsigned char comida_y = 5;
//longitud de la serpiente
unsigned char longitud = 1;


//muestra el dibujo de la serpiente donde sea q esté ubicado para el juego
void mostrar_juego(){
  for(int j=0;j<8;j++){
    PORTD = COLUMNAS[j];
    PORTB = ~pantalla[j];
    _delay_ms(0.06);
    PORTB = 0xFF;
  }
}
//movimiento de la serpiente
void mover_serpiente()
{
    for(int i = longitud-1; i > 0; i--)
    {
        serpiente_x[i] = serpiente_x[i-1];
        serpiente_y[i] = serpiente_y[i-1];
    }
    //movimiento de la serpiente hacia la derecha
    if(direccion == DERECHA)
    {
        serpiente_x[0]++;
        if(serpiente_x[0] > 7)//si pasa el limite de la pantalla, vuelve a aparecer por el otro lado
            serpiente_x[0] = 0;
    }
    //movimiento de la serpiente hacia la izquierda
    else if(direccion == IZQUIERDA)
    {
        if(serpiente_x[0] == 0)//si pasa el limite de la pantalla, vuelve a aparecer por el otro lado
            serpiente_x[0] = 7;
        else
            serpiente_x[0]--;
    }
    //movimiento de la serpiente hacia arriba
    else if(direccion == ARRIBA)
    {
        if(serpiente_y[0] == 0)// si pasa el limite de la pantalla, vuelve a aparecer por el otro lado
            serpiente_y[0] = 7;
        else
            serpiente_y[0]--;
    }
    //movimiento de la serpiente hacia abajo
    else if(direccion == ABAJO)
    {
        serpiente_y[0]++;

        if(serpiente_y[0] > 7)// si pasa el limite de la pantalla, vuelve a aparecer por el otro lado
            serpiente_y[0] = 0;
    }
}

void dibujar_juego()
{
    for(int i=0;i<8;i++)
    {
        pantalla[i] = 0x00;//limpieza de la pantalla para volver a dibujar la serpiente y la comida en su nueva posición
    }

    // posicion comida
    pantalla[comida_y] |= (1 << comida_x);

    // posicion serpiente
    for(int i=0;i<longitud;i++)
    {
        pantalla[serpiente_y[i]] |= (1 << serpiente_x[i]);
    }
}
//lee si se presionó algún botón para cambiar la dirección de la serpiente
void leer_botones()
{
    if(!(PINC & (1<<PC0)))   // DERECHA
    {
        direccion = DERECHA;
    }

    else if(!(PINC & (1<<PC1)))   // IZQUIERDA
    {
        direccion = IZQUIERDA;
    }

    else if(!(PINC & (1<<PC2)))   // ARRIBA
    {
        direccion = ARRIBA;
    }

    else if(!(PINC & (1<<PC3)))   // ABAJO
    {
        direccion = ABAJO;
    }
}

void mostrar_snake() {
  // MUESTRA SNAKE SCROLLING
  for (int velocidad = 0; velocidad < 5; velocidad++) { 
    for (int j = 0; j < 8; j++) {
        PORTD = COLUMNAS[j];
        PORTB = ~SECUENCIA_SNAKE[(offset_x + j) % TAMANO_SNAKE];
        _delay_ms(0.06); 
        PORTB = 0xFF; // BORRADO DE FILA
    }
  }
}
//dibuja la pantalla "estatica" del numero de la dificultad
void mostrar_dificultades(int dificultad) {
  // MUESTRA # DIFICULTAD
  for (int j = 0; j < 8; j++) {
    PORTD = COLUMNAS[j];
    PORTB = ~difultades[(dificultad-1)*8 + j];
    _delay_ms(0.06); 
    PORTB = 0xFF; // BORRADO DE FILA
  }
}



int main(void) {
    // VARIABLES DE CONTROL
    int estado = 1; // 0: Pantalla de inicio, 1: Juego 
    int contador = 0;
    
    //setup
    DDRB = 0xFF; // PUERTO B SALIDA
    DDRD = 0xFF; // PUERTO D SALIDA 
    
    // PUERTO C ENTRADA PARA PULSADOR(aunque no es necesario declararlo como entrada)
    DDRC &= ~((1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3));
    PORTC |= (1<<PC0)|(1<<PC1)|(1<<PC2)|(1<<PC3); // PULL-UP INTERNO

    // POSICIÓN INICIAL DE LA SERPIENTE
    serpiente_x[0] = 3;
    serpiente_y[0] = 3;
    
    while(1) {
      //loop
      if (estado == 0) {
        mostrar_snake(); // MUESTRA SNAKE SCROLLING
        //Mensaje Seleccione Dificultad
        
        //mostrar_dificultades(3); // MUESTRA DIFICULTADES

        /* // VELOCIDAD DEL TEXTO
        contador++;
        if (contador > 2) { 
            contador = 0;
            offset_x++;
            if (offset_x >= TAMANO_SNAKE) { offset_x = 0; }
        } */
        
        // LECTURA DEL PULSADOR PARA EMPEZAR
        if (!(PINC & (1<<PC0))) {
            _delay_ms(200);
            estado = 1; 
        }
      }
      
      else if (estado == 1) {
          // MATRIZ EN BLANCO - AQUI LES TOCA PROGRAMAR EL JUEGO A LOS DEMAS
          //PORTB = 0xFF;
          dibujar_juego();

          for(int t = 0; t < 50; t++)
          {
              mostrar_juego();
          }
          leer_botones();
          mover_serpiente();
          _delay_ms(2);
          //comprueba si la serpiente comió la comida, si es así, aumenta la longitud de la serpiente y genera una nueva comida en una posición diferente
          if(serpiente_x[0] == comida_x &&serpiente_y[0] == comida_y){
              longitud++;
              comida_x = (comida_x + 3) % 8;
              comida_y = (comida_y + 5) % 8;
          }

      }
    }
    return 0;
}
