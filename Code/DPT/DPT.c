//=======================================================================================================================================================
// This code generates a user-configurable double pulse when the push-button connected to GPIO14 (Pin 74) is pulled low. 
// The double pulse is generated on GPIO0 (Pin 40), using the  DELAY_US(0.0); function. 
// Initially, the Red LED (GPIO34) toggles every 0.5 seconds until the push button conncted to GPIO14 is pressed. 
// Once the push-button connceted to GPIO14 is pressed, the Red LED stops toggling,a nd the Blue LED turns on permanently. 
// The code must be reset to generate a double pulse again using the GPIO push button once the Blue LED turns on.
//
// Change the delay times for the pulse in the code.
// 
// In the project properties, under Tools -> C2000 complier -> Predifiend symbols, check that _LAUNCHXL_F28379D is added to get correct delay time.
//
//=======================================================================================================================================================



#include "F28x_Project.h"

void main(void)
{
    // 1. Initialize System Control (PLL, Watchdog, Clocks)
    InitSysCtrl();

    // 2. Initialize GPIO
    InitGpio();

    // 3. Clear all interrupts and initialize PIE vector table
    DINT;
    InitPieCtrl();
    IER = 0x0000;
    IFR = 0x0000;
    InitPieVectTable();

    // ==========================================================
    // 4. Unconditional Flash-to-RAM Copy
    // ==========================================================
    memcpy(&RamfuncsRunStart, &RamfuncsLoadStart, (size_t)&RamfuncsLoadSize);
    InitFlash(); 

    // ==========================================================
    // 5. PIN CONFIGURATIONS
    // ==========================================================
    EALLOW;
    
    // GPIO0 (EPWM1A - Pulse Output)
    GpioCtrlRegs.GPAPUD.bit.GPIO0 = 1;    // Disable pull-up 
    GpioCtrlRegs.GPAMUX1.bit.GPIO0 = 0;   // 0 = General Purpose I/O
    GpioCtrlRegs.GPADIR.bit.GPIO0 = 1;    // 1 = Output direction
    GpioDataRegs.GPACLEAR.bit.GPIO0 = 1;  // Force low initially

    // GPIO14 (Push Button Input - Normally High, Active Low)
    GpioCtrlRegs.GPAPUD.bit.GPIO14 = 0;   // Enable internal pull-up (safety)
    GpioCtrlRegs.GPAMUX1.bit.GPIO14 = 0;  // 0 = General Purpose I/O
    GpioCtrlRegs.GPADIR.bit.GPIO14 = 0;   // 0 = Input direction

    // GPIO31 (Blue LED - Launchpad D10) -> PORT A
    GpioCtrlRegs.GPAPUD.bit.GPIO31 = 1;   // Disable pull-up
    GpioCtrlRegs.GPAMUX2.bit.GPIO31 = 0;  // 0 = General Purpose I/O
    GpioCtrlRegs.GPADIR.bit.GPIO31 = 1;   // 1 = Output direction
    GpioDataRegs.GPASET.bit.GPIO31 = 1;   // Set HIGH to turn OFF (Active Low)

    // GPIO34 (Red LED - Launchpad D9) -> PORT B
    GpioCtrlRegs.GPBPUD.bit.GPIO34 = 1;   // Disable pull-up
    GpioCtrlRegs.GPBMUX1.bit.GPIO34 = 0;  // 0 = General Purpose I/O
    GpioCtrlRegs.GPBDIR.bit.GPIO34 = 1;   // 1 = Output direction
    GpioDataRegs.GPBSET.bit.GPIO34 = 1;   // Set HIGH to turn OFF (Active Low)
    
    EDIS;

    // ==========================================================
    // 6. PRE-TRIGGER STATE: Blink Red, Wait for Button
    // ==========================================================
    uint32_t blink_timer = 0;

    // Loop runs continuously as long as GPIO14 is HIGH (Button not pressed)
    while(GpioDataRegs.GPADAT.bit.GPIO14 == 1)
    {
        DELAY_US(1000.0); // Wait 1 millisecond
        blink_timer++;    // Increase timer

        // Toggle the Red LED every 500ms (1 second total period)
        if(blink_timer >= 500)
        {
            GpioDataRegs.GPBTOGGLE.bit.GPIO34 = 1; // Toggle Red LED
            blink_timer = 0;                       // Reset timer
        }
    }

/*
    // ==========================================================
    // 7. BUTTON PRESSED: Update LEDs and Fire Pulse
    // ==========================================================
    
    // Ensure Red LED is permanently OFF
    GpioDataRegs.GPBSET.bit.GPIO34 = 1;   

    // Turn Blue LED permanently ON
    GpioDataRegs.GPACLEAR.bit.GPIO31 = 1; 

    // Initial blanking time: 50us 
    DELAY_US(50.0);

    // First Pulse: 5us
    GpioDataRegs.GPASET.bit.GPIO0 = 1;    // Drive High
    DELAY_US(5.0);

    // Intermediate blanking time: 3us
    GpioDataRegs.GPACLEAR.bit.GPIO0 = 1;  // Drive Low
    DELAY_US(3.0);

    // Second Pulse: 2us
    GpioDataRegs.GPASET.bit.GPIO0 = 1;    // Drive High
    DELAY_US(2.0);

    // End of sequence: Keep low indefinitely
    GpioDataRegs.GPACLEAR.bit.GPIO0 = 1;  // Drive Low
*/    


    // ==========================================================
    // 7. BUTTON PRESSED: Update LEDs and Fire Pulse
    // ==========================================================
    
    // Ensure Red LED is permanently OFF
    GpioDataRegs.GPBSET.bit.GPIO34 = 1;   

    // Turn Blue LED permanently ON
    GpioDataRegs.GPACLEAR.bit.GPIO31 = 1; 

    // Initial blanking time: 50us (Large enough that DELAY_US is fine)
    DELAY_US(50.0);

    // ==========================================================
    // NANOSECOND PRECISION PULSE
    // CPU runs at 200 MHz (1 cycle = 5 ns)
    // Formula: Cycles needed = (Target ns) / 5
    // Assembly: __asm(" RPT #N || NOP"); delays for N + 2 cycles.
    // MAX VALUE FOR N IS 255!
    // ==========================================================

    // First Pulse: 2500ns (Target: 2500ns/5ns = 500 cycles)
    // Since 496 > 255, we must split the delay into two instructions.
    // 2 cycles (GPIO write) + 250 cycles (RPT 1) + 248 cycles (RPT 2) = 500 cycles.
    GpioDataRegs.GPASET.bit.GPIO0 = 1;    // Drive High
    __asm(" RPT #248 || NOP");            // Hardware delay 1250ns (250*5ns)
    __asm(" RPT #246 || NOP");            // Hardware delay 1240ns (248*5ns)

    // Intermediate blanking time: 1000ns (Target: 1000ns/5ns = 200 cycles)
    // Using N=196 (Valid, < 255). 200 - 2 (GPIO) = 198. 198 = N + 2 => N=196.
    GpioDataRegs.GPACLEAR.bit.GPIO0 = 1;  // Drive Low
    __asm(" RPT #196 || NOP");            // Hardware delay 990ns

    // Second Pulse: 500ns (Target: 500ns/5ns = 100 cycles)
    // Using N=96 (Valid, < 255). 100 - 2 (GPIO) = 98. 98 = N + 2 => N=96.
    GpioDataRegs.GPASET.bit.GPIO0 = 1;    // Drive High
    __asm(" RPT #96 || NOP");             // Hardware delay 490ns

    // End of sequence: Keep low indefinitely
    GpioDataRegs.GPACLEAR.bit.GPIO0 = 1;  // Drive Low


    // ==========================================================
    // END OF SEQUENCE
    // ==========================================================

    // 8. Main application loop
    // Because this loop is empty and infinite, pressing the button 
    // again will do absolutely nothing until the board is physically reset.
    while(1)
    {
        // Program locks here safely
    }
}