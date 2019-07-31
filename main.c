#include "config.h"
#define _XTAL_FREQ 4000000
#define VALOR_TIMER0 26472
#define RETARDO 5 //CAMBIO DE DISPLAY CADA 5 MS
//son 5 segundos en tiempo real y 35 para simulacion en Proteus

unsigned int AmPm = 1; // |0  cuando es AM | 1 cuando es PM
unsigned int formato = 0; //|0 cuando es 24 Horas | 1  cuando es 12 horas
int contadorBotonSet = 0;
signed int* digitoActual;
signed int contDecHora = 1, contHora = 2, contDecMin = 0, contMin = 0;
signed int contDecHoraAux, contHoraAux;
signed int *contDecHoraMostrar, *contHoraMostrar;
unsigned int interrupcionesRealizar = 6; //Se necesitan 10 interrupciones para
//que transcurra un minuto
unsigned int numeros[] = {63, 6, 91, 79, 102, 109, 125, 71, 127, 103};
                        // 0  1   2   3   4    5    6    7   8    9

void ajustarReloj(void);
void dameTemperatura(void);
void controlContadores(void);
void controlBotones(void);
void mostrarDigitos(void);
void validaMinutos(void);
void validaDecenasMinutos(void);
void validaHoras(void);
void parpadearDigitos(void);
void verificaAmPm(void);
void convertirFormato(void);

void __interrupt() desbordamiento(void) {
    if (INTCONbits.TMR0IF) //¿Se desbordo el registro Timer 0?
    {
        interrupcionesRealizar--;
        if (!interrupcionesRealizar) {
            interrupcionesRealizar = 6;
            contMin++;
        }

        INTCONbits.TMR0IF = 0; //Regresando Bandera a 0 (Interrupcion por Timer 0)
        TMR0 = VALOR_TIMER0; //Inicializando Timer 0
    }

}

void ajustarReloj(void) {
    T0CONbits.TMR0ON = 0;
    __delay_ms(125);
    T0CONbits.TMR0ON = 1;
}

void controlContadores(void) {

    if (((contDecHora == 1 && contHora == 2) || (contDecHora == 2 && contHora == 3)) && (contDecMin == 5 && contMin == 10))
        ajustarReloj();

    if (contDecHora == 2 && contHora == 3 && contDecMin == 5 && contMin == 10) {
        contDecHora = 0;
        contHora = 0;
        contDecMin = 0;
        contMin = 0;
    } else {

        if (contMin == 10) {
            contMin = 0;
            contDecMin++;
        }
        if (contDecMin == 6) {
            contDecMin = 0;
            contHora++;
        }
        if (contHora == 10) {
            contHora = 0;
            contDecHora++;
        }

    }

}

void validaMinutos(void) {
    if (*digitoActual == -1) {
        *digitoActual = 9;
    } else if (*digitoActual == 10) {
        *digitoActual = 0;
    }
}

void validaDecenasMinutos(void) {
    if (*digitoActual == -1) {
        *digitoActual = 5;
    } else if (*digitoActual == 6) {
        *digitoActual = 0;
    }
}

void validaHoras(void) {
    if (contDecHora == 1 && *digitoActual == -1) {
        contDecHora = 0;
        *digitoActual = 9;
    } else if (contDecHora == 2 && *digitoActual == 4) {
        contDecHora = 0;
        *digitoActual = 0;
    } else if (contDecHora == 2 && *digitoActual == -1) {
        contDecHora = 1;
        *digitoActual = 9;
    } else if (!contDecHora && *digitoActual == -1) {
        contDecHora = 2;
        *digitoActual = 3;
    }
}

void dameTemperatura(void) {

    while (PORTBbits.RB1); //ANTIREBOTE

    int unidades = 0, decenas = 0, repeticiones = 400, temperatura;

    ADCON0bits.CHS = 0b0000; //Leer canal 0
    ADCON0bits.GO_DONE = 1; //Bandera en 1
    while (ADCON0bits.GO_DONE); //Hasta que se baje la bandera
    temperatura = ADRESH; //  Lectura de valor AD.
    temperatura = (temperatura << 8) + ADRESL; //Juntando los 2 registros para una variable de 10 bits
    temperatura = temperatura * 0.48875; //10 MV por grado

    unidades = temperatura % 10;
    decenas = (temperatura / 10) % 10;

    while (repeticiones) {

        LATD = numeros[unidades];
        LATA = 0b000010;
        __delay_ms(RETARDO);

        if (decenas) {
            LATD = numeros[decenas];
            LATA = 0b000100;
            __delay_ms(RETARDO);
        }

        repeticiones--;

    }

}

void controlBotones(void) {

    if (PORTBbits.RB0) { //BOTON SET

        while (PORTBbits.RB0); //ANTIREBOTE

        contadorBotonSet++;

        switch (contadorBotonSet) {

            case 1:
                digitoActual = &contMin;
                break;

            case 2:
                digitoActual = &contDecMin;
                break;

            case 3:
                digitoActual = &contHora;
                break;

            default:
                contadorBotonSet = 0;
                break;

        }
    } else if (PORTBbits.RB1) { //INCREMENTAR DIGITO

        while (PORTBbits.RB1); //ANTIREBOTE

        (*digitoActual)++;

        switch (contadorBotonSet) {
            case 1:
                validaMinutos();
                break;

            case 2:
                validaDecenasMinutos();
                break;

            case 3:
                validaHoras();
                break;
        }



    } else if (PORTBbits.RB2) //DECREMENTAR DIGITO
    {
        while (PORTBbits.RB2); //ANTIREBOTE

        (*digitoActual)--;

        switch (contadorBotonSet) {
            case 1:
                validaMinutos();
                break;

            case 2:
                validaDecenasMinutos();
                break;

            case 3:
                validaHoras();
                break;
        }
    }


}

