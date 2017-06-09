#pragma once
#include<string>

std::string inputFile = "testFiles/Fugue";

bool calculateLengthBySelf = true; // overwrites nPixX
int nPixX = 2400;
int nPixY = 600;

int xBufferSize = 50;
int yBufferSizeTop = 0;
int yBufferSizeBottom = 0;

//colors for each note, starting with C,C#,D...
//first index is R,G,B
const unsigned char colorArrayOffset = 0; //optional thing that 'rotates the colors around by 1 each (from 0-12) if you want to change the aestetics
unsigned char colorArray[3][12] = { {255,16 ,219,99 ,123,240,10 ,246,47 ,165,162,83 },
									{5  ,164,195,75 ,198,64 ,156,107,82 ,190,75 ,159},
									{6  ,238,0  ,173,53 ,128,143,2  ,163,0  ,172,50 } };

unsigned char staffColor[3] = { 111,111,111 };
unsigned char stavePitch[10] = {43,47,50,53,57,64,67,71,74,77};


//note struct
struct noteDat {
	int beginning;
	int end;
	unsigned char pitch;
	unsigned char velocity;
};