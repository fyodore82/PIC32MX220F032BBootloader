/*********************************************************************
 *
 *                  PIC32 Boot Loader
 *
 *********************************************************************
 * FileName:        Bootloader.c
 * Dependencies:
 * Processor:       PIC32
 *
 * Compiler:        MPLAB C32
 *                  MPLAB IDE
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 *
 * The software supplied herewith by Microchip Technology Incorporated
 * (the �Company�) for its PIC32 Microcontroller is intended
 * and supplied to you, the Company�s customer, for use solely and
 * exclusively on Microchip PIC32 Microcontroller products.
 * The software is owned by the Company and/or its supplier, and is
 * protected under applicable copyright laws. All rights are reserved.
 * Any use in violation of the foregoing restrictions may subject the
 * user to criminal sanctions under applicable laws, as well as to
 * civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN �AS IS� CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *
 * $Id:  $
 * $Name: $
 *
 **********************************************************************/
#include "Include\GenericTypeDefs.h"
#include "Include\HardwareProfile\HardwareProfile.h"
#include "Include\Bootloader.h"
#include "Include\FrameWork\Framework.h"
#include <stdlib.h>
// #include <plib.h>
#include <xc.h>
#include "Include\addressConvertion.h"

// DEVCFG0
#pragma config CP = 1
#pragma config BWP = OFF                // Boot write protect: OFF
// #pragma config PWP = 0b111111111       
#pragma config ICESEL = ICS_PGx3    // ICE pins configured on PGx1 (PGx2 is multiplexed with USB D+ and D- pins).
#pragma config JTAGEN = 0
// #pragma config DEBUG = 00        // Debug can be enabled by MPLAB
#pragma config DEBUG    = OFF           // Background Debugger disabled

// DEVCFG1
#pragma config FWDTEN = OFF             // WD timer: OFF
#pragma config FCKSM = 0b11               // Disabled
#pragma config FPBDIV = DIV_1           // Peripheral Bus Clock: Divide by 1
#pragma config OSCIOFNC = 0             // OLKO is disabled
#pragma config POSCMOD = HS             // Primary Oscillator Mode: High Speed xtal
#pragma config IESO = 0                 // Internal External Switchover mode is disabled
#pragma config FSOSCEN = 0              // Disable Secondary Oscillator
#pragma config FNOSC = PRIPLL           // Oscillator Selection: Primary oscillator  w/ PLL

// DEVCFG2
#pragma config FPLLODIV = DIV_2         // PLL Output Divider: Divide by 2 (80 / 2 = 40)
// Configuring the Device Configuration Registers
// 80Mhz Core/Periph, Pri Osc w/PLL, Write protect Boot Flash
#pragma config UPLLEN   = ON        // USB PLL Enabled
#pragma config UPLLIDIV = DIV_5     // USB PLL Input Divider = Divide by 5 (20 / 5 = 4)

#pragma config FPLLMUL = MUL_20         // PLL Multiplier: Multiply by 20 (4 * 20 = 80))
#pragma config FPLLIDIV = DIV_5         // PLL Input Divider:  Divide by 5 (20 / 5 = 4)

// DEVCFG3
#pragma config FVBUSONIO = 0        // pin 25: VBUSON pin is controlled by the port function
#pragma config FUSBIDIO = 0         // pin 14: USBID pin is controlled by the port function

#pragma config IOL1WAY = ON
#pragma config PMDL1WAY = ON

#define SWITCH_PRESSED 0

#ifdef __cplusplus
extern "C" {
#endif
BOOL CheckTrigger(void);
void JumpToApp(void);
BOOL ValidAppPresent(void);
#ifdef __cplusplus
}
#endif

    /*********************************************************************
 * Function:       unsigned int SYSTEMConfig(unsigned int sys_clock, unsigned int flags)
 *
 * PreCondition:    None
 *
 * Input:           sys_clock - system clock in Hz
 *                  flags -
 *                  -    SYS_CFG_WAIT_STATES  - configure the flash wait states from the system clock
 *                  -    SYS_CFG_PB_BUS       - configure the PB bus from the system clock
 *                  -    SYS_CFG_PCACHE      - configure the pCache (if used)
 *                  -    SYS_CFG_ALL          - configure all based off of system clock
 *
 * Output:          the PB clock frequency
 *
 * Side Effects:    Could set the wait state, pb bus and turn on the pre-fetch buffer and cache. Sets the RAM
 *                  wait state to 0.
 *
 * Overview:	    The function sets the PB divider, the Flash Wait states and the DRM wait states to the optimum value.
 *                  It also enables the cacheability for the K0 segment.
 *
 * Note:            The interrupts are disabled shortly, the DMA is suspended and the system is unlocked while performing the operation.
 *                  Upon return the previous status of the interrupts and the DMA are restored. The system is re-locked.
 *
 * Example:	    SYSTEMConfig(72000000, SYS_CFG_ALL);
 ********************************************************************/
extern inline unsigned int __attribute__((always_inline)) SYSTEMConfig(unsigned int sys_clock, unsigned int flags)
{
    unsigned int pb_clk;
    unsigned int int_status;
#ifdef _PCACHE
    unsigned int cache_status;
#endif

    int_status=INTDisableInterrupts();

    mBMXDisableDRMWaitState();

//    if(flags & SYS_CFG_WAIT_STATES)
//    {
//        SYSTEMConfigWaitStates(sys_clock);
//    }

//    if(flags & SYS_CFG_PB_BUS)
//    {
//        SYSTEMConfigPB(sys_clock);
//    }


#ifdef _PCACHE
    if(flags & SYS_CFG_PCACHE)
    {
        cache_status = mCheGetCon();
        cache_status |= CHE_CONF_PF_ALL;
        mCheConfigure(cache_status);
        CheKseg0CacheOn();
    }
#endif

    pb_clk = sys_clock;
    pb_clk >>= OSCCONbits.PBDIV;

    INTRestoreInterrupts(int_status);

    return pb_clk;

}

