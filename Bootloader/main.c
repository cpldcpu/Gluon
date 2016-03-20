/* 
 * Project: Gluon-Bootload -  v2.3
 *
 * License: GNU GPL v2 (see License.txt)
 */
 
#define MICRONUCLEUS_VERSION_MAJOR 2
#define MICRONUCLEUS_VERSION_MINOR 3


#define RX PA3
#define TX PA4
#define BAUD 57600


#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <util/delay.h>

#ifndef __AVR_ATtiny104__
#include <avr/boot.h>
#endif

#include "bootloaderconfig.h"
//#include "bootloadasm.h"
// #include "usbdrv/usbdrv.c"

// verify the bootloader address aligns with page size
#if (defined __AVR_ATtiny841__)||(defined __AVR_ATtiny441__)  
  #if BOOTLOADER_ADDRESS % ( SPM_PAGESIZE * 4 ) != 0
    #error "BOOTLOADER_ADDRESS in makefile must be a multiple of chip's pagesize"
  #endif
#else
  #if BOOTLOADER_ADDRESS % SPM_PAGESIZE != 0
    #error "BOOTLOADER_ADDRESS in makefile must be a multiple of chip's pagesize"
  #endif  
#endif

#if SPM_PAGESIZE>256
  #error "Micronucleus only supports pagesizes up to 256 bytes"
#endif

#if ((AUTO_EXIT_MS>0) && (AUTO_EXIT_MS<1000))
  #error "Do not set AUTO_EXIT_MS to below 1s to allow Micronucleus to function properly"
#endif

// Device configuration reply
// Length: 6 bytes
//   Byte 0:  Bootloader version. High and low nibble.
//   Byte 1:  User program memory size, low byte   
//   Byte 2:  Flash Pagesize in bytes

  typedef union {
    uint16_t w;
    uint8_t b[2];
  } uint16_union_t;

#ifdef __AVR_ATtiny104__
uint16_union_t currentAddress; 
#else
register uint16_union_t currentAddress  asm("r4");  // r4/r5 current progmem address, used for erasing and writing 
#endif

static    uint8_t  *buffer=(uint8_t*)RAMSTART;
static    uint16_t *bufword=(uint16_t*)RAMSTART;

// command system schedules functions to run in the main loop
enum {
  cmd_local_nop=0, 
  cmd_device_info=0,
  cmd_transfer_page=1,
  cmd_erase_application=2,
  cmd_write_data=3,
  cmd_exit=4,
  cmd_write_page=64  // internal commands start at 64
};

// Definition of sei and cli without memory barrier keyword to prevent reloading of memory variables
#define sei() asm volatile("sei")
#define cli() asm volatile("cli")
#define nop() asm volatile("nop")
#define wdr() asm volatile("wdr")
#define spm(adr)     __asm__ __volatile__      \
                     (   "spm\n\t"             \
                      :                        \
                      : "z" ((uint16_t)adr)); 

/* ------------------------------------------------------------------------ */
static inline void eraseApplication(void);
static void writeFlashPage(void);
static void writeWordToPageBuffer(uint16_t data);
static uint8_t usbFunctionSetup(uint8_t data[8]);
static inline void leaveBootloader(void);


#ifdef __AVR_ATtiny104__

static inline void eraseApplication(void) 
{

}

static void writeWordToPageBuffer(uint16_t data) 
{
}


#else

// erase all pages until bootloader, in reverse order (so our vectors stay in place for as long as possible)
// to minimise the chance of leaving the device in a state where the bootloader wont run, if there's power failure
// during upload
static inline void eraseApplication(void) {
  uint16_t ptr = BOOTLOADER_ADDRESS;

  while (ptr) {
#if (defined __AVR_ATtiny841__)||(defined __AVR_ATtiny441__)    
    ptr -= SPM_PAGESIZE * 4;        
#else
    ptr -= SPM_PAGESIZE;        
#endif    

    __SPM_REG=(__BOOT_PAGE_ERASE|_BV(__SPM_ENABLE)|_BV(CTPB));
    spm(ptr);
  }
 
  // Reset address to ensure the reset vector is written first.
  currentAddress.w = 0;   
}

