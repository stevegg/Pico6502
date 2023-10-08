PORTB       = $6000
PORTA       = $6001
DDRB        = $6002
DDRA        = $6003

ACIA_DATA   = $5000
ACIA_STATUS = $5001
ACIA_CMD    = $5002
ACIA_CTRL   = $5003

E           = %10000000
RW          = %01000000
RS          = %00100000

  .org $8000

reset:
   ldx #$ff
   txs

   ; Initialize LCD
   jsr lcd_init

   ; Initialize Serial
   jsr serial_init

start:
   ; Print message
   ldx #0
print:
   lda message,x
   beq start
   ; Print to LCD
   jsr lcd_print_char
   ; Print to Serial
   jsr serial_print_char
   inx
   jmp print

message: .asciiz "Hello, world!"

lcd_init:
   pha
   lda #%11111111 ; Set all pins on port B to output
   sta DDRB
   lda #%11100000 ; Set top 3 pins on port A to output
   sta DDRA

   lda #%00111000 ; Set 8-bit mode; 2-line display; 5x8 font
   jsr lcd_instruction
   lda #%00001110 ; Display on; cursor on; blink off
   jsr lcd_instruction
   lda #%00000110 ; Increment and shift cursor; don't shift display
   jsr lcd_instruction
   lda #$00000001 ; Clear display
   jsr lcd_instruction
   pla
   rts

lcd_wait:
  pha
  lda #%00000000  ; Port B is input
  sta DDRB
lcdbusy:
  lda #RW
  sta PORTA
  lda #(RW | E)
  sta PORTA
  lda PORTB
  and #%10000000
  bne lcdbusy

  lda #RW
  sta PORTA
  lda #%11111111  ; Port B is output
  sta DDRB
  pla
  rts

lcd_instruction:
  jsr lcd_wait
  sta PORTB
  lda #0         ; Clear RS/RW/E bits
  sta PORTA
  lda #E         ; Set E bit to send instruction
  sta PORTA
  lda #0         ; Clear RS/RW/E bits
  sta PORTA
  rts

lcd_print_char:
   pha               ; Save A Register          
   jsr lcd_wait
   sta PORTB
   lda #RS           ; Set RS; Clear RW/E bits
   sta PORTA
   lda #(RS | E)     ; Set E bit to send instruction
   sta PORTA
   lda #RS           ; Clear E bits
   sta PORTA
   pla               ; Restore A Register
   rts

serial_init:

   lda #$1F          ; 8-N-1, 19200 baud.
   sta ACIA_CTRL
   lda #$0B          ; No parity, no echo, no interrupts.
   sta ACIA_CMD
   rts

serial_print_char:
   pha               ; Save A.
   sta ACIA_DATA     ; Output character.
   lda #$FF          ; Initialize delay loop.
txdelay:
   dec               ; Decrement A.
   bne txdelay       ; Until A gets to 0.
   pla               ; Restore A.
   rts               ; Return.

  .org $fffc
  .word reset
  .word $0000