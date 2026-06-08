//#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <util/delay.h>

#define TAMANO_SNAKE 48

// ========== VARIABLES GLOBALES ==========
int serpienteX[64];
int serpienteY[64];
int largo = 3;
int direccion = 2;  // 0=arriba, 1=abajo, 2=izq, 3=der
int comidaX, comidaY;
int juegoTerminado = 0;
int mapa[8][8];
int primerMovimiento = 1;

// ARREGLO PALABRA SNAKE
unsigned char SECUENCIA_SNAKE[TAMANO_SNAKE] = {
  0x00, 0x46, 0x4A, 0x4A, 0x4A, 0x32, 0x00, 0x00, // S
  0x00, 0x7E, 0x0C, 0x10, 0x20, 0x7E, 0x00, 0x00, // N
  0x00, 0x7E, 0x12, 0x12, 0x12, 0x7E, 0x00, 0x00, // A
  0x00, 0x7E, 0x18, 0x24, 0x42, 0x42, 0x00, 0x00, // K
  0x00, 0x7E, 0x52, 0x52, 0x52, 0x52, 0x00, 0x00, // E
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // ESPACIO
};

unsigned char COLUMNAS[8] = {1, 2, 4, 8, 16, 32, 64, 128};

// ========== DECLARACIÓN DE FUNCIONES ==========
void inicializarJuego(void);
void generarComida(void);
void generarObstaculos(int cantidadParedes); // NUEVA FUNCION PAREDES
int leerBoton(int pin);
void actualizarDireccion(void);
int moverSerpiente(void);
void mostrarMatriz(void);
void mostrarGameOver(void);
void mostrarPantallaInicio(void);
void mostrarMapaEnMatriz(void);

// ========== IMPLEMENTACIÓN DE FUNCIONES ==========

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
    
    // GENERAR 3 PAREDES DE PRUEBA (LUEGO SE CAMBIA SEGUN EL NIVEL)
    generarObstaculos(3); 
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
        // No hay espacios vacíos = VICTORIA
        juegoTerminado = 1;
    }
}

// GENERA PAREDES ALEATORIAS EN EL MAPA
void generarObstaculos(int cantidadParedes) {
    int espaciosVacios[64][2];
    int contador = 0;
    
    // BUSCAR CELDAS VACIAS
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            if(mapa[i][j] == 0) {
                espaciosVacios[contador][0] = i;
                espaciosVacios[contador][1] = j;
                contador++;
            }
        }
    }
    
    // COLOCAR PAREDES (NUMERO 3)
    for(int p = 0; p < cantidadParedes; p++) {
        if(contador > 0) {
            int seleccion = rand() % contador;
            int paredY = espaciosVacios[seleccion][0];
            int paredX = espaciosVacios[seleccion][1];
            mapa[paredY][paredX] = 3; 
            
            // ELIMINAR ESPACIO USADO PARA NO REPETIR
            espaciosVacios[seleccion][0] = espaciosVacios[contador-1][0];
            espaciosVacios[seleccion][1] = espaciosVacios[contador-1][1];
            contador--;
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
    
    // Colisión con pared exterior
    if(nuevaX < 0 || nuevaX >= 8 || nuevaY < 0 || nuevaY >= 8)
        return 0;
        
    // COLISION CON PAREDES INTERNAS (OBSTACULOS)
    if(mapa[nuevaY][nuevaX] == 3) {
        return 0; 
    }
    
    int comio = (nuevaX == comidaX && nuevaY == comidaY);
    
    // Verificar colisión con cuerpo
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
    
    // Dibujar nueva cabeza
    mapa[nuevaY][nuevaX] = 1;
    
    primerMovimiento = 0;
    
    // Si comió, aumentar largo y generar comida
    if(comio) {
        largo++;
        generarComida();  // Asegurar que se llama después de aumentar largo
    }
    
    return 1;
}

//Muestra solo el mapa del juego
void mostrarMapaEnMatriz(void) {
    for(int col = 0; col < 8; col++) {
        PORTD = COLUMNAS[col];
        
        unsigned char filaData = 0;
        for(int fila = 0; fila < 8; fila++) {
            // SE AÑADIO EL NUMERO 3 (PARED) PARA QUE ENCIENDA EL LED
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

void mostrarPantallaInicio(void) {
    static int offset_x = 0;
    static int contador = 0;
    
    // Mostrar scrolling "SNAKE"
    for(int barrido = 0; barrido < 3; barrido++) {  // Reducido para no saturar
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

int main(void) {
    int estado = 0;  // 0 = inicio, 1 = jugando
    int contadorMovimiento = 0;

    // Configurar puertos
    DDRB = 0xFF;  // Filas - salida
    DDRD = 0xFF;  // Columnas - salida
    
    // Configurar botones en PC0, PC1, PC2, PC3
    DDRC &= ~((1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3));  // Todos como entrada
    PORTC |= (1<<PC0) | (1<<PC1) | (1<<PC2) | (1<<PC3); 
    
    inicializarJuego();
    generarComida();

    while(1) {
        if(estado == 0) {
            mostrarPantallaInicio();
            
            if(leerBoton(PC0) || leerBoton(PC1) || leerBoton(PC2) || leerBoton(PC3)) {
                estado = 1;
                inicializarJuego();
                generarComida();
                _delay_ms(500);
            }
        }
        
        // ========== JUEGO ==========
        else if(estado == 1) {
            
            // Leer botones para mover
            actualizarDireccion();
            
            contadorMovimiento++;

            if(contadorMovimiento  >= 25) {  // Ajusta este valor (mayor = más lento)
                if(!moverSerpiente()) {
                    juegoTerminado = 1;
                }
                contadorMovimiento = 0;
            }
            
            // Mostrar el juego en la matriz
            mostrarMapaEnMatriz();
            
            // ========== GAME OVER ==========
            if(juegoTerminado) {
                mostrarGameOver();
                _delay_ms(1500);
                
                // Esperar confirmación para reiniciar
                estado = 0;  // Vuelve a pantalla de inicio
                inicializarJuego();
                generarComida();
                juegoTerminado = 0;
                _delay_ms(500);
            }
        }
    }
    
    return 0;
}
