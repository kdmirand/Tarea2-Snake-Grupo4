//#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>

#define TAMANO_SNAKE 48
#define tamnio_dificultades 24

// ========== VARIABLES GLOBALES ==========
int serpienteX[64];
int serpienteY[64];
int largo = 3;
int direccion = 2;  // 0=arriba, 1=abajo, 2=izq, 3=der
int comidaX, comidaY;
int juegoTerminado = 0;
int mapa[8][8];
int primerMovimiento = 1;
int faseDificultad = 0;   // 0 = texto, 1 = selección
int dificultad = 1;       // por defecto fácil
int contadorTexto = 0;
int offset_x_dificultad = 0;
int contador_dificultad = 0;

// ARREGLO PALABRA SNAKE
unsigned char SECUENCIA_SNAKE[TAMANO_SNAKE] = {
  0x00, 0x46, 0x4A, 0x4A, 0x4A, 0x32, 0x00, 0x00, // S
  0x00, 0x7E, 0x0C, 0x10, 0x20, 0x7E, 0x00, 0x00, // N
  0x00, 0x7E, 0x12, 0x12, 0x12, 0x7E, 0x00, 0x00, // A
  0x00, 0x7E, 0x18, 0x24, 0x42, 0x42, 0x00, 0x00, // K
  0x00, 0x7E, 0x52, 0x52, 0x52, 0x52, 0x00, 0x00, // E
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // ESPACIO
};

// Arreglo para mostrar la palabra "DIFICULTAD" en la pantalla de selección de dificultad
unsigned char texto_dificultad[88] = {
    0x00, 0x7E,    0x7E, 0x42,    0x42, 0x7E,    0x3C, 0x00, // D
    0x00, 0x42,    0x42, 0x7E,    0x7E, 0x42,    0x42, 0x00, // I
    0x00, 0x7E,    0x7E, 0x12,    0x12, 0x02,    0x02, 0x00, // F
    0x00, 0x42,    0x42, 0x7E,    0x7E, 0x42,    0x42, 0x00, // I
    0x00, 0x3C,    0x7E, 0x42,    0x42, 0x42,    0x24, 0x00, // C
    0x00, 0x3E,    0x7E, 0x40,    0x40, 0x7E,    0x3E, 0x00, // U
    0x00, 0x7E,    0x7E, 0x40,    0x40, 0x40,    0x40, 0x00, // L
    0x00, 0x02,    0x02, 0x7E,    0x7E, 0x02,    0x02, 0x00, // T
    0x00, 0x7C,    0x7E, 0x12,    0x12, 0x7E,    0x7C, 0x00, // A
    0x00, 0x7E,    0x7E, 0x42,    0x42, 0x7E,    0x3C, 0x00, // D
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // ESPACIO
};

// ARREGLO DIFICULTADES
unsigned char difultades[tamnio_dificultades] = {
  0x00,    0x40,    0x44,    0x7E,    0x7E,    0x40,    0x40,    0x00, // FACIL(1)
  0x00,    0x66,    0x76,    0x7E,    0x5E,    0x4C,    0x00,    0x00, // NORMAL(2)
  0x00,    0x5A,    0x5A,    0x5A,    0x5A,    0x7E,    0x3C,    0x00  // DIFICIL(3)
};

unsigned char COLUMNAS[8] = {1, 2, 4, 8, 16, 32, 64, 128};

// ========== DECLARACIÓN DE FUNCIONES ==========
void inicializarJuego(void);
void generarComida(void);
int leerBoton(int pin);
void actualizarDireccion(void);
int moverSerpiente(void);
void mostrarMatriz(void);
void mostrarGameOver(void);
void mostrarPantallaInicio(void);
void mostrarMapaEnMatriz(void);
void generarObstaculos(void);
void enviarComandoAudio(unsigned char comando); // <-- INTEGRADA: Control de Audio

// ========== IMPLEMENTACIÓN DE FUNCIONES ==========

