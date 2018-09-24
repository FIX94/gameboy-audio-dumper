/*
 * Copyright (C) 2018 FIX94
 *
 * This software may be modified and distributed under the terms
 * of the MIT license.  See the LICENSE file for details.
 */
#include <stdio.h>
#include <string.h>
#include <inttypes.h>
int main()
{
	printf("GameBoy Audio Dumper WIP by FIX94\n");
	//read in raw sender code
	FILE *f = fopen("../sender/WRAM.bin","rb");
	if(!f)
	{
		printf("Unable to open sender code!\n");
		return 0;
	}
	fseek(f,0,SEEK_END);
	size_t sendersize = ftell(f);
	if(sendersize > 0x27C)
	{
		printf("Sender too big to be installed!\n");
		fclose(f);
		return 0;
	}
	//extend code to 0x27C bytes if needed
	uint8_t senderbuf[0x27C];
	memset(senderbuf,0,0x27C);
	rewind(f);
	fread(senderbuf,1,sendersize,f);
	fclose(f);
	//open up input log to install sender
	f = fopen("tmp/Input Log.txt","rb+");
	if(!f)
	{
		printf("Unable to open input log!\n");
		return 0;
	}
	fseek(f,0,SEEK_END);
	size_t fsize = ftell(f);
	if(fsize != 777965)
	{
		printf("Unexpected input log size!\n");
		fclose(f);
		return 0;
	}
	size_t i;
	//install DC.. section
	for(i = 0; i < 0x100; i++)
	{
		//seek to exact frame position
		fseek(f,0x3D+((53679+(i*3))*0xD),SEEK_SET);
		uint8_t v = senderbuf[0x27C-i-1];
		//print in button presses
		fprintf(f,"|%s%s%s%s%s%s%s%s.|",(v&0x40)?"U":".",(v&0x80)?"D":".",(v&0x20)?"L":".",(v&0x10)?"R":".",
			(v&0x8)?"S":".",(v&0x4)?"s":".",(v&0x2)?"B":".",(v&0x1)?"A":".");
	}
	//install DB.. section
	for(i = 0; i < 0x100; i++)
	{
		//seek to exact frame position
		fseek(f,0x3D+((55197+(i*3))*0xD),SEEK_SET);
		uint8_t v = senderbuf[0x17C-i-1];
		//print in button presses
		fprintf(f,"|%s%s%s%s%s%s%s%s.|",(v&0x40)?"U":".",(v&0x80)?"D":".",(v&0x20)?"L":".",(v&0x10)?"R":".",
			(v&0x8)?"S":".",(v&0x4)?"s":".",(v&0x2)?"B":".",(v&0x1)?"A":".");
	}
	//install DA.. section
	for(i = 0; i < 0x7C; i++)
	{
		//seek to exact frame position
		fseek(f,0x3D+((56135+(i*3))*0xD),SEEK_SET);
		uint8_t v = senderbuf[0x7C-i-1];
		//print in button presses
		fprintf(f,"|%s%s%s%s%s%s%s%s.|",(v&0x40)?"U":".",(v&0x80)?"D":".",(v&0x20)?"L":".",(v&0x10)?"R":".",
			(v&0x8)?"S":".",(v&0x4)?"s":".",(v&0x2)?"B":".",(v&0x1)?"A":".");
	}
	//done
	printf("Added sender to inputs!\n");
	fclose(f);
	return 0;
}