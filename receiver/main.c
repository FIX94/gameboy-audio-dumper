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
int32_t getpeak(int16_t *smpl, size_t len)
{
	int32_t pm = 0, pp = 0;
	size_t i;
	for(i = 0; i < len; i+=2)
	{
		if(smpl[i] < pm) pm = smpl[i];
		if(smpl[i] > pp) pp = smpl[i];
	}
	return pp-pm;
}
void curpeak(int32_t *pl, int32_t *pr, int16_t *smpl)
{
	*pl = 0, *pr = 0;
	//"silence"
	int32_t peakl1 = getpeak(smpl+0, 8);
	int32_t peakr1 = getpeak(smpl+1, 8);
	//volume spike
	int32_t peakl2 = getpeak(smpl+8, 16);
	int32_t peakr2 = getpeak(smpl+9, 16);
	//"silence"
	int32_t peakl3 = getpeak(smpl+24, 8);
	int32_t peakr3 = getpeak(smpl+25, 8);
	//0x2000 is hardcoded base volume for now
	if(peakl2 > 0x2000 && peakl2 > peakl1 && peakl2 > peakl3 &&
		peakr2 > 0x2000 && peakr2 > peakr1 && peakr2 > peakr3)
	{
		*pl = peakl2;
		*pr = peakr2;
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
		curpeak(&peakl, &peakr, smpl+i);
		//if there was one, check samples around it
		//to find the middle of it for accurate capture
		if(peakl && peakr)
		{
			int32_t peakltmp, peakrtmp;
			size_t add = 0;
			for(j = 2; j < 8; j+=2)
			{
				curpeak(&peakltmp, &peakrtmp, smpl+i+j);
				if(peakltmp > peakl && peakrtmp > peakr)
				{
					peakl = peakltmp;
					peakr = peakrtmp;
					add = j;
				}
			}
			i+=add;
		}
		//process peak in the middle
		if(peakl && peakr)
		{
			//init the 4 input volumes
			if(statesdone < 4)
			{
				statesltop[statesdone] = peakl;
				statesrtop[statesdone] = peakr;
				statesdone++;
				if(statesdone == 4)
				{
					stateslbottom[0] = statesltop[1]+((statesltop[0]-statesltop[1])/2);
					stateslbottom[1] = statesltop[2]+((statesltop[1]-statesltop[2])/2);
					stateslbottom[2] = statesltop[3]+((statesltop[2]-statesltop[3])/2);

					statesrbottom[0] = statesrtop[1]+((statesrtop[0]-statesrtop[1])/2);
					statesrbottom[1] = statesrtop[2]+((statesrtop[1]-statesrtop[2])/2);
					statesrbottom[2] = statesrtop[3]+((statesrtop[2]-statesrtop[3])/2);
				}
			}
			else //process nibble
			{
				if(bstate == 1)
					byte >>= 4;
				if(peakl > stateslbottom[0])
					byte |= 0xC0;
				else if(peakl > stateslbottom[1])
					byte |= 0x80;
				else if(peakl > stateslbottom[2])
					byte |= 0x40;
				if(peakr > statesrbottom[0])
					byte |= 0x30;
				else if(peakr > statesrbottom[1])
					byte |= 0x20;
				else if(peakr > statesrbottom[2])
					byte |= 0x10;
				//got 2 nibbles, write byte
				if(bstate == 1)
				{
					fwrite(&byte,1,1,f);
					byte = 0;
					wpos++;
					if(i-previ > 30)
						printf("WARNING: Possible desync at byte %i sample %i with a value of %i\n", wpos, i>>1, i-previ);
				}
				bstate^=1;
			}
			previ = i;
			//add 10 samples before next check
			i+=20;
		}
		else //no peak, add sample
			i+=2;
	}
	printf("Done! Wrote %i bytes\n", wpos);
	fclose(f);
	free(buf);
	return 0;
}