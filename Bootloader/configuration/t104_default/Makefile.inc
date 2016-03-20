# Name: Makefile
# Project: Micronucleus
# License: GNU GPL v2 (see License.txt)

# Controller type: ATtiny 104 - 8 MHz
# Configuration:   Default
# Last Change:     Mar 16,2016
 
 
F_CPU = 8000000
DEVICE = attiny104
LDPATH =-B $(CURDIR)"\configuration\t104_default"

# hexadecimal address for bootloader section to begin. To calculate the best value:
# - make clean; make main.hex; ### output will list data: 2124 (or something like that)
# - for the size of your device (8kb = 1024 * 8 = 8192) subtract above value 2124... = 6068
# - How many pages in is that? 6068 / 64 (tiny85 page size in bytes) = 94.8125
# - round that down to 94 - our new bootloader address is 94 * 64 = 6016, in hex = 1780
BOOTLOADER_ADDRESS = 200

FUSEOPT = -U fuse:w:0xf7:m 



#---------------------------------------------------------------------
# ATtiny104
#---------------------------------------------------------------------
# Configuration Byte
#
#
# Bit 7:4	Unused
# Bit 3		self-programming       (enable=0)
# Bit 2     System clock output    (enable=0)
# Bit 1 	Watchdog always on     (enable=0)	
# Bit 0 	External Reset Disable (enable=0)	


###############################################################################
