#
# Please see this video for details:
# https://www.youtube.com/watch?v=yl8vPW5hydQ

rom = bytearray([0xea] * (32768))

rom[0x7ffc] = 0x00
rom[0x7ffd] = 0x80

with open("rom.bin", "wb") as out_file:
  out_file.write(rom)