;
; compile the sections of the OS
;

   .org $8000

	; ACIA init (19200,n,8,1)
   .include "ACIA1.65c02.asm"

   ; OS
 	.include "sbcos.65c02.asm"         
 
   ; Reset & IRQ handler
	.include "reset.65c02.asm"        

