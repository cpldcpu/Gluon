from intelhex import IntelHex

bootstart=0x200

ih=IntelHex()
ih.fromfile('main.hex',format='hex')

for addr in range(ih.minaddr(), ih.maxaddr()+1):
	ih[addr+bootstart]=ih[addr]
	ih[addr]=0xff

print("Done!")
ih.tofile('main.hex',format='hex')