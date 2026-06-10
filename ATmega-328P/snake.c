//#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>

#define TAMANO_SNAKE 48

// ========== VARIABLES GLOBALES ==========
int serpienteX[64];
int serpienteY[64];
int largo = 3;
int direccion = 2;  // 0=ARRIBA, 1=ABAJO, 2=IZQ, 3=DER
int comidaX, comidaY;
int juegoTerminado = 0;
int mapa[8][8];
int primerMovimiento = 1;

// VARIABLES DE NIVELES
int nivelActual = 1;
int manzanasComidas = 0;

// ARREGLO PALABRA SNAKE
unsigned char SECUENCIA_SNAKE[TAMANO_SNAKE] = {
  0x00, 0x46, 0x4A, 0x4A, 0x4A, 0x32, 0x00, 0x00, // S
  0x00, 0x7E, 0x0C, 0x10, 0x20, 0x7E, 0x00, 0x00, // N
  0x00, 0x7E, 0x12, 0x12, 0x12, 0x7E, 0x00, 0x00, // A
  0x00, 0x7E, 0x18, 0x24, 0x42, 0x42, 0x00, 0x00, // K
  0x00, 0x7E, 0x52, 0x52, 0x52, 0x52, 0x00, 0x00, // E
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // ESPACIO
};

// TEXTOS PARA CAMBIO DE NIVEL (L1, L2, L3)
unsigned char TEXTO_L1[8] = {0x7F, 0x40, 0x40, 0x00, 0x42, 0x7F, 0x40, 0x00};
unsigned char TEXTO_L2[8] = {0x7F, 0x40, 0x40, 0x00, 0x62, 0x51, 0x49, 0x46};
unsigned char TEXTO_L3[8] = {0x7F, 0x40, 0x40, 0x00, 0x22, 0x41, 0x49, 0x36};

unsigned char COLUMNAS[8] = {1, 2, 4, 8, 16, 32, 64, 128};

// ========== DECLARACIÓN DE FUNCIONES ==========
void cargarNivel(int nivel);
void inicializarJuego(int nivel);
void generarComida(void);
int leerBoton(int pin);
void actualizarDireccion(void);
int moverSerpiente(void);
void mostrarMapaEnMatriz(void);
void mostrarGameOver(void);
void mostrarVictoria(void);
void mostrarPantallaInicio(void);
void mostrarTransicion(int nivel);
void enviarComandoAudio(unsigned char comando); // <-- REINTEGRADA

// ========== IMPLEMENTACIÓN DE FUNCIONES ==========