// Función integrada para controlar la comunicación paralela hacia el PIC
void enviarComandoAudio(unsigned char comando) {
    // Usaremos PC4 y PC5 para enviar los datos en binario al PIC:
    // Comando 0x00 (Silencio) -> PC5=0, PC4=0
    // Comando 0x01 (Música)   -> PC5=0, PC4=1
    // Comando 0x02 (Comida)    -> PC5=1, PC4=0
    // Comando 0x03 (Muerte)   -> PC5=1, PC4=1
    
    // Limpiamos los bits PC4 y PC5 sin alterar los botones de PC0-PC3
    PORTC &= ~((1 << PC4) | (1 << PC5));
    
    // Asignamos el valor del comando a los pines correspondientes
    if (comando & 0x01) PORTC |= (1 << PC4);
    if (comando & 0x02) PORTC |= (1 << PC5);
}

void inicializarJuego(void) {
    serpienteX[0] = 4; serpienteY[0] = 4;  // cabeza
    serpienteX[1] = 3; serpienteY[1] = 4;
    serpienteX[2] = 2; serpienteY[2] = 4;
    largo = 3;
    direccion = 2;
    juegoTerminado = 0;
    primerMovimiento = 1;

    // Limpiar mapa
    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 8; j++){
            mapa[i][j] = 0;
        }
    }
    
    // Poner serpiente
    for(int i = 0; i < largo; i++){
        mapa[serpienteY[i]][serpienteX[i]] = 1;
    }
}

void generarComida(void) {
    int espaciosVacios[64][2];
    int contador = 0;
    
    // Buscar todas las celdas vacías
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            if(mapa[i][j] == 0) {
                espaciosVacios[contador][0] = i;
                espaciosVacios[contador][1] = j;
                contador++;
            }
        }
    }
    
    if(contador > 0) {
        // Elegir posición aleatoria
        int seleccion = rand() % contador;
        comidaY = espaciosVacios[seleccion][0];
        comidaX = espaciosVacios[seleccion][1];
        mapa[comidaY][comidaX] = 2;
    } else {
        // No hay espacios vacíos = VICTORIA / TERMINADO
        juegoTerminado = 1;
    }
}

void generarObstaculos(void) {
    int cantidad = 1 + (rand() % 3);   // entre 1 y 3 obstáculos
    int longitud = 2 + (rand() % 2);   // tamaño de 2 o 3

    for(int obs = 0; obs < cantidad; obs++) {
        int colocado = 0;
        while(!colocado) {
            int x = rand() % 8;
            int y = rand() % 8;
            int orientacion = rand() % 2; // 0=horizontal, 1=vertical
            int valido = 1;

            // Verificar si cabe
            for(int i = 0; i < longitud; i++) {
                int nx = x;
                int ny = y;

                if(orientacion == 0)
                    nx += i;
                else
                    ny += i;
                
                // Sale del tablero
                if(nx >= 8 || ny >= 8) {
                    valido = 0;
                    break;
                }

                // Revisar alrededor (incluyendo diagonales)
                for(int dy = -1; dy <= 1; dy++) {
                    for(int dx = -1; dx <= 1; dx++) {
                        int rx = nx + dx;
                        int ry = ny + dy;

                        if(rx >= 0 && rx < 8 && ry >= 0 && ry < 8) {
                            if(mapa[ry][rx] != 0) {
                                valido = 0;
                            }
                        }
                    }
                }

                if(!valido)
                    break;
            }

            // Colocar obstáculo
            if(valido) {
                for(int i = 0; i < longitud; i++) {
                    int nx = x;
                    int ny = y;

                    if(orientacion == 0)
                        nx += i;
                    else
                        ny += i;

                    mapa[ny][nx] = 3;
                }
                colocado = 1;
            }
        }
    }
}

int leerBoton(int pin) {
    if(!(PINC & (1 << pin))) {
        _delay_ms(50);
        if(!(PINC & (1 << pin))) {
            while(!(PINC & (1 << pin)));
            _delay_ms(50);
            return 1;
        }
    }
    return 0;
}

void actualizarDireccion(void) {
    if(leerBoton(PC0)) {
        if(direccion != 1) direccion = 0;
    }
    else if(leerBoton(PC1)) {
        if(direccion != 0) direccion = 1;
    }
    else if(leerBoton(PC2)) {
        if(direccion != 3) direccion = 2;
    }
    else if(leerBoton(PC3)) {
        if(direccion != 2) direccion = 3;
    }
}

