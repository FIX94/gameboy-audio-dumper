/*
 * Copyright (C) 2018 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <malloc.h>

//0x1800 is harcoded minimum spike in volume to count as peak
#define PEAK_MIN 0x1800
//0x1000 is harcoded limit for difference in "silence" levels
#define SDIFF_MAX 0x1000
//set this to 1 if dump was made in gameboy interface
#define GBPLAYER_GBI 0

static void curpeak(int32_t *pl, int32_t *pr, int16_t *smpl)//, int32_t i)
{
	*pl = 0, *pr = 0;
	//"silence"
	int32_t sl1 = (smpl[0]+smpl[2]+smpl[4]) / 3;
	int32_t sr1 = (smpl[1]+smpl[3]+smpl[5]) / 3;
	//volume spike
#if GBPLAYER_GBI
	//seems to only have 1 distinct spike
	int32_t peakl = smpl[10];
	int32_t peakr = smpl[11];
#else
	//average out top 2 spikes
	int32_t peakl = (smpl[10]+smpl[12]) / 2;
	int32_t peakr = (smpl[11]+smpl[13]) / 2;
#endif
	//"silence"
	int32_t sl2 = (smpl[18]+smpl[20]+smpl[22]) / 3;
	int32_t sr2 = (smpl[19]+smpl[21]+smpl[23]) / 3;
	int32_t sdiffl = abs(sl1-sl2);
	int32_t savgl = (sl1+sl2) / 2;
	int32_t sdiffr = abs(sr1-sr2);
	int32_t savgr = (sr1+sr2) / 2;
#if GBPLAYER_GBI
	//the wave channel seems to be inverted
	peakl*=-1; peakr*=-1;
	savgl*=-1; savgr*=-1;
#endif
	//printf("%i %i %i %i %i %i %i %i %i %i %i\n", i, sl1, sr1, peakl, peakr, sl2, sr2, sdiffl, savgl, sdiffr, savgr);
	if(sdiffl < SDIFF_MAX && peakl > savgl && (peakl-savgl) > PEAK_MIN &&
		sdiffr < SDIFF_MAX && peakr > savgr && (peakr-savgr) > PEAK_MIN)
	{
		*pl = peakl-savgl;
		*pr = peakr-savgr;
	}
}
int main(int argc, char *argv[])
{
	printf("GameBoy Audio Dumper POC by FIX94\n");
	if(argc < 2 || strlen(argv[1]) < 5 || memcmp(argv[1]+strlen(argv[1])-4,".wav",4) != 0)
	{
		printf("Please provide a .wav file to process!\n");
		return 0;
	}
	FILE *f = fopen(argv[1],"rb");
	if(!f)
	{
		printf("ERROR: Unable to open %s!\n", argv[1]);
		return 0;
	}
	fseek(f,0,SEEK_END);
	size_t fsize = ftell(f);
	if(fsize < 0x30)
	{
		printf("Not enough data in .wav to process!\n");
		fclose(f);
		return 0;
	}
	rewind(f);
	uint8_t hdr[0x2C];
	fread(hdr,1,0x2C,f);
	if(memcmp("RIFF",hdr,4) != 0 || memcmp("WAVE",hdr+8,4) != 0 || memcmp("fmt",hdr+12,3) != 0 || *(uint32_t*)(hdr+16) != 0x10
		|| memcmp("data",hdr+36,4) != 0)
	{
		printf("Invalid .wav header!\n");
		fclose(f);
		return 0;
	}
	if(*(uint16_t*)(hdr+20) != 1 || *(uint16_t*)(hdr+22) != 2 || *(uint32_t*)(hdr+24) != 44100 || *(uint32_t*)(hdr+28) != 176400
		|| *(uint16_t*)(hdr+32) != 4 || *(uint16_t*)(hdr+34) != 16)
	{
		printf(".wav has to be 16bit Stereo 44100Hz PCM!\n");
		fclose(f);
		return 0;
	}
	size_t datsize = *(uint32_t*)(hdr+40);
	if(datsize > (fsize-0x2C))
	{
		printf("Data size bigger than .wav!\n");
		fclose(f);
		return 0;
	}
	uint8_t *buf = malloc(datsize);
	fread(buf,1,datsize,f);
	fclose(f);
	memcpy(argv[1]+strlen(argv[1])-4,".gbc",4);
	f = fopen(argv[1],"wb");
	if(!f)
	{
		printf("ERROR: unable to write %s!\n", argv[1]);
		free(buf);
		return 0;
	}
	size_t i = 0,j = 0;
	int16_t *smpl = (int16_t*)buf;
	size_t smplsize = datsize/2;
	printf("Processing %i samples\n", smplsize);
	int32_t statesltop[4] = {0,0,0,0};
	int32_t statesrtop[4] = {0,0,0,0};
	int32_t stateslbottom[4] = {0,0,0,0};
	int32_t statesrbottom[4] = {0,0,0,0};
	int32_t devl[4] = {0,0,0,0};
	int32_t devr[4] = {0,0,0,0};
	size_t devlpos[4] = {0,0,0,0};
	size_t devrpos[4] = {0,0,0,0};
	uint8_t statesdone = 0;
	uint8_t bstate = 0;
	uint8_t byte = 0;
	size_t previ = 0;
	size_t wpos = 0;
	//go through read in samples
	while(i+38 < smplsize)
	{
		int32_t peakl, peakr;
		//try see if theres a peak in between
		curpeak(&peakl, &peakr, smpl+i);//, i>>1);
		//if there was one, check samples around it
		//to find the middle of it for accurate capture
		//size_t addl = 0, addr = 0;
		if(peakl && peakr)
		{
			int32_t peakltmp, peakrtmp;
			for(j = 2; j < 6; j+=2)
			{
				curpeak(&peakltmp, &peakrtmp, smpl+i+j);//, (i+j)>>1);
				//if(i>>1 == 36154) printf("%i %i\n", peakltmp, peakrtmp);
				if(peakltmp > peakl)
				{
					peakl = peakltmp;
					//addl = j;
				}
				if(peakrtmp > peakr)
				{
					peakr = peakrtmp;
					//addr = j;
				}
			}
		}
		//process peak in the middle
		if(peakl && peakr)
		{
			//printf("%i, %i %i %i %i %i\n", wpos, i>>1, addl, addr, peakl, peakr);
			//init the 4 input volumes
			if(statesdone < 4)
			{
				devl[statesdone] = statesltop[statesdone] = peakl;
				devr[statesdone] = statesrtop[statesdone] = peakr;
				statesdone++;
				if(statesdone == 4)
				{
					stateslbottom[0] = statesltop[0]-((statesltop[0]-statesltop[1])/2);
					stateslbottom[1] = statesltop[1]-((statesltop[1]-statesltop[2])/2);
					stateslbottom[2] = statesltop[2]-((statesltop[2]-statesltop[3])/2);

					statesrbottom[0] = statesrtop[0]-((statesrtop[0]-statesrtop[1])/2);
					statesrbottom[1] = statesrtop[1]-((statesrtop[1]-statesrtop[2])/2);
					statesrbottom[2] = statesrtop[2]-((statesrtop[2]-statesrtop[3])/2);
					printf("States Top Left: %i %i %i %i\n", statesltop[0], statesltop[1], statesltop[2], statesltop[3]);
					printf("States Top Right: %i %i %i %i\n", statesrtop[0], statesrtop[1], statesrtop[2], statesrtop[3]);
					printf("States Bottom Left: %i %i %i\n", stateslbottom[0], stateslbottom[1], stateslbottom[2]);
					printf("States Bottom Right: %i %i %i\n", statesrbottom[0], statesrbottom[1], statesrbottom[2]);
				}
			}
			else //process nibble
			{
				if(bstate == 1)
					byte >>= 4;
				if(peakl > stateslbottom[0])
				{
					byte |= 0xC0;
					if(peakl < devl[0])
					{
						devl[0] = peakl;
						devlpos[0] = wpos;
					}
				}
				else if(peakl > stateslbottom[1])
				{
					byte |= 0x80;
					if(peakl < devl[1])
					{
						devl[1] = peakl;
						devlpos[1] = wpos;
					}
				}
				else if(peakl > stateslbottom[2])
				{
					byte |= 0x40;
					if(peakl < devl[2])
					{
						devl[2] = peakl;
						devlpos[2] = wpos;
					}
				}
				if(peakr > statesrbottom[0])
				{
					byte |= 0x30;
					if(peakr < devr[0])
					{
						devr[0] = peakr;
						devrpos[0] = wpos;
					}
				}
				else if(peakr > statesrbottom[1])
				{
					byte |= 0x20;
					if(peakr < devr[1])
					{
						devr[1] = peakr;
						devrpos[1] = wpos;
					}
				}
				else if(peakr > statesrbottom[2])
				{
					byte |= 0x10;
					if(peakr < devr[2])
					{
						devr[2] = peakr;
						devrpos[2] = wpos;
					}
				}
				//got 2 nibbles, write byte
				if(bstate == 1)
				{
					fwrite(&byte,1,1,f);
					byte = 0;
					wpos++;
				}
				bstate^=1;
			}
			if(i-previ > 24)
				printf("WARNING: Possible desync at byte %i sample %i with a value of %i\n", wpos, i>>1, i-previ);
			previ = i;
			//add 10 samples before next check
			i+=18;
		}
		else //no peak, add sample
			i+=2;
	}
	printf("Done! Wrote %i bytes\n", wpos);
	printf("Statistics:\n");
	printf("Biggest Deviations from States Top to Bottom 0, 1 and 2 Left:\n");
	printf("Byte %i with %i%%, Byte %i with %i%%, Byte %i with %i%%\n", devlpos[0], 100-((devl[0]-stateslbottom[0])*100/(statesltop[0]-stateslbottom[0])), 
																		devlpos[1], 100-((devl[1]-stateslbottom[1])*100/(statesltop[1]-stateslbottom[1])),
																		devlpos[2], 100-((devl[2]-stateslbottom[2])*100/(statesltop[2]-stateslbottom[2])));
	printf("Biggest Deviations from States Top to Bottom 0, 1 and 2 Right:\n");
	printf("Byte %i with %i%%, Byte %i with %i%%, Byte %i with %i%%\n", devrpos[0], 100-((devr[0]-statesrbottom[0])*100/(statesrtop[0]-statesrbottom[0])),
																		devrpos[1], 100-((devr[1]-statesrbottom[1])*100/(statesrtop[1]-statesrbottom[1])),
																		devrpos[2], 100-((devr[2]-statesrbottom[2])*100/(statesrtop[2]-statesrbottom[2])));
	fclose(f);
	free(buf);
	return 0;
}