void parpadearDigitos(void) {

    unsigned int repeticiones = 5;
    while (repeticiones) {

        LATD = numeros[contMin];

        if (contadorBotonSet == 1) {
            if (repeticiones == 1) {
                LATA = 0b000010;
                __delay_ms(RETARDO);
            }
        } else {
            LATA = 0b000010;
            __delay_ms(RETARDO);
        }

        LATD = numeros[contDecMin];

        if (contadorBotonSet == 2) {
            if (repeticiones == 1) {
                LATA = 0b000100;
                __delay_ms(RETARDO);
            }
        } else {
            LATA = 0b000100;
            __delay_ms(RETARDO);
        }

        LATD = numeros[(*contHoraMostrar)] + 128;

        if (contadorBotonSet == 3) {
            if (repeticiones == 1) {
                LATA = 0b001000;
                __delay_ms(RETARDO);
            }
        } else {
            LATA = 0b001000;
            __delay_ms(RETARDO);
        }

        LATD = numeros[(*contDecHoraMostrar)];

        if (contadorBotonSet == 3) {
            if (repeticiones == 1) {
                LATA = 0b010000;
                __delay_ms(RETARDO);
            }
        } else {
            LATA = 0b010000;
            __delay_ms(RETARDO);
        }

        repeticiones--;
    }
}

void mostrarDigitos(void) {
    //MULTIPLEXACION

    LATD = numeros[contMin];
    LATA = 0b000010;
    __delay_ms(RETARDO);

    LATD = numeros[contDecMin];
    LATA = 0b000100;
    __delay_ms(RETARDO);

    LATD = numeros[(*contHoraMostrar)] + 128;
    LATA = 0b001000;
    __delay_ms(RETARDO);

    LATD = numeros[(*contDecHoraMostrar)];
    LATA = 0b010000;
    __delay_ms(RETARDO);

}

void verificaAmPm(void) {

    if (((contHora >= 0 && contHora <= 9)&&!contDecHora) || ((contHora >= 0 && contHora <= 1) && contDecHora == 1)) { //AM
        AmPm = 0;
        LATC = 1;
    } else { //PM
        AmPm = 1;
        LATC = 2;
    }
}

void convertirFormato(void) {

    unsigned int numeroEvaluar = (contDecHora * 10) + contHora;

    if (numeroEvaluar > 12) {
        numeroEvaluar -= 12;
        contDecHoraAux = (numeroEvaluar / 10) % 10;
        contHoraAux = numeroEvaluar % 10;

        contDecHoraMostrar = &contDecHoraAux;
        contHoraMostrar = &contHoraAux;
    } else if (!numeroEvaluar) {

        contDecHoraAux = 1;
        contHoraAux = 2;

        contDecHoraMostrar = &contDecHoraAux;
        contHoraMostrar = &contHoraAux;
    }

}

void main(void) {

    ADCON0bits.ADON = 1; //Encendiendo ADC
    ADCON1 = 0b00001110; //VSS REFERENCIA|TODOS DIGITALES MENOS AN0
    ADCON2 = 0b10100101; //TIEMPO DE ADQUISICION 8 TAD, JUSTIFICADO A LA DERECHA, FOSC/16

    TRISB = 1; //PUERTO B COMO ENTRADA  | LECTURA BOTONES
    TRISD = 0; //PUERTO D COMO SALIDA   | CONTROL DISPLAYS
    TRISA = 0b000001; //PUERTO A COMO ENTRADA  | LEER TEMPERATURA | CONTROL TRANSISTORES
    TRISC = 0; //PUERTO C COMO SALIDA PARA IDENTIFICAR CUANDO ES AM Y PM

    INTCONbits.GIE = 1; //Interrupciones Globales Activadas

    INTCONbits.TMR0IE = 1; //Interrupcion desbordamiento Timer 0
    INTCONbits.TMR0IF = 0; //Inicializar la bandera de interrupción Timer 0

    T0CON = 0b10000111; //Timer 0 encendido, 16 bits , Temporizador, Prescaler 1:256

    TMR0 = VALOR_TIMER0; //Inicializando Timer 0

    contDecHoraMostrar = &contDecHora;
    contHoraMostrar = &contHora;

    while (1) {

        if (PORTBbits.RB1&&!contadorBotonSet) { //BOTON MOSTRAR TEMPERATURA
            dameTemperatura();
        } else if (PORTBbits.RB2&&!contadorBotonSet) { //BOTON PARA CAMBIO DE FORMATO
            while (PORTBbits.RB2); //ANTIREBOTE
            formato = ~formato;
        } else if ((PORTBbits.RB0) || ((PORTBbits.RB1 || PORTBbits.RB2) && contadorBotonSet))
            controlBotones(); //BOTONES PARA CAMBIAR LA HORA

        controlContadores();

        if (!formato) {
            contDecHoraMostrar = &contDecHora;
            contHoraMostrar = &contHora;
            LATC = 0; //APAGAR INDICADOR DE AM|PM CUANDO EL FORMATO SEA DE 24 HRS.
        } else {
            verificaAmPm();
            convertirFormato();
        }

        if (!contadorBotonSet) {
            mostrarDigitos();
        } else {
            parpadearDigitos();
        }
    }
    return;
}
