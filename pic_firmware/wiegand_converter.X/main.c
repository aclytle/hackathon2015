/* 
 * File:   main.c
 * Author: alytle
 *
 * Created on February 7, 2015, 3:34 PM
// */
//
#pragma config FPLLIDIV = DIV_2         // PLL Input Divider (2x Divider)
#pragma config FPLLMUL = MUL_20         // PLL Multiplier (20x Multiplier)
#pragma config FPLLODIV = DIV_1         // System PLL Output Clock Divider (PLL Divide by 1)
//
//// DEVCFG1
#pragma config FNOSC = PRIPLL           // Oscillator Selection Bits (Primary Osc w/PLL (XT+,HS+,EC+PLL))
#pragma config FSOSCEN = OFF            // Secondary Oscillator Enable (Disabled)
#pragma config IESO = OFF               // Internal/External Switch Over (Disabled)
#pragma config POSCMOD = XT             // Primary Oscillator Configuration (External clock mode)
//#pragma config OSCIOFNC = ON            // CLKO Output Signal Active on the OSCO Pin (Enabled)
#pragma config FPBDIV = DIV_8           // Peripheral Clock Divisor (Pb_Clk is Sys_Clk/8)
//#pragma config FCKSM = CSDCMD           // Clock Switching and Monitor Selection (Clock Switch Disable, FSCM Disabled)
//#pragma config WDTPS = PS1048576        // Watchdog Timer Postscaler (1:1048576)
#pragma config FWDTEN = OFF             // Watchdog Timer Enable (WDT Disabled (SWDTEN Bit Controls))
//
//// DEVCFG0
//#pragma config DEBUG = OFF              // Background Debugger Enable (Debugger is disabled)
//#pragma config ICESEL = ICS_PGx2        // ICE/ICD Comm Channel Select (ICE EMUC2/EMUD2 pins shared with PGC2/PGD2)
//#pragma config PWP = OFF                // Program Flash Write Protect (Disable)
//#pragma config BWP = OFF                // Boot Flash Write Protect bit (Protection Disabled)
//#pragma config CP = OFF                 // Code Protect (Protection Disabled)
//

#define PCLOCK 10000000
#define UCLOCK 9600
#define WIEGAND_DELAY 2000
#define GRANTED_BEEPS 15
#define GRANTED_BEEP_ON_DELAY 2500
#define GRANTED_BEEP_OFF_DELAY 2500
#define DENIED_BEEPS 3
#define DENIED_BEEP_ON_DELAY 30000
#define DENIED_BEEP_OFF_DELAY 20000

#include <stdio.h>
#include <stdlib.h>
#include <peripheral/uart.h>
#include <peripheral/int.h>
#include <peripheral/timer.h>
#include <peripheral/ports.h>
#include <xc.h>
#include <inttypes.h>


/*
 * 
 */

static volatile unsigned int wiegand_buffer = 0;
static volatile unsigned int wiegand_bit = 0;
static volatile char access_feedback = 'g'; // Access granted sequence on power on

int init_ports() {

    mPORTGSetPinsDigitalOut(BIT_6); // LED
    mPORTFSetPinsDigitalOut(BIT_0); // LED
    mPORTEOpenDrainOpen(BIT_0 | BIT_1 | BIT_2); // Buzzer, Green LD, Red LD
    mPORTGWrite(BIT_6); // Initially on
    mPORTFWrite(BIT_0);
    mPORTEWrite(BIT_0 | BIT_1 | BIT_2);

}

int init_uart() {
    UARTConfigure(UART1,0);
    UARTSetDataRate(UART1, PCLOCK, UCLOCK);

    INTEnable(INT_U1RX, INT_ENABLED);
    INTSetVectorPriority(INT_UART_1_VECTOR, INT_PRIORITY_LEVEL_2);
    
    UARTEnable(UART1, UART_ENABLE_FLAGS(UART_PERIPHERAL| UART_TX | UART_RX));

}

int init_ints() {

    ConfigINT3(EXT_INT_ENABLE | EXT_INT_PRI_2 | FALLING_EDGE_INT);
    ConfigINT4(EXT_INT_ENABLE | EXT_INT_PRI_2 | FALLING_EDGE_INT);

    INTConfigureSystem(INT_SYSTEM_CONFIG_MULT_VECTOR);
    INTEnableInterrupts();
}



