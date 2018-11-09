rgbasm -o start.o start.asm
rgblink -o gb-audio-dumper.gb start.o
rgbfix -i GDMP -j -k 00 -l 0x33 -m 0 -n 0 -p 0 -r 0 -t GBDMP -v gb-audio-dumper.gb

pause