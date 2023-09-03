  .org $8000

PORTB = $6000
PORTA = $6001
DDRB = $6002
DDRA = $6003

reset:
  lda #$ff
  sta PORTB

  lda #$50
  sta PORTB

repeat:
  jsr rotate
  jmp repeat

rotate:
  ror
  sta PORTB
  rts

  .org $fffc
  .word reset
  .word $0000