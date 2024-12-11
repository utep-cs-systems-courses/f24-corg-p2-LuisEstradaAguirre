#include <msp430.h>
#include "libTimer.h"
#include "buzzer.h"

#define LED_RED BIT6               // P1.6
#define LED_GREEN BIT0             // P1.0
#define LEDS (BIT0 | BIT6)

#define SW1 BIT0                   // P2.0 for play/pause
#define SW2 BIT1                   // P2.1 for green LED control
#define SW3 BIT2                   // P2.2 for red LED control
#define SW4 BIT3                   // P2.3 for flashing both LEDs
#define SWITCHES (SW1 | SW2 | SW3 | SW4) // Using all four switches

volatile int play = 0;             // Flag to control play/pause state
volatile int flash = 0;            // Flag to control LED flashing

void main(void) {
    configureClocks();
    buzzer_init();

    P1DIR |= LEDS;
    P1OUT &= ~LEDS;                // LEDs initially off

    P2DIR &= ~SWITCHES;            // Set P2.0, P2.1, P2.2, P2.3 as input
    P2REN |= SWITCHES;             // Enable resistors for switches
    P2OUT |= SWITCHES;             // Pull-ups for switches
    P2IE |= SWITCHES;              // Enable interrupts for P2.0, P2.1, P2.2, P2.3
    P2IES |= SWITCHES;             // Trigger on high-to-low transition
    P2IFG &= ~SWITCHES;            // Clear interrupt flags for P2.0, P2.1, P2.2, P2.3

    __enable_interrupt();          // Enable global interrupts

    short period;
    short minPeriod = 500;
    short maxPeriod = 2000;

    while (1) {
        if (play) {  

            // Loud (high frequency)
            for (period = minPeriod; period <= maxPeriod; period += 100) {
                buzzer_set_period(period);
                __delay_cycles(250000); // Short delay for each change
            }

            // Soft (low frequency)
            for (period = maxPeriod; period >= minPeriod; period -= 100) {
                buzzer_set_period(period);
                __delay_cycles(250000); // Short delay for each change
            }
        } 

        else {
            buzzer_set_period(0); // Silence the buzzer when paused
        }

        if (flash) {  // Flash both LEDs when flash flag is set
            
            P1OUT ^= LEDS;        // Toggle both LEDs
            __delay_cycles(500000); // Delay for visible flashing
        }
    }
}

void switch_interrupt_handler() {
    char p2val = P2IN;          

    /* Update switch interrupt sense to detect changes from current state */
    P2IES |= (p2val & SWITCHES);   // If switch is up, sense down
    P2IES &= (p2val | ~SWITCHES);  // If switch is down, sense up

    if (!(p2val & SW1)) {          // Check if Sw1 (P2.0) is pressed (active low)
        play ^= 1;                 // Toggle play/pause state
    }

    if (!(p2val & SW2)) {          // Check if Sw2 (P2.1) is pressed (active low)
        P1OUT ^= LED_GREEN;        // Toggle the green LED
    }

    if (!(p2val & SW3)) {          // Check if Sw3 (P2.2) is pressed (active low)
        P1OUT ^= LED_RED;          // Toggle the red LED
    }

    if (!(p2val & SW4)) {          // Check if Sw4 (P2.3) is pressed (active low)
        flash ^= 1;                // Toggle the flash flag to start/stop flashing
        if (!flash) {
            P1OUT &= ~LEDS;        // Turn off both LEDs when stopping flash
        }
    }
}

void __interrupt_vec(PORT2_VECTOR) Port_2() {
    if (P2IFG & SWITCHES) {         // Check if any button caused the interrupt
        P2IFG &= ~SWITCHES;         // Clear pending switch interrupts
        switch_interrupt_handler(); // Call the single handler for switches
    }
}
