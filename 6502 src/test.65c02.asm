  .org $8000

reset:
  lda #$ff
  sta $6002

  lda #$50
  sta $6000

loop:
  jsr rotate
  jmp loop

rotate:
  ror
  sta $6000
  rts

  .org $fffc
  .word reset
  .word $0000