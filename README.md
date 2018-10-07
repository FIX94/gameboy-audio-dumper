# GameBoy Audio Dumper
This project allows you to dump gameboy cartridges, both ROMs and Saves, using the audio output.  
It is intended for a gameboy color as you can hotswap cartridges on it, making this actually usable.    

# Important  
There may be a chance on certain carts that the save file may get damaged when you insert them while the gameboy color is already running, my cart of tetris dx appears to always wipe the first byte of the save when inserted into an already running gameboy color, no matter if the dumper is running on it or if it is any other game or just the bootup screen, this does only happen on this one cart though that uses the old MBC1 chip on it, my other carts that have newer MBC2 and MBC5 chips on them dont have any corruption like that, so use this with older carts at your own risk.    

# Requirements  
- GameCube with a GameBoy Player, a SD Gecko and a method to run Homebrew
- Copy of either the English or German Version of Pokemon Yellow  
- GameBoy Color so you can hotswap cartridges  
- Male to Male 3.5mm Stereo Cable or similar to record the Headphone Output of your GameBoy Color  
- PC to record and convert the ROM/Save on    

# Installation  
1. Make sure you have both Swiss-GC and GameBoy Interface on your SD Gecko:  
https://github.com/emukidid/swiss-gc/releases  
https://www.gc-forever.com/forums/viewtopic.php?t=2782  
2. Grab the current release from the "release" tab on top of this page and put the GBI folder from the "sender-installer-gbi" folder onto your SD Gecko root  
3. Put either gbi-fix94-pokemongelb-installer.txt.cli or gbi-fix94-pokemonyellow-installer.txt.cli renamed to gbi.cli depending on the game language you have onto your SD Gecko root as well and make sure GameBoy Interface is also on the root with the name gbi.dol  
4. Clear whatever save you have on your pokemon cartridge by holding down Up+Select+Start on the title screen  
5. Run the GameBoy Interface gbi.dol on your SD Gecko root from Swiss-GC, it will now install the dumper onto your cart, just wait for it to save+reset the game, at that point you can turn off your GameCube    

# Usage  
1. Put your cart into a GameBoy Color, get into the game and start the dumper by going into the item menu and use the glitch item ws m, it is the very first one in the list  
2. Make sure you have connected your audio cable from the headphone out into your PC and your recording software open, for all my recording I used Audacity set to 44100Hz 16bit Stereo  
3. Test record and start the dumping you ROM/Save on the GameBoy Color, now you can make sure your volume levels are set so the audio does not clip, which basically means it should not hit the absolute maximum volume, we will normalize the volume later anyways  
4. If the volume levels look good, just restart the GameBoy Color, get into the dumper, put in the cartridge you want to dump and then start recording a bit of silence so the volume level settles, then start dumping on the GameBoy Color  
5. Wait for it to finish dumping, after that you can stop the recording  
6. Cut off the bit of silence before and after the dumped data and then normalize the audio, in Audacity that is under Effect -> Normalize, I personally have remove dc offset unchecked and normalize maximum amplitude set to 0.0dB and normalize stereo channels independently checked  
7. Export the now cut and normalized recording as a standard .wav file into a new folder together with receiver-gbc.exe from the receiver folder found in the current release zip you downloaded earlier  
8. Drag and drop your .wav file into receiver-gbc.exe and wait for it to convert the .wav into a .gbc or .sav file!    

Some older GameBoy Cartridges may reset your GameBoy Color when you insert them, on those you have to tape off pin 30, the RESET pin of the cartridge:  
http://www.rickard.gunee.com/projects/playmobile/html/3/Image9.gif  
Also sometimes it can just randomly reset when taking out a cart or inserting one so before taping off that port you can just try again.  
Please let me know if you run into any issues, I only was able to test all this on the german version of pokemon yellow.    

# Technical Details  
To compile the sender, you need rgbasm 0.3.3 and rgbbin from here:  
https://github.com/zzazzdzz/rgbbin  
The resulting bin then goes into the game RAM at 0xDA7F in the english and 0xDA84 in the german version of pokemon yellow, executed with glitch item ws m.  
The sender right now hits about 2.27KB/s so even the biggest official cartridge size of 8MB takes an hour to dump, generally most carts you will run into will be much smaller and thus send much faster.  

The installer is a set of inputs I created in an emulator that go from the beginning of the game up to a point where it can install that sender. See the .bat files in the installer folder for how those put the sender into those inputs. Also in the "code" folder you will find the little C program that does the actual writing of those inputs, you can compile that with gcc.  
The installer is putting up to 1122 bytes of code into the memory region of the current PC Box.  
Of course you could use the installer to really execute anything you want on a gameboy, you can write up whatever code instead of the sender if you want to do something else with those 1122 bytes.  
For example I did also make this small side project using this same installer that gives you a sound test with a visualizer:  
https://github.com/FIX94/pkm-sound-visualizer  
To get from the .bk2 it creates to a .txt that gameboy interface can then use, you can just play back the .bk2 in BizHawk2 while you have this lua script from TiKevin83 loaded in the lua console:  
https://pastebin.com/NbTRNePD    

The receiver is a simple C program you can easily compile with gcc.  
It basically goes through the .wav file sample by sample, tries to find "silence", then a "peak" and then "silence" again, with that information it evaluates which volume level it got and interprets that as part of the byte it ends up writing into the .gbc or .sav file it creates.  

This project can theoretically be extended to run from many other generation 1 pokemon games and probably also generation 2, at this point I did not look into anything else though.  
It may even work on a gameboy advance if you have a way to constantly push down the small switch on the side of the case that normally switches it back into gameboy advance mode when removing the gameboy color cartridge, though at this point I have not tested that.  
What I did test however is removing the power switch on an original gameboy, and on that one sadly it appears to reset every time you remove the pokemon cartridge.  
