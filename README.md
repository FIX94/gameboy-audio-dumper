# gameboy-audio-dumper
This is still very much an early proof of concept project, it aims to dump gameboy cartridges using the audio output.  
It is intended for a gameboy color as you can hotswap cartridges on it, making this actually usable.    

The sender right now was written for the one exploitable game I own, the german version of pokemon yellow.  
To compile it, you need rgbasm 0.3.3 and rgbbin from here:  
https://github.com/zzazzdzz/rgbbin  
The resulting bin then goes into the game RAM at 0xDA84, executed with glitch item ws l' m. All this is done by a installer I wrote.  
The sender will prompt you on screen to insert the cartridge you want to dump and when you insert one and continue, it then starts sending the current cartridge inserted as audio via the headphone out at about 2.2KB/s, if it is unable to properly read the cart header because of no cart being inserted or the cart being dirty it will say so on screen so you can try again.  
With that send speed of 2.2KB/s, it takes under 8 minutes to go dump 1MB carts which I think is not too bad at all, I dont think it could go much faster with my current design.    

To get it into that position, I created a set of inputs in an emulator that go from the beginning of the game up to a point where it can install that sender.  
The installer is putting up to 1122 bytes of code into the set of inputs I created so when you then convert it and play it back on a game boy player, it will automatically put the sender into your save, ready to use on a real gameboy color.  
Of course you could use the installer to really execute anything you want on a gameboy, you can write up whatever code instead of the sender if you want to do something else with those 1122 bytes.    

The receiver is a simple C program that takes a .wav file of the recorded audio of the sender and turns it into a usable ROM.  
It takes a standard 16bit Stereo 44100Hz PCM file to do so.  
Right now I just tested recording the headphone out of my gameboy color in audacity and then normalized the audio before exporting it to .wav.    

I hope to extend this to a point where it can be installed in many generation 1 pokemon games and also give the sender some more functionality.  
For now I just wanted to upload the base proof of concept idea.
