# gameboy-audio-dumper
This is still very much an early proof of concept project, it aims to dump gameboy cartridges using the audio output.  
It is intended for a gameboy color as you can hotswap cartridges on it, making this actually usable.    

The sender right now was written for the one exploitable game I own, the german version of pokemon yellow.  
To compile it, you need rgbasm 0.3.3 and rgbbin from here:  
https://github.com/zzazzdzz/rgbbin  
The resulting bin then goes into the game RAM at 0xDA84, executed with glitch item ws l' m.  
All it does is wait for you to press start and then starts sending the current cartridge inserted via audio at about 2.2KB/s, no options or interface as of now.  
With that speed, it takes under 8 minutes to go dump 1MB carts which I think is not too bad at all, I dont think it could go much faster with my current design.  
To get it into that position, I created a set of inputs in an emulator that go from the beginning of the game up to a point where it can install that sender.  
Right now that installer is too dirty for me to feel comfortable sharing it.    

The receiver is a simple C program that takes a .wav file of the recorded audio of the sender and turns it into a usable ROM.  
It takes a standard 16bit Stereo 44100Hz PCM file to do so.  
Right now I just tested recording the headphone out of my gameboy color in audacity and then normalized the audio before exporting it to .wav.    

I hope to extend this to a point where it can be installed in many generation 1 pokemon games and also give the sender some more functionality.  
For now I just wanted to upload the base proof of concept idea.