// Write a word into the page buffer.
// Will patch the bootloader reset vector into the main vectortable to ensure
// the device can not be bricked. 
static void writeWordToPageBuffer(uint16_t data) {

#ifndef ENABLE_UNSAFE_OPTIMIZATIONS     
  #if BOOTLOADER_ADDRESS < 8192
  // rjmp
  if (currentAddress.w == RESET_VECTOR_OFFSET * 2) {
    data = 0xC000 + (BOOTLOADER_ADDRESS/2) - 1;
  }
  #else
  // far jmp
  if (currentAddress.w == RESET_VECTOR_OFFSET * 2) {
    data = 0x940c;
  } else if (currentAddress.w == (RESET_VECTOR_OFFSET +1 ) * 2) {
    data = (BOOTLOADER_ADDRESS/2);
  }    
  #endif
#endif

  boot_page_fill(currentAddress.w, data);
  currentAddress.w += 2;

  if ((currentAddress.b[0] % SPM_PAGESIZE) == 0)  {
       __SPM_REG=(__BOOT_PAGE_WRITE|_BV(__SPM_ENABLE));
      spm((uint16_t)currentAddress.w - 2);
  }
}

#define RX PB3
#define TX PB4
#define BAUD 57600

#endif


void DelayBaud(void)
{
  _delay_us(1000000/BAUD);  
}

static void DelayBaudHalf(void)
{
  _delay_us(500000/BAUD);
}


static uint8_t GetByte(void)
{

  DDRB  &= ~_BV(RX);

  while (!(PINB&_BV(RX))); // Wait for hi
  while ( (PINB&_BV(RX))); // Wait for start bit
  
  DelayBaudHalf();
  
  uint8_t data=0;
  uint8_t i;

  for (i=0; i<8; i++)
  {
    DelayBaud();
    
    data>>=1;
    if (PINB&_BV(RX)) data|=0x80;       
  } 
  
  return data;    
}


static uint8_t GetBlock(void) 
{
  uint8_t sum=0,b,i=0;
    do
    {
      // Detect next falling edge
      uint8_t last=0;
      uint32_t ctr=800000;

      while (1) {
        if (PINB&_BV(RX)) {
          last=1;
        } else {
          if (last) 
            break;        
        }         

        if (!ctr--) {
            asm volatile ("rjmp __vectors - 2"); 
        }
      }

      // b=GetByte();
      // while (!(PINB&_BV(RX))); // Wait for hi
      // while ( (PINB&_BV(RX))); // Wait for start bit

      DelayBaudHalf();

      uint8_t data=0;
      uint8_t j;

      for (j=0; j<8; j++)
      {
        DelayBaud();
        
        data>>=1;
        if (PINB&_BV(RX)) data|=0x80;       
      }       
      buffer[i]=data;      
      sum+=data;     
    } while (++i<18);         

  return sum;  
}

static void SendByte(uint8_t data)
{
  uint8_t i;
  
   PORTB &=~_BV(TX);
  DelayBaud();
  
  for (i=0; i<8; i++)
  {
    if (data&1)
      PORTB |= _BV(TX);
    else
      PORTB &=~_BV(TX);
    data>>=1;
    DelayBaud();
  }

    PORTB |= _BV(TX);
  DelayBaud();
}

int main(void) {
 
  bootLoaderInit();
  
  if (bootLoaderStartCondition()) {
  
   DDRB  |= _BV(TX); 
    
   currentAddress.w = 0;

   do {

    uint8_t sum,i;

    sum=GetBlock();
    
    if (sum==0x59) {
      switch(buffer[16+2-1]) 
      {
        case 0x40:  // configuration reply
          SendByte(0x11);  // Version
          SendByte(BOOTLOADER_ADDRESS&0xff);  // bootstart_low
          SendByte(BOOTLOADER_ADDRESS>>8);  // bootstart_high
          break;

        case 0x41: // erase flash
          eraseApplication();
          break;

        case 0x42: // transfer data
          for (i=0; i<8; i++)
            writeWordToPageBuffer(bufword[i]);
        break;        
      }
      SendByte(0xAD);

    }

    // if (currentAddress.w>=BOOTLOADER_ADDRESS) break;
    
    // } while(1);  
    } while(currentAddress.w<BOOTLOADER_ADDRESS);  
  }
   
  asm volatile ("rjmp __vectors - 2"); // jump to application reset vector at end of flash
  
   for (;;); // Make sure function does not return to help compiler optimize
}
/* ------------------------------------------------------------------------ */
