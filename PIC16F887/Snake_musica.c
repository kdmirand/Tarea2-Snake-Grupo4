//TAREA #2 - SISTEMAS EMBEBIDOS

unsigned short comandoActual = 0;
unsigned short muerteReproducida = 0;
unsigned short Comida = 0; // Bandera para registrar el evento de comer

// Notas
void nota_sol3(int duration){
     Sound_Play(196.00, duration);
}
void nota_la3(int duration){
     Sound_Play(220.00, duration); 
}
void nota_si3(int duration){
     Sound_Play(246.94, duration);
}
void nota_do4(int duration){ 
     Sound_Play(261.63, duration); 
}
void nota_re4(int duration){ 
     Sound_Play(293.66, duration); 
}
void nota_mi4(int duration){ 
     Sound_Play(329.63, duration);
}
void nota_fa4(int duration){ 
     Sound_Play(349.23, duration);
}
void nota_sol4(int duration){ 
     Sound_Play(392.00, duration); 
}

// Música de fondo
void Fondo_juego() {
    int d = 140;
    nota_do4(d);     
    if ((PORTB & 0x0F) == 0x02){
       Comida = 1;
       return;
    }
    nota_re4(d);     
    if ((PORTB & 0x0F) == 0x02){
       Comida = 1;
       return;
    }
    nota_mi4(d * 2); 
    if ((PORTB & 0x0F) == 0x02){ 
       Comida = 1;
       return;
    }
    nota_do4(d);     
    if ((PORTB & 0x0F) == 0x02){ 
       Comida = 1;
       return;
    }
    nota_sol3(d * 2);
}

// Melodía de comida
void Fondo_Comida(){
    nota_do4(90);
    nota_mi4(90);
    nota_sol4(180);
    Comida = 0; // Consumimos la bandera al terminar de tocar
}

void Fondo_Muelte() {
    nota_sol4(200);
    nota_mi4(200);
    nota_do4(400);
}

void main() {
    ANSEL  = 0;
    ANSELH = 0;
    C1ON_bit = 0;
    C2ON_bit = 0;

    TRISB = 0xFF; // Puerto B como Entrada
    TRISC = 0x00; // Puerto C como Salida

    Sound_Init(&PORTC, 3); // Inicializa buzzer en RC3

    while (1) {
        comandoActual = PORTB & 0x0F;

        if (comandoActual == 0x02) {
            Comida = 1;
        }

        // CONTROL DE PRIORIDADES
        if (comandoActual == 0x03) { // Prioridad Máxima si se muere
            if (muerteReproducida == 0) {
                Fondo_Muelte();
                muerteReproducida = 1;
            }
            Sound_Play(0, 0);
            Delay_ms(30);
        }
        else if (Comida == 1) {   //Prioridad Media, el Sonido de comida
            Fondo_Comida();
        }
        else if (comandoActual == 0x01) { // Estado Normal del Juego
            muerteReproducida = 0;
            Fondo_juego();
        }
        else {
            Sound_Play(0, 0);
            muerteReproducida = 0;
            Delay_ms(30);
        }
    }
}