#define _XTAL_FREQ   40000000UL

void __delay_ms(UINT8 ms) {
  uint32_t start_count = _CP0_GET_COUNT();
  uint32_t now_count;
  uint32_t cycles = ((_XTAL_FREQ + 1000000U) / 2000U) * ms;
  do {
    now_count = _CP0_GET_COUNT();
  } while ((uint32_t)(now_count-start_count) < cycles);
}

void playNote(UINT8 note, UINT8 delay) {
  BEEPER_CTRL_OUT = 1;
  for (UINT8 beep = 0; beep < delay; beep++) {
    BEEPER_CTRL_OUT = !BEEPER_CTRL_OUT;
    __delay_ms(note);
  }
  BEEPER_CTRL_OUT = 0;
}

/********************************************************************
* Function: 	main()
*
* Precondition: 
*
* Input: 		None.
*
* Output:		None.
*
* Side Effects:	None.
*
* Overview: 	Main entry function. If there is a trigger or 
*				if there is no valid application, the device 
*				stays in firmware upgrade mode.
*
*			
* Note:		 	None.
********************************************************************/
INT main(void)
{
	UINT pbClk;
  
    // ------------ init ports -----------------
  PORTA = 0;
  PORTB = 0;
  LATA = 0;
  LATB = 0;

  // 12.2.4 ODCx. ODCx for all ports are 0 on reset = normal digital output, NOT open-drain

  // 12.2.5 ANSELx. Set all ports to digital.
  ANSELA = 0x0;
  ANSELB = 0x0;
  
    // V pin 26 BEEPER_CTRL_OUT
  TRISBbits.TRISB15 = 0;
  BEEPER_CTRL_OUT = 0;
  
  // If Btn was pressed during at least 30ms at the start
  __delay_ms(10);
  UINT8 btnPressed = 0;
  UINT8 btn = BUTTON_IN;
  btn = BUTTON_IN;
  while (!btn && btnPressed <= 3) {
    btnPressed++;
    __delay_ms(10);
    btn = BUTTON_IN;
  }
  
	// Setup configuration
	pbClk = SYSTEMConfig(SYS_FREQ, SYS_CFG_WAIT_STATES | SYS_CFG_PCACHE);
	
  UINT8 swr = RCONbits.SWR;
  UINT8 validApp = ValidAppPresent();
	if(!validApp || (validApp && swr) || btnPressed >= 3)
	{
    playNote(3, 90);
    playNote(1, 90);
    playNote(2, 90);
    
		// Initialize the transport layer - UART/USB/Ethernet
		TRANS_LAYER_Init(pbClk);
		
		while(!FRAMEWORK_ExitFirmwareUpgradeMode()) // Be in loop till framework recieves "run application" command from PC
		{
			// Enter firmware upgrade mode.
			// Be in loop, looking for commands from PC
			TRANS_LAYER_Task(); // Run Transport layer tasks
			FRAMEWORK_FrameWorkTask(); // Run frame work related tasks (Handling Rx frame, process frame and so on)
			// Blink LED (Indicates the user that bootloader is running).
			// BlinkLED();	
		}
		// Close trasnport layer.
		TRANS_LAYER_Close();
	}

	
	// No trigger + valid application = run application.
	JumpToApp();
	
	return 0;
}			


/********************************************************************
* Function: 	CheckTrigger()
*
* Precondition: 
*
* Input: 		None.
*
* Output:		TRUE: If triggered
				FALSE: No trigger
*
* Side Effects:	None.
*
* Overview: 	Checks if there is a trigger to enter 
				firmware upgrade mode.
*
*			
* Note:		 	None.
********************************************************************/
/* BOOL  CheckTrigger(void)
{
	UINT SwitchStatus;
	SwitchStatus = ReadSwitchStatus();
	if(SwitchStatus == SWITCH_PRESSED)
	{
		// Switch is pressed
		return TRUE;		
	}	
	else
	{
		// Switch is not pressed.
		return FALSE;	
	}	
}	*/


/********************************************************************
* Function: 	JumpToApp()
*
* Precondition: 
*
* Input: 		None.
*
* Output:		
*
* Side Effects:	No return from here.
*
* Overview: 	Jumps to application.
*
*			
* Note:		 	None.
********************************************************************/
void JumpToApp(void)
{	
	void (*fptr)(void);
	fptr = (void (*)(void))USER_APP_RESET_ADDRESS;
	fptr();
}	


/********************************************************************
* Function: 	ValidAppPresent()
*
* Precondition: 
*
* Input: 		None.
*
* Output:		TRUE: If application is valid.
*
* Side Effects:	None.
*
* Overview: 	Logic: Check application vector has 
				some value other than "0xFFFFFFFF"
*
*			
* Note:		 	None.
********************************************************************/
BOOL ValidAppPresent(void)
{
	DWORD *AppPtr;
	AppPtr = (DWORD *)USER_APP_RESET_ADDRESS;
	if(*AppPtr == 0xFFFFFFFF)
	{
		return FALSE;
	}
	else
	{
		return TRUE;
	}
}			


/*********************End of File************************************/