int init_timers() {
    ConfigIntTimer1(T1_INT_ON | T1_INT_PRIOR_1); // Timer 1 is the Wiegand
                                                 // timeout delay. See ISR
    OpenTimer1(T1_ON | T1_INT_ON | T1_PS_1_256, WIEGAND_DELAY);
    OpenTimer2(T2_ON | T2_INT_OFF | T2_PS_1_256, 40000);
}
/*
 Writes 4 bytes to UART1 as hex characters
 */
int write_wiegand_uart(unsigned int food) {
    char temp[6];
    int i;
    sprintf(temp, "%04X", food);
    temp[4]='\n';
    //temp[5]='\r'; // Python doesn't like carriage returns
    for (i=0; i<5; i++) {
        while(!UARTTransmitterIsReady(UART1));
        U1TXREG = temp[i];
    }
    while(!UARTTransmitterIsReady(UART1));
    //access_denied();
}

/*
 Plays buzzer pattern and turns LED on
 */
int access_granted() {
    int i;

    mPORTEClearBits(BIT_2); // green led on
    for(i=0; i<GRANTED_BEEPS; i++) {
        mPORTEClearBits(BIT_0); // Buzzer on
        TMR2 = 0;
        while(TMR2 < GRANTED_BEEP_ON_DELAY);
        mPORTESetBits(BIT_0);  // Buzzer off
        TMR2 = 0;
        while(TMR2 < GRANTED_BEEP_OFF_DELAY);
    }
    mPORTESetBits(BIT_2); // green led off
}

int access_denied() {
    int i;

    mPORTEClearBits(BIT_1); // red led on
    for(i=0; i<DENIED_BEEPS; i++) {
        mPORTEClearBits(BIT_0); // Buzzer on
        TMR2 = 0;
        while(TMR2 < DENIED_BEEP_ON_DELAY);
        mPORTESetBits(BIT_0);  // Buzzer off
        TMR2 = 0;
        while(TMR2 < DENIED_BEEP_OFF_DELAY);
    }
    mPORTESetBits(BIT_1); // red led off
}

int main(int argc, char** argv) {

    init_ports();
    init_uart();
    init_ints();
    init_timers();

    
    while(1) {
        if(access_feedback == 'g') {
            access_granted();
            access_feedback = 0;
        }
        else if(access_feedback == 'd') {
            access_denied();
            access_feedback = 0;
        }
    }
    return (EXIT_SUCCESS);
}

// data_1 line
void __ISR(_EXTERNAL_3_VECTOR, IPL2) extint3_handler(void) {
    mPORTGSetBits(BIT_6);
    wiegand_buffer = wiegand_buffer | (1 << wiegand_bit);
    wiegand_bit++;
    TMR1=0;
    mINT3ClearIntFlag();
}

// data_0 line
void __ISR(_EXTERNAL_4_VECTOR, IPL2) extint4_handler(void) {
    mPORTGSetBits(BIT_6);
    wiegand_bit++;
    TMR1 = 0;
    mINT4ClearIntFlag();
}

void __ISR(_TIMER_1_VECTOR, IPL1) timer1_handler(void) {
    mPORTFToggleBits(BIT_0);
    mPORTGClearBits(BIT_6);
    INTClearFlag(INT_T1);
    //U1TXREG = 'Q';
    //write_wiegand_uart(0x1234);
    // Timer has expired and buffer is not empty, write uart, clear buffer
    if(wiegand_buffer) {
        write_wiegand_uart(wiegand_buffer);
        wiegand_buffer = 0;
        wiegand_bit = 0;
    }
}

void __ISR(_UART1_VECTOR, IPL2) uart1_handler(void) {
    char temp;
    if (INTGetFlag(INT_U1RX)) {
        temp = UARTGetDataByte(UART1);
        if (temp == 'g') { // Access Granted
            access_feedback = 'g';
        }
        else if (temp == 'd') {
            access_feedback = 'd';
        }
        INTClearFlag(INT_U1RX);
    }
    if (INTGetFlag(INT_U1TX)) {
        INTClearFlag(INT_U1TX);
    }
    INTClearFlag(INT_UART1);
}