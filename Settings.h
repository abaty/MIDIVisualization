#pragma once
#include<string>

std::string inputFile = "testFiles/FugueMovie/Fugue";

//debugging and useful items
bool doAlternateFrameBGColor = true;//for frame scrolling debug
bool stopEarly = true;
int stopAfterNFrames = 10;
bool noFrameScanning = false;//false means that every frame image is output with the 0 of hte x axis redefined in order to give smooth video

//video settings
bool makeVideo = true;
int vidnPixX = 1280;
int vidnPixY = 720;
int FPS = 25;// 25;
bool usedFixedYNoteWidth = false;
float scrollSpeedFactor = 0.125; //controls the conversion from midi units to pixels (factor*units = pixels)
								//assuming 480 midiunits/beat and 120 BPM, then we scan at 240 pix/s if this is 1.0
								//higher value = faster scanning (less notes at one time), lower = slower
								//best results with a lower value usually

//image settings
bool makeImage = false;
bool calculateLengthBySelf = true; // overwrites nPixX, only works is doVideo is off
int nPixX = 2400;
int nPixY = 600;

int xBufferSize = 50;//not used in video mode
int yBufferSizeTop = 0;//used in video mode
int yBufferSizeBottom = 0;

//colors for each note, starting with C,C#,D...
//first index is R,G,B
const unsigned char colorArrayOffset = 0; //optional thing that 'rotates the colors around by 1 each (from 0-12) if you want to change the aestetics
unsigned char colorArray[3][12] = { {254,0  ,255,1  ,232,196,15 ,255,0  ,255,99 ,142},
									{0  ,162,147,15 ,232,0  ,173,101,100,197,1  ,200},
									{2  ,200,0  ,163,0  ,124,0  ,1  ,180,3  ,166,2  } };
//unsigned char colorArrayLight[3][12] = {	{ 255,38 ,255,18 ,255,255,18 ,254,9  ,255,160,183 },//more white in these colors forhighlighting (otherwise the same)
//											{ 64 ,211,170,36 ,255,34 ,242,134,145,217,20 ,253 },
//											{ 64 ,255,55 ,254,130,172,0  ,54 ,255,85 ,254,19 } };
unsigned char colorArrayLight[3][12] = { { 255,255,255,255,255,255,255,255,255,255,255,255 },//more white in these colors forhighlighting (otherwise the same)
{ 255,255,255,255,255,255,255,255,255,255,255,255 },
{ 255,255,255,255,255,255,255,255,255,255,255,255 } };

unsigned char staffColor[3] = { 111,111,111 };
unsigned char stavePitch[10] = {43,47,50,53,57,64,67,71,74,77};

const float tempoScale = 100;


//note struct
struct noteDat {
	int beginning;
	int end;
	unsigned char pitch;
	unsigned char velocity;
};