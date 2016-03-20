import serial
import time
from intelhex import IntelHex

# install missing libraries:
# 	pip install pyserial
# 	pip install intelhex


def sendblock(command,payload):
	if len(payload)!=16:
		raise Exception('Payload length incorrect')

	magicnumber=0x59

	checksum=(magicnumber-(sum(payload)+command))&0xff
	ser.write(payload)
	ser.write([checksum])
	ser.write([command])

ser=serial.Serial('com6',57600,timeout=0.1)

ih=IntelHex()
ih.fromfile('Blinky.hex',format='hex')

while True:
	try:
		print("Connecting...")
		""" send dummy packet to sync RX/TX """
		sendblock(0x00,[1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8])
		ser.timeout=0.05
		ackdat=ser.read(1)

		if len(ackdat)==1:
			print("\t",ackdat)
		else:
			print("timeout")
			continue


		sendblock(0x40,[1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8])
		ser.timeout=0.05
		ackdat=ser.read(4)

		if len(ackdat)==4:
			print("\t",ackdat)
		else:
			print("timeout")
			continue
		
		bootstart=ackdat[1]+256*ackdat[2]
		versionlo=ackdat[0]&15;
		versionhi=ackdat[0]>>4;

		print("Bootloader version:",versionhi,".",versionlo,sep='')
		if (versionhi!=1):
			print("Unknown bootloader version. Aborting!")
			break

		print("Bootstart:",hex(bootstart))


		# Patch in rjmp to user program
 
		word0 = ih[0] + 256 * ih[1]

		if (word0&0xf000)!=0xc000:
			print("The reset vector of the user program does not contain a branch instruction.\n"
 				  "Therefore, the bootloader can not be inserted. Please rearrage your code.")
			print("Aborting!")
			break	

		userreset = (word0 & 0x0fff) - 0 + 1
		data = 0xc000 | ((userreset - ((bootstart - 2)>>1)  - 1) & 0xfff)
		ih[bootstart-2]	 = data &  0xff
		ih[bootstart-2+1]= (data >> 8) & 0xff

		# add rjmp to program start

		data = 0xC000 + (bootstart>>1) - 1
		ih[0]= data & 0xff
		ih[1]= data >> 8


		#ih.dump()

		print("Erasing...")
		sendblock(0x41,[1,2,3,4,5,6,7,8,1,2,3,4,5,6,7,8])
		ser.timeout=1
		ackdat=ser.read(1)
		if ackdat:
			print("\t",ackdat)
		else:
			print("timeout")

		print("Programming...")

		for addr in range(0, bootstart, 16):
			sendblock(0x42,ih.tobinarray(start=addr, size=16))
			ser.timeout=0.2
			ackdat=ser.read(1)
			if ackdat:
				print("addr:",hex(addr),ackdat)
			else:
				print("timeout")

		print("Done!")
		break
	
	except KeyboardInterrupt:
		print("Aborted!")
		break