// Función para controlar la comunicación paralela hacia el PIC
void enviarComandoAudio(unsigned char comando) {
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

// FUNCION QUE DIBUJA LOS MAPAS SEGUN EL NIVEL
void cargarNivel(int nivel) {
    // NIVEL 1: MARCO EXTERIOR
    if (nivel == 1) {
        for(int i = 0; i < 8; i++) {
            mapa[0][i] = 3; // PARED SUPERIOR
            mapa[7][i] = 3; // PARED INFERIOR
            mapa[i][0] = 3; // PARED IZQUIERDA
            mapa[i][7] = 3; // PARED DERECHA
        }
    }
    // NIVEL 2: 4 ESQUINAS DE 2X2 (DEJA UNA CRUZ LIBRE)
    else if (nivel == 2) {
        for(int i = 0; i < 2; i++) {
            for(int j = 0; j < 2; j++) {
                mapa[i][j] = 3;         // ESQUINA SUP-IZQ
                mapa[i][j+6] = 3;       // ESQUINA SUP-DER
                mapa[i+6][j] = 3;       // ESQUINA INF-IZQ
                mapa[i+6][j+6] = 3;     // ESQUINA INF-DER
            }
        }
    }
    // NIVEL 3: BLOQUE CENTRAL DE 4X4 (DEJA PASILLO EXTERIOR)
    else if (nivel == 3) {
        for(int i = 2; i < 6; i++) {
            for(int j = 2; j < 6; j++) {
                mapa[i][j] = 3;
            }
        }
    }
}

void inicializarJuego(int nivel) {
    // NACIMIENTO EN PASILLO SUPERIOR (Y=1) PARA EVITAR CHOQUE CON MAPAS
    serpienteX[0] = 4; serpienteY[0] = 1;  // CABEZA
    serpienteX[1] = 3; serpienteY[1] = 1;
    serpienteX[2] = 2; serpienteY[2] = 1;
    largo = 3;
    direccion = 3; // INICIA HACIA LA DERECHA
    juegoTerminado = 0;
    primerMovimiento = 1;
    manzanasComidas = 0; // REINICIO DE CONTADOR

    // LIMPIAR MAPA
    for(int i = 0; i < 8; i++){
        for(int j = 0; j < 8; j++){
            mapa[i][j] = 0;
        }
    }
    
    // CARGAR MAPA DEL NIVEL
    cargarNivel(nivel);
    
    // PONER SERPIENTE
    for(int i = 0; i < largo; i++){
        mapa[serpienteY[i]][serpienteX[i]] = 1;
    }
}

void generarComida(void) {
    int espaciosVacios[64][2];
    int contador = 0;
    
    // BUSCAR CELDAS VACIAS (IGNORA LAS PAREDES QUE SON 3)
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
        // ELEGIR POSICION ALEATORIA
        int seleccion = rand() % contador;
        comidaY = espaciosVacios[seleccion][0];
        comidaX = espaciosVacios[seleccion][1];
        mapa[comidaY][comidaX] = 2;
    } else {
        // NO HAY ESPACIOS VACIOS
        juegoTerminado = 1;
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

// RETORNA: 0 = MUERTE, 1 = OK, 2 = PASA DE NIVEL
int moverSerpiente(void) {
    int nuevaX = serpienteX[0];
    int nuevaY = serpienteY[0];
    
    switch(direccion) {
        case 0: nuevaY--; break;
        case 1: nuevaY++; break;
        case 2: nuevaX--; break;
        case 3: nuevaX++; break;
    }
    
    // COLISION CON BORDES EXTERIORES
    if(nuevaX < 0 || nuevaX >= 8 || nuevaY < 0 || nuevaY >= 8) return 0;
        
    // COLISION CON PAREDES INTERNAS DEL MAPA (LOS 3)
    if(mapa[nuevaY][nuevaX] == 3) return 0; 
    
    int comio = (nuevaX == comidaX && nuevaY == comidaY);
    
    // VERIFICAR COLISION CON CUERPO
    if(!primerMovimiento) {
        for(int i = 1; i < largo; i++) {
            if(serpienteX[i] == nuevaX && serpienteY[i] == nuevaY) return 0;
        }
    }
    
    // BORRAR COLA SI NO COMIO
    if(!comio) {
        int colaX = serpienteX[largo-1];
        int colaY = serpienteY[largo-1];
        mapa[colaY][colaX] = 0;
    }
    
    // MOVER CUERPO
    for(int i = largo; i > 0; i--) {
        serpienteX[i] = serpienteX[i-1];
        serpienteY[i] = serpienteY[i-1];
    }
    
    // NUEVA CABEZA
    serpienteX[0] = nuevaX;
    serpienteY[0] = nuevaY;
    mapa[nuevaY][nuevaX] = 1;
    
    primerMovimiento = 0;
    
    // SI COMIO, CRECER Y VERIFICAR SI PASA DE NIVEL
    if(comio) {
        largo++;
        manzanasComidas++;
        
        // REINTEGRADO: Avisar al PIC que comió y sostener el renderizado para no apagar la matriz
        enviarComandoAudio(0x02); 
        for(int i = 0; i < 4; i++) {
            mostrarMapaEnMatriz();
        }
        
        // CONDICION: 3 MANZANAS PARA GANAR EL NIVEL
        if(manzanasComidas >= 3) {
            return 2; // CODIGO PARA SUBIR DE NIVEL
        }
        generarComida(); 
    }
    
    return 1; // MOVIMIENTO NORMAL
}

void mostrarMapaEnMatriz(void) {
    for(int col = 0; col < 8; col++) {
        PORTD = COLUMNAS[col];
        unsigned char filaData = 0;
        for(int fila = 0; fila < 8; fila++) {
            // ENCIENDE SI HAY SERPIENTE(1), COMIDA(2) O PARED(3)
            if(mapa[fila][col] == 1 || mapa[fila][col] == 2 || mapa[fila][col] == 3) {  
                filaData |= (1 << fila);
            }
        }
        PORTB = ~filaData;
        _delay_ms(3);
        PORTB = 0xFF;
    }
}

void mostrarGameOver(void) {
    enviarComandoAudio(0x03); // <-- REINTEGRADA: Avisar al PIC la muerte
    
    // PARPADEO RAPIDO
    for(int rep = 0; rep < 3; rep++) {
        for(int col = 0; col < 8; col++) {
            PORTD = COLUMNAS[col]; PORTB = 0x00; _delay_ms(3);
        }
        _delay_ms(200);
        for(int col = 0; col < 8; col++) {
            PORTD = COLUMNAS[col]; PORTB = 0xFF; _delay_ms(3);
        }
        _delay_ms(200);
    }
}

void mostrarVictoria(void) {
    enviarComandoAudio(0x00); // Silencio al ganar
    // LLENADO LENTO INDICANDO QUE GANO TODO EL JUEGO
    for(int i = 0; i < 8; i++) {
        for(int col = 0; col < 8; col++) {
            PORTD = COLUMNAS[col]; PORTB = ~(0xFF >> (7-i)); _delay_ms(3);
        }
        _delay_ms(150);
    }
    _delay_ms(1000);
}

void mostrarPantallaInicio(void) {
    static int offset_x = 0;
    static int contador = 0;
    
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
        contador = 0; offset_x++;
        if(offset_x >= TAMANO_SNAKE) offset_x = 0;
    }
}

// MUESTRA L1, L2 o L3
void mostrarTransicion(int nivel) {
    unsigned char* textoActual;
    if (nivel == 1) textoActual = TEXTO_L1;
    else if (nivel == 2) textoActual = TEXTO_L2;
    else textoActual = TEXTO_L3;
    
    for(int tiempo = 0; tiempo < 150; tiempo++) {
        for(int j = 0; j < 8; j++) {
            PORTD = COLUMNAS[j];
            PORTB = ~textoActual[j];
            _delay_ms(2);
            PORTB = 0xFF;
        }
    }
}

int main(void) {
    int estado = 0;  // 0=INICIO, 1=TRANSICION, 2=JUGANDO
    int contadorMovimiento = 0;
    int velocidad = 25;

    //setup
    DDRB = 0xFF;  // FILAS 
    DDRD = 0xFF;  // COLUMNAS 
    
    // CONFIGURACIÓN REINTEGRADA: Botones como entradas (PC0-PC3), Pines de audio como salidas (PC4-PC5)
    DDRC &= ~((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3));  
    DDRC |= (1<<PC4) | (1<<PC5);
    
    PORTC |= (1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3); 
    
    while(1) {
        //loop
        
        // ========== MENU ==========
        if(estado == 0) {
            enviarComandoAudio(0x00); // REINTEGRADA: Silencio en menú de inicio
            mostrarPantallaInicio();
            
            if(leerBoton(PC0) || leerBoton(PC1) || leerBoton(PC2) || leerBoton(PC3)) {
                nivelActual = 1;
                estado = 1; // PASAR A TRANSICION
                _delay_ms(500);
            }
        }
        
        // ========== TRANSICION (L1, L2, L3) ==========
        else if(estado == 1) {
            enviarComandoAudio(0x00); // REINTEGRADA: Silencio durante pantallas de transición ("L1"...)
            mostrarTransicion(nivelActual);
            
            // AJUSTE DE VELOCIDAD POR NIVEL (DIFICULTAD DINAMICA)
            if(nivelActual == 1) velocidad = 25; // LENTO
            if(nivelActual == 2) velocidad = 15; // MEDIO
            if(nivelActual == 3) velocidad = 8;  // RAPIDO
            
            inicializarJuego(nivelActual);
            generarComida();
            estado = 2; // PASAR A JUGAR
        }
        
        // ========== JUEGO ==========
        else if(estado == 2) {
            enviarComandoAudio(0x01); // REINTEGRADA: Música de fondo activa en partida
            
            actualizarDireccion();
            contadorMovimiento++;

            if(contadorMovimiento >= velocidad) {  
                int resultado = moverSerpiente();
                
                // CHOQUE
                if(resultado == 0) {
                    juegoTerminado = 1;
                }
                // PASO DE NIVEL
                else if(resultado == 2) {
                    nivelActual++;
                    if(nivelActual > 3) {
                        mostrarVictoria();
                        estado = 0; // REGRESA AL MENU SI GANO TODO
                    } else {
                        estado = 1; // MUESTRA LA SIGUIENTE TRANSICION
                    }
                }
                contadorMovimiento = 0;
            }
            
            mostrarMapaEnMatriz();
            
            // GAME OVER
            if(juegoTerminado) {
                mostrarGameOver();
                _delay_ms(1500);
                estado = 0;  // VUELVE A INICIO
                juegoTerminado = 0;
                _delay_ms(500);
            }
        }
    }
    
    return 0;
}
