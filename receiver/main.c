/*
 * Copyright (C) 2018 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <malloc.h>
#include "crc32.h"
#include "polarssl/md5.h"
#include "polarssl/sha1.h"

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

static unsigned int crc32tmp = 0;
static md5_context md5ctx;
static sha1_context sha1ctx;

static void emptywbuf(FILE *f, uint8_t *buf, size_t size)
{
	if(size)
	{
		crc32tmp = crc32buffer(buf, size, crc32tmp);
		md5_update(&md5ctx, buf, size);
		sha1_update(&sha1ctx, buf, size);
		fwrite(buf, 1, size, f);
	}
}

static void updatedev(int32_t peak, int32_t *dev, size_t pos, size_t *devpos)
{
	if(peak < *dev)
	{
		*dev = peak;
		*devpos = pos;
	}
}

int main(int argc, char *argv[])
{
	//file we read from/write to
	FILE *fr = NULL, *fw = NULL;
	size_t frsize = 0, fwsize = 0;
	//wav file header
	uint8_t hdr[0x2C];
	//read/write buffers
	uint8_t *rbuf = NULL;
	size_t rremain = 0;
	uint8_t *wbuf = NULL;
	size_t datsize = 0;
	int16_t *smpl = NULL;
	size_t smplsize = 0;
	size_t smpltotal = 0, prevsmpltotal = 0, smplpos = 0;
	size_t i = 0;
	//amount of volume states measured
	uint8_t statesdone = 0;
	//measured volume states and deviations of them
	int32_t statesltop[4] = {0,0,0,0}, statesrtop[4] = {0,0,0,0};
	int32_t stateslbottom[4] = {0,0,0,0}, statesrbottom[4] = {0,0,0,0};
	int32_t devl[4] = {0,0,0,0}, devr[4] = {0,0,0,0};
	size_t devlpos[4] = {0,0,0,0}, devrpos[4] = {0,0,0,0};
	//currently processed peaks
	int32_t peakl = 0, peakr = 0;
	int32_t peakltmp = 0, peakrtmp = 0;
	//currently processed byte
	uint8_t bstate = 0;
	uint8_t byte = 0;
	//bytes written total
	size_t wpos = 0;
	//hashing
	uint32_t crc32 = 0;
	uint8_t md5[16];
	uint8_t sha1[20];
	//finally go
	printf("GameBoy Audio Dumper v0.2 by FIX94\n");
	if(argc < 2 || strlen(argv[1]) < 5 || memcmp(argv[1]+strlen(argv[1])-4,".wav",4) != 0)
	{
		printf("Please provide a .wav file to process!\n");
		goto end_prog;
	}
	fr = fopen(argv[1],"rb");
	if(!fr)
	{
		printf("ERROR: Unable to open %s!\n", argv[1]);
		goto end_prog;
	}
	fseek(fr,0,SEEK_END);
	frsize = ftell(fr);
	if(frsize < 0x30)
	{
		printf("Not enough data in .wav to process!\n");
		goto end_prog;
	}
	rewind(fr);
	fread(hdr,1,0x2C,fr);
	if(memcmp("RIFF",hdr,4) != 0 || memcmp("WAVE",hdr+8,4) != 0 || memcmp("fmt",hdr+12,3) != 0 || *(uint32_t*)(hdr+16) != 0x10
		|| memcmp("data",hdr+36,4) != 0)
	{
		printf("Invalid .wav header!\n");
		goto end_prog;
	}
	if(*(uint16_t*)(hdr+20) != 1 || *(uint16_t*)(hdr+22) != 2 || *(uint32_t*)(hdr+24) != 44100 || *(uint32_t*)(hdr+28) != 176400
		|| *(uint16_t*)(hdr+32) != 4 || *(uint16_t*)(hdr+34) != 16)
	{
		printf(".wav has to be 16bit Stereo 44100Hz PCM!\n");
		goto end_prog;
	}
	datsize = *(uint32_t*)(hdr+40);
	if(datsize > (frsize-0x2C))
	{
		printf("Data size bigger than .wav!\n");
		goto end_prog;
	}
	fwsize = 0;
	rbuf = malloc(16*1024*1024);
	wbuf = malloc(1024*1024);
	if(!rbuf || !wbuf)
	{
		printf("ERROR: Unable to allocate read/write buffers!\n");
		goto end_prog;
	}
	rremain = 0;
	smpl = (int16_t*)rbuf;
	smplsize = datsize/2;
	printf("Processing %i samples\n", smplsize);
	//initialize hash variables
	crc32tmp = 0xFFFFFFFF;
	md5_starts(&md5ctx);
	sha1_starts(&sha1ctx);
	//go through read in samples
	while(smpltotal+30 < smplsize)
	{
		//refill read buffer if needed
		if(rremain < 60)
		{
			fseek(fr,(smpltotal<<1)+0x2C,SEEK_SET);
			rremain = fread(rbuf,1,16*1024*1024,fr);
			//not enough remaining of the file for another peak
			if(rremain < 60)
			{
				printf("Ending sample reading early at sample %i\n", i>>1);
				break;
			}
			printf("%i%%\n", (size_t)((((float)smpltotal)*100.f)/((float)smplsize)));
			smplpos = 0;
		}
		//try see if theres a peak in between
		curpeak(&peakl, &peakr, smpl+smplpos);//, i>>1);
		//if there was one, check samples around it
		//to find the middle of it for accurate capture
		//size_t addl = 0, addr = 0;
		if(peakl && peakr)
		{
			for(i = 2; i < 6; i+=2)
			{
				curpeak(&peakltmp, &peakrtmp, smpl+smplpos+i);//, (i+j)>>1);
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
					printf("States Top Left: %i %i %i %i\n", statesltop[0], statesltop[1], statesltop[2], statesltop[3]);
					printf("States Top Right: %i %i %i %i\n", statesrtop[0], statesrtop[1], statesrtop[2], statesrtop[3]);
					if(statesltop[0] <= statesltop[1] || statesrtop[0] <= statesrtop[1] ||
						statesltop[1] <= statesltop[2] || statesrtop[1] <= statesrtop[2] ||
						statesltop[2] <= statesltop[3] || statesrtop[2] <= statesrtop[3])
					{
						printf("ERROR: States not read in as expected! Possible volume problem?\n");
						goto end_prog;
					}
					stateslbottom[0] = statesltop[0]-((statesltop[0]-statesltop[1])/2);
					stateslbottom[1] = statesltop[1]-((statesltop[1]-statesltop[2])/2);
					stateslbottom[2] = statesltop[2]-((statesltop[2]-statesltop[3])/2);

					statesrbottom[0] = statesrtop[0]-((statesrtop[0]-statesrtop[1])/2);
					statesrbottom[1] = statesrtop[1]-((statesrtop[1]-statesrtop[2])/2);
					statesrbottom[2] = statesrtop[2]-((statesrtop[2]-statesrtop[3])/2);

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
					updatedev(peakl, devl+0, wpos, devlpos+0);
				}
				else if(peakl > stateslbottom[1])
				{
					byte |= 0x80;
					updatedev(peakl, devl+1, wpos, devlpos+1);
				}
				else if(peakl > stateslbottom[2])
				{
					byte |= 0x40;
					updatedev(peakl, devl+2, wpos, devlpos+2);
				}
				if(peakr > statesrbottom[0])
				{
					byte |= 0x30;
					updatedev(peakr, devr+0, wpos, devrpos+0);
				}
				else if(peakr > statesrbottom[1])
				{
					byte |= 0x20;
					updatedev(peakr, devr+1, wpos, devrpos+1);
				}
				else if(peakr > statesrbottom[2])
				{
					byte |= 0x10;
					updatedev(peakr, devr+2, wpos, devrpos+2);
				}
				//got 2 nibbles, write byte
				if(bstate == 1)
				{
					wbuf[fwsize++] = byte;
					byte = 0;
					//empty buf to file
					if(fwsize == 1024*1024)
					{
						emptywbuf(fw, wbuf, fwsize);
						fwsize = 0;
					}
					wpos++;
				}
				bstate^=1;
				//first peak after
				if(statesdone == 4)
				{
					statesdone++;
					if(byte == 0x50)
						memcpy(argv[1]+strlen(argv[1])-4,".sav",4);
					else if(byte == 0xA0)
						memcpy(argv[1]+strlen(argv[1])-4,".gbc",4);
					else
					{
						printf("ERROR: not ROM or save file!\n");
						goto end_prog;
					}
					fw = fopen(argv[1],"wb");
					if(!fw)
					{
						printf("ERROR: unable to write %s!\n", argv[1]);
						goto end_prog;
					}
					//reset
					byte = 0;
					bstate = 0;
				}
			}
			if(statesdone > 1 && smpltotal-prevsmpltotal > 26)
				printf("WARNING: Possible desync at byte %i sample %i with a value of %i\n", wpos, smpltotal>>1, smpltotal-prevsmpltotal);
			prevsmpltotal = smpltotal;
			//add 9 samples before next check
			smpltotal+=18;
			smplpos+=18;
			rremain-=36;
		}
		else //no peak, add sample
		{
			smpltotal+=2;
			smplpos+=2;
			rremain-=4;
		}
	}
	//remainder remainder if any
	if(fwsize)
	{
		emptywbuf(fw, wbuf, fwsize);
		fwsize = 0;
	}
	if(wpos == 0)
	{
		printf("Unable to find any data in the provided .wav file!\n");
		printf("Make sure the file was properly recorded and normalized before\n");
		printf("Throwing it into this application!\n");
	}
	else
	{
		//print stats
		printf("Done! Wrote %i bytes\n", wpos);
		printf(" \nStatistics:\n");
		printf("Biggest Deviations from States Top to Bottom 0, 1 and 2 Left:\n");
		printf("Byte %i with %i%%, Byte %i with %i%%, Byte %i with %i%%\n", devlpos[0], 100-((devl[0]-stateslbottom[0])*100/(statesltop[0]-stateslbottom[0])), 
																			devlpos[1], 100-((devl[1]-stateslbottom[1])*100/(statesltop[1]-stateslbottom[1])),
																			devlpos[2], 100-((devl[2]-stateslbottom[2])*100/(statesltop[2]-stateslbottom[2])));
		printf("Biggest Deviations from States Top to Bottom 0, 1 and 2 Right:\n");
		printf("Byte %i with %i%%, Byte %i with %i%%, Byte %i with %i%%\n", devrpos[0], 100-((devr[0]-statesrbottom[0])*100/(statesrtop[0]-statesrbottom[0])),
																			devrpos[1], 100-((devr[1]-statesrbottom[1])*100/(statesrtop[1]-statesrbottom[1])),
																			devrpos[2], 100-((devr[2]-statesrbottom[2])*100/(statesrtop[2]-statesrbottom[2])));
		//print file hash
		printf(" \nHashes:\n");
		crc32 = ~crc32tmp;
		printf("CRC32: %08X\n", crc32);
		md5_finish(&md5ctx, md5);
		printf("MD5: ");
		for(i = 0; i < 16; i++)
			printf("%02X", md5[i]);
		printf("\n");
		sha1_finish(&sha1ctx, sha1);
		printf("SHA1: ");
		for(i = 0; i < 20; i++)
			printf("%02X", sha1[i]);
		printf("\n");
	}
end_prog:
	if(fr) fclose(fr);
	fr = NULL;
	if(fw) fclose(fw);
	fw = NULL;
	if(rbuf) free(rbuf);
	rbuf = NULL;
	if(wbuf) free(wbuf);
	wbuf = NULL;
	puts("Press enter to exit");
	getc(stdin);
	return 0;
}