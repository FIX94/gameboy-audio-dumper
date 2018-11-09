del gelb.bin gelb.o
rgbasm -o gelb.o -DRAMOFFSET=$DA84 sender.asm
python -m rgbbin gelb.o
ren WRAM.bin gelb.bin

del yellow.bin yellow.o
rgbasm -o yellow.o -DRAMOFFSET=$DA7F sender.asm
python -m rgbbin yellow.o
ren WRAM.bin yellow.bin

del standalone.bin standalone.o
rgbasm -o standalone.o -DRAMOFFSET=$C000 sender.asm
python -m rgbbin standalone.o
ren WRAM.bin standalone.bin

pause