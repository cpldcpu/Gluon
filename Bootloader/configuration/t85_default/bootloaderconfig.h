/* Name: bootloaderconfig.h
 * This file (together with some settings in Makefile.inc) configures the boot loader
 * according to the hardware.
 * 
 * Controller type: ATtiny 85 - 16 MHz
 * Configuration:   Default configuration
 *       USB D- :   PB3
 *       USB D+ :   PB4
 *       Entry  :   Always
 *       LED    :   None
 *       OSCCAL :   Stays at 16 MHz
 * Note: Uses 16.5 MHz V-USB implementation with PLL
 * Last Change:     Mar 10,2016
 *
 * License: GNU GPL v2 (see License.txt
 */
#ifndef __bootloaderconfig_h_included__
#define __bootloaderconfig_h_included__



/* ---------------------- feature / code size options ---------------------- */
/*               Configure the behavior of the bootloader here               */
/* ------------------------------------------------------------------------- */

/*
 *  Define Bootloader entry condition
 * 
 *  If the entry condition is not met, the bootloader will not be activated and the user program
 *  is executed directly after a reset. If no user program has been loaded, the bootloader
 *  is always active.
 * 
 *  ENTRY_ALWAYS        Always activate the bootloader after reset. Requires the least
 *                      amount of code.
 *
 *  ENTRY_WATCHDOG      Activate the bootloader after a watchdog reset. This can be used
 *                      to enter the bootloader from the user program.
 *                      Adds 22 bytes.
 *
 *  ENTRY_EXT_RESET     Activate the bootloader after an external reset was issued by 
 *                      pulling the reset pin low. It may be necessary to add an external
 *                      pull-up resistor to the reset pin if this entry method appears to
 *                      behave unreliably.
 *                      Adds 22 bytes.
 *
 *  ENTRY_JUMPER        Activate the bootloader when a specific pin is pulled low by an 
 *                      external jumper. 
 *                      Adds 34 bytes.
 *
 *       JUMPER_PIN     Pin the jumper is connected to. (e.g. PB0)
 *       JUMPER_PORT    Port out register for the jumper (e.g. PORTB)  
 *       JUMPER_DDR     Port data direction register for the jumper (e.g. DDRB)  
 *       JUMPER_INP     Port inout register for the jumper (e.g. PINB)  
 * 
 */

#define ENTRYMODE ENTRY_ALWAYS

#define JUMPER_PIN    PB0
#define JUMPER_PORT   PORTB 
#define JUMPER_DDR    DDRB 
#define JUMPER_INP    PINB 
 
/*
  Internal implementation, don't change this unless you want to add an entrymode.
*/ 
 
#define ENTRY_ALWAYS    1
#define ENTRY_WATCHDOG  2
#define ENTRY_EXT_RESET 3
#define ENTRY_JUMPER    4

#if ENTRYMODE==ENTRY_ALWAYS
  #define bootLoaderInit()
  #define bootLoaderExit()
  #define bootLoaderStartCondition() 1
#elif ENTRYMODE==ENTRY_WATCHDOG
  #define bootLoaderInit()
  #define bootLoaderExit()
  #define bootLoaderStartCondition() (MCUSR&_BV(WDRF))
#elif ENTRYMODE==ENTRY_EXT_RESET
  #define bootLoaderInit()
  #define bootLoaderExit()
  #define bootLoaderStartCondition() (MCUSR&_BV(EXTRF))
#elif ENTRYMODE==ENTRY_JUMPER
  // Enable pull up on jumper pin and delay to stabilize input    
  #define bootLoaderInit()   {JUMPER_DDR&=~_BV(JUMPER_PIN);JUMPER_PORT|=_BV(JUMPER_PIN);_delay_ms(1);}
  #define bootLoaderExit()   {JUMPER_PORT&=~_BV(JUMPER_PIN);}
  #define bootLoaderStartCondition() (!(JUMPER_INP&_BV(JUMPER_PIN)))
#else
   #error "No entry mode defined"
#endif

/*
 * Define bootloader timeout value. 
 * 
 *  The bootloader will only time out if a user program was loaded.
 * 
 *  AUTO_EXIT_NO_USB_MS        The bootloader will exit after this delay if no USB is connected.
 *                             Set to 0 to disable
 *                             Adds ~6 bytes.
 *                             (This will wait for an USB SE0 reset from the host)
 *
 *  AUTO_EXIT_MS               The bootloader will exit after this delay if no USB communication
 *                             from the host tool was received.
 *                             Set to 0 to disable
 *  
 *  All values are approx. in milliseconds
 */

#define AUTO_EXIT_NO_USB_MS    0
#define AUTO_EXIT_MS           6000
  
/*  
 *  Defines handling of an indicator LED while the bootloader is active.  
 * 
 *  LED_MODE                  Define behavior of attached LED or suppress LED code.
 *
 *          NONE              Do not generate LED code (gains 18 bytes).
 *          ACTIVE_HIGH       LED is on when output pin is high. This will toggle bettwen 1 and 0.
 *          ACTIVE_LOW        LED is on when output pin is low.  This will toggle between Z and 0.
 *
 *  LED_DDR,LED_PORT,LED_PIN  Where is your LED connected?
 *
 */ 

#define LED_MODE    NONE

#define LED_DDR     DDRB
#define LED_PORT    PORTB
#define LED_PIN     PB1

/*
 *  This is the implementation of the LED code. Change the configuration above unless you want to 
 *  change the led behavior
 *
 *  LED_INIT                  Called once after bootloader entry
 *  LED_EXIT                  Called once during bootloader exit
 *  LED_MACRO                 Called in the main loop with the idle counter as parameter.
 *                            Use to define pattern.
*/

#define NONE        0
#define ACTIVE_HIGH 1
#define ACTIVE_LOW  2

#if LED_MODE==ACTIVE_HIGH
  #define LED_INIT(x)   LED_DDR   |= _BV(LED_PIN); 
  #define LED_EXIT(x)   {LED_DDR  &=~_BV(LED_PIN);LED_PORT  &=~_BV(LED_PIN);}
  #define LED_MACRO(x)  if ( x & 0x4c ) {LED_PORT&=~_BV(LED_PIN);} else {LED_PORT|=_BV(LED_PIN);}
#elif LED_MODE==ACTIVE_LOW
  #define LED_INIT(x)   LED_PORT &=~_BV(LED_PIN);   
  #define LED_EXIT(x)   LED_DDR  &=~_BV(LED_PIN);
  #define LED_MACRO(x)  if ( x & 0x4c ) {LED_DDR&=~_BV(LED_PIN);} else {LED_DDR|=_BV(LED_PIN);}  
#elif LED_MODE==NONE
  #define LED_INIT(x)
  #define LED_EXIT(x)
  #define LED_MACRO(x)
#endif

/* --------------------------------------------------------------------------- */
/* Micronucleus internal configuration. Do not change anything below this line */
/* --------------------------------------------------------------------------- */

// Microcontroller vectortable entries in the flash
#define RESET_VECTOR_OFFSET         0

// number of bytes before the boot loader vectors to store the tiny application vector table
#define TINYVECTOR_RESET_OFFSET     2

/* ------------------------------------------------------------------------ */
// postscript are the few bytes at the end of programmable memory which store tinyVectors
#define POSTSCRIPT_SIZE 2
#define PROGMEM_SIZE (BOOTLOADER_ADDRESS - POSTSCRIPT_SIZE) /* max size of user program */

#endif /* __bootloader_h_included__ */
