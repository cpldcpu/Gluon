/* Register assignments for gcc */
/* Calling conventions on gcc:
 * First parameter passed in r24/r25, second in r22/23 and so on.
 * Callee must preserve r1-r17, r28/r29
 * Result is passed in r24/r25
 */
#if 0

#define i		r20	
#define opV		r19
#define opD		r18
#define try		r27
#define stp		r26
#define cnt16L   r24
#define cnt16H   r25

#define j		r21
#define data	r24

.global DelayBaud
DelayBaud:

#if (F_CPU == 16500000)
 	ldi		i, lo8(93)
#endif

DelayBaudLoop:
	dec		i
	brne	DelayBaudLoop
	ret

.global DelayBaudHalf
DelayBaudHalf:

#if (F_CPU == 16500000)
 	ldi	i, lo8(44)
#endif
	rjmp	DelayBaudLoop

/*
void SendByte(uint8_t data)
{
  uint8_t i;
  
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
}*/

.global SendByte
SendByte:
	cbi		0x18, 4
	rcall	DelayBaud
	ldi		j,8
SendByteLoop:
	sbrs	data,0
	cbi		0x18, 4
	sbrc	data,0
	sbi		0x18, 4
	lsr		data
	rcall	DelayBaud
	dec 	j
	brne	SendByteLoop
	sbi		0x18, 4
	rjmp	DelayBaud
#endif





  