int moverSerpiente(void) {
    int nuevaX = serpienteX[0];
    int nuevaY = serpienteY[0];
    
    switch(direccion) {
        case 0: nuevaY--; break;
        case 1: nuevaY++; break;
        case 2: nuevaX--; break;
        case 3: nuevaX++; break;
    }
    
    // Colisión con pared según nivel de dificultad
    if(dificultad != 1) {
        // MODOS NORMAL Y DIFÍCIL: muere al tocar los bordes
        if(nuevaX < 0 || nuevaX >= 8 || nuevaY < 0 || nuevaY >= 8)
            return 0;
    }
    else {
        // MODO FACIL: puede atravesar paredes (teletransportarse)
        if(nuevaX < 0) nuevaX = 7;
        else if(nuevaX >= 8) nuevaX = 0;
        if(nuevaY < 0) nuevaY = 7;
        else if(nuevaY >= 8) nuevaY = 0;
    }
    
    // Colisión con obstáculo
    if(mapa[nuevaY][nuevaX] == 3)
        return 0;

    int comio = (nuevaX == comidaX && nuevaY == comidaY);
    
    // Verificar colisión con el propio cuerpo
    if(!primerMovimiento) {
        for(int i = 1; i < largo; i++) {
            if(serpienteX[i] == nuevaX && serpienteY[i] == nuevaY) {
                return 0;
            }
        }
    }
    
    // Borrar cola solo si NO comió
    if(!comio) {
        int colaX = serpienteX[largo-1];
        int colaY = serpienteY[largo-1];
        mapa[colaY][colaX] = 0;
    }
    
    // Mover cuerpo (desde el último hasta el primero)
    for(int i = largo; i > 0; i--) {
        serpienteX[i] = serpienteX[i-1];
        serpienteY[i] = serpienteY[i-1];
    }
    
    // Colocar nueva cabeza
    serpienteX[0] = nuevaX;
    serpienteY[0] = nuevaY;
    mapa[nuevaY][nuevaX] = 1;
    
    primerMovimiento = 0;
    
    // Si comió, aumentar largo, sonar buzzer y generar comida nueva
    if(comio) {
        largo++;
        
        // INTERFAZ DE SONIDO: Envía pulso seguro al PIC y actualiza la matriz simultáneamente
        enviarComandoAudio(0x02); 
        for(int i = 0; i < 4; i++) {
            mostrarMapaEnMatriz();
        }
        
        generarComida();  // Asegurar que se llama después de aumentar largo
    }
    
    return 1;
}

// Muestra el mapa completo del juego (Cuerpo, Fruta, Obstáculos)
void mostrarMapaEnMatriz(void) {
    for(int col = 0; col < 8; col++) {
        PORTD = COLUMNAS[col];
        
        unsigned char filaData = 0;
        for(int fila = 0; fila < 8; fila++) {
            if(mapa[fila][col] == 1 || mapa[fila][col] == 2 || mapa[fila][col] == 3) {  // Serpiente, comida u obstáculo
                filaData |= (1 << fila);
            }
        }
        PORTB = ~filaData;
        _delay_ms(3);
        PORTB = 0xFF;
    }
}

void mostrarGameOver(void) {
    enviarComandoAudio(0x03); // <-- INTEGRADA: Envía señal de muerte al PIC
    
    // Parpadeo rápido 3 veces
    for(int rep = 0; rep < 3; rep++) {
        // Todo encendido
        for(int col = 0; col < 8; col++) {
            PORTD = COLUMNAS[col];
            PORTB = 0x00;
            _delay_ms(3);
        }
        _delay_ms(200);
        
        // Todo apagado
        for(int col = 0; col < 8; col++) {
            PORTD = COLUMNAS[col];
            PORTB = 0xFF;
            _delay_ms(3);
        }
        _delay_ms(200);
    }
}

// Dibuja la pantalla estática con el número indicador del nivel
void mostrar_dificultades(int dificultad) {
  for (int j = 0; j < 8; j++) {
    PORTD = COLUMNAS[j];
    PORTB = ~difultades[(dificultad-1)*8 + j];
    _delay_ms(5); 
    PORTB = 0xFF; // Borrado de fila para evitar reflejos
  }
}

void mostrar_texto_dificultad(void) {
    for(int barrido = 0; barrido < 3; barrido++) {
        for(int j = 0; j < 8; j++) {
            PORTD = COLUMNAS[j];
            PORTB = ~texto_dificultad[(offset_x_dificultad + j) % 88];
            _delay_ms(5);
            PORTB = 0xFF;
        }
    }
    contador_dificultad++;
    if(contador_dificultad > 2) {
        contador_dificultad = 0;
        offset_x_dificultad++;
        if(offset_x_dificultad >= 88)
            offset_x_dificultad = 0;
    }
}

void mostrarPantallaInicio(void) {
    static int offset_x = 0;
    static int contador = 0;
    
    // Mostrar scrolling "SNAKE"
    for(int barrido = 0; barrido < 3; barrido++) {  
        for(int j = 0; j < 8; j++) {
            PORTD = COLUMNAS[j];
            PORTB = ~SECUENCIA_SNAKE[(offset_x + j) % TAMANO_SNAKE];
            _delay_ms(5);
            PORTB = 0xFF;
        }
    }
    
    contador++;
    if(contador > 2) {
        contador = 0;
        offset_x++;
        if(offset_x >= TAMANO_SNAKE) offset_x = 0;
    }
}

// ========== PROGRAMA PRINCIPAL ==========
int main(void) {
    int estado = 0;  // 0 = inicio, 1 = selección dificultad, 2 = juego
    int contadorMovimiento = 0;

    // Configurar puertos de salida gráfica
    DDRB = 0xFF;  // Filas - salida
    DDRD = 0xFF;  // Columnas - salida
    
    // Configurar botones en PC0-PC3 como ENTRADAS, y PC4-PC5 como SALIDAS de audio al PIC
    DDRC &= ~((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3));  
    DDRC |= (1<<PC4) | (1<<PC5); 
    
    // Pull-up para los pulsadores de control
    PORTC |= (1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3);  
     
    inicializarJuego();
    generarComida();

    while(1) {
        if(estado == 0) {
            enviarComandoAudio(0x00); // Silencio en el menú de inicio
            mostrarPantallaInicio();
            
            if(leerBoton(PC0) || leerBoton(PC1) || leerBoton(PC2) || leerBoton(PC3)) {
                estado = 1;
                contadorTexto = 0;    // Reset del contador para el texto de dificultad
                faseDificultad = 0;   // Inicia con el texto desplazándose
                _delay_ms(500);
            }
        }
        else if(estado == 1) {
            enviarComandoAudio(0x00); // Silencio mientras se configura el nivel
            
            // FASE 0: TEXTO SCROLLING "DIFICULTAD"
            if(faseDificultad == 0) {
                mostrar_texto_dificultad();
                contadorTexto++;
                if(contadorTexto > 200) {
                    faseDificultad = 1;
                }
            }
            // FASE 1: MENÚ DE SELECCIÓN DE DIFICULTAD
            else {
                mostrar_dificultades(dificultad);
                
                // Subir nivel (PC3)
                if(leerBoton(PC3)) {
                    if(dificultad < 3)
                        dificultad++;
                }
                // Bajar nivel (PC2)
                if(leerBoton(PC2)) {
                    if(dificultad > 1)
                        dificultad--;
                }
                // Confirmar Selección e iniciar partida (PC0)
                if(leerBoton(PC0)) {
                    inicializarJuego();
                    // SOLO genera obstáculos físicos en el nivel Difícil (Nivel 3)
                    if(dificultad == 3) {
                        generarObstaculos();
                    }
                    generarComida();
                    estado = 2;  // Cambia a estado de Juego activo
                    _delay_ms(300);
                }
            }
        }
        // ========== LOGICA JUEGO ACTIVO ==========
        else if(estado == 2) {
            enviarComandoAudio(0x01); // <-- INTEGRADA: Envío de música de fondo constante en juego
            
            actualizarDireccion();
            contadorMovimiento++;

            // El regulador de avance no interrumpe el refresco dinámico de la matriz
            if(contadorMovimiento >= 25) {  
                if(!moverSerpiente()) {
                    juegoTerminado = 1;
                }
                contadorMovimiento = 0;
            }
            
            mostrarMapaEnMatriz();
            
            // ========== MANEJO GAME OVER ==========
            if(juegoTerminado) {
                mostrarGameOver();
                _delay_ms(1500);
                
                estado = 0;  // Vuelve suavemente a la pantalla de inicio ("SNAKE")
                inicializarJuego();
                generarComida();
                juegoTerminado = 0;
                _delay_ms(500);
            }
        }
    }
    
    return 0;
}
