
// From old code
unsigned int __attribute__((nomips16)) INTDisableInterrupts(void)
{
    unsigned int status = 0;

    asm volatile("di    %0" : "=r"(status));

    return status;
}
        
 /*********************************************************************
 * Function:        INTRestoreInterrupts(unsigned int status)
 *
 * PreCondition:    None
 *
 * Input:           value of the status registor
 *
 * Output:
 *
 * Side Effects:    Interrupts are restored to previous state
 *
 * Overview:        Interrupts are enabled by setting the IE bit
 *                  in the status register
 ********************************************************************/
void __attribute__((nomips16))  INTRestoreInterrupts(unsigned int status)
{
    if(status & 0x00000001)
        asm volatile("ei");
    else
        asm volatile("di");
}

/******************************************************************************
 * Function:        unsigned int ReadCoreTimer(void)
 *
 * Description:		Returns the current Core Timer value.
 *
 * PreCondition:    None
 *
 * Inputs:			None
 *
 * Output:          Count value.
 *
 * Example:			ReadCoreTimer()
 *
 *****************************************************************************/
unsigned int __attribute__((nomips16)) ReadCoreTimer(void)
{
    unsigned int timer;

    // get the count reg
    asm volatile("mfc0   %0, $9" : "=r"(timer));

    return timer;
}

/******************************************************************************
 * Function:        void WriteCoreTimer(unsigned int value)
 *
 * Description:		Loads 32-bit value into CoreTimer.
 *
 * PreCondition:    None
 *
 * Inputs:			value: The value to be loaded.
 *
 * Output:          None
 *
 * Example:			WriteCoreTimer(0x1234)
 *
 *****************************************************************************/
void __attribute__((nomips16)) WriteCoreTimer(unsigned int timer)
{
    /* get the count reg */
    asm volatile("mtc0   %0, $9": "+r"(timer));

}