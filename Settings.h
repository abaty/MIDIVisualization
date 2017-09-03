#pragma once
#include<string>
#include<mutex>

//std::string inputFile = "testFiles/FugueMovie/Fugue";
std::string inputFile = "testFiles/Meditation7_in_FSharpMinor/Meditation7";

/*TODO
---visualization
- fix unisons
- slurs/continuations, something moving between connected notes/parts
- make played notes change size?
- fix overlaps on things half a note apart (again)

--- sync w/ real music via FFT
- uncomment input file line in readWav.cpp
*/

//generalSettings
bool doImageVolumeScaling = true;
bool doVideoVolumeScaling = true;
double volScaleMax = 0.25;//units of yNoteWidth
double volMaxModifier = 2;//applied to volScalMax only to increase range on loud end
bool doHighlightDecay = true;
bool doHighlightAttenuationInY = false;
bool doVideoBarLines = true;
bool doVideoStaffLines = true;
bool doMiniScore = true;
int miniScoreYPix = 120;
bool usedFixedYNoteWidth = false;

//debugging and other items
bool doAlternateFrameBGColor = false;//for frame scrolling debug
bool stopEarly = false;
int stopAfterNFrames = 300;
bool noFrameScanning = false;//false means that every frame image is output with the 0 of hte x axis redefined in order to give smooth video
								//do not use w/ doMiniScore

//video settings
bool doMultiThreading = true;
const int nThreads = 6;
double timeBetweenThreadUpdates = 20;
int startingFrame = 0;//used if you want to start from a mid-point (due to failure or something, otherwise leave at 0)
bool makeVideo = true;
int vidnPixX = 1280;
int vidnPixY = 720;
//int vidnPixX = 1920;
//int vidnPixY = 1080;
int FPS = 60;
float scrollSpeedFactor = 0.125;// *3. / 2.0; //controls the conversion from midi units to pixels (factor*units = pixels)
								//assuming 480 midiunits/beat and 120 BPM, then we scan at 240 pix/s if this is 1.0
								//higher value = faster scanning (less notes at one time), lower = slower
								//best results with a lower value usually

//image settings
bool makeImage = true;
bool doImageBarLines = false;
bool doImageStaffLines = true;
bool calculateLengthBySelf = true; // overwrites nPixX, only works is doVideo is off
int nPixX = 2600;
int nPixY = 600;

int xBufferSize = 10;//not used in video mode unless have miniScore
int yBufferSizeTop = 0;//used in video mode
int yBufferSizeBottom = 0;

//colors for each note, starting with C,C#,D...
//first index is R,G,B
const unsigned char colorArrayOffset = 0; //optional thing that 'rotates the colors around by 1 each (from 0-12) if you want to change the aestetics
unsigned char colorArray[3][12] = { {254,0  ,255,63  ,232,196,15 ,255,0  ,255,99 ,142},
									{0  ,199,147,78 ,232,0  ,173,101,131,197,1  ,200},
									{2  ,249,0  ,254,0  ,124,0  ,1  ,238,3  ,166,2  } };
double hueArray[12] = {0};
//unsigned char colorArrayLight[3][12] = {	{ 255,38 ,255,18 ,255,255,18 ,254,9  ,255,160,183 },//more white in these colors forhighlighting (otherwise the same)
//											{ 64 ,211,170,36 ,255,34 ,242,134,145,217,20 ,253 },
//											{ 64 ,255,55 ,254,130,172,0  ,54 ,255,85 ,254,19 } };
unsigned char colorArrayLight[3][12] = { { 255,255,255,255,255,255,255,255,255,255,255,255 },//more white in these colors forhighlighting (otherwise the same)
{ 255,255,255,255,255,255,255,255,255,255,255,255 },
{ 255,255,255,255,255,255,255,255,255,255,255,255 } };

unsigned char staffColor[3] = { 111,111,111 };
unsigned char stavePitch[10] = {43,47,50,53,57,64,67,71,74,77};
unsigned char miniScoreHighlight[3] = {50,50,50};

//note struct
struct noteDat {
	int beginning;
	int end;
	unsigned char pitch;
	unsigned char velocity;
};

const float tempoScale = 1000;

//multithreading diagnostic structures
int assignedFrames[nThreads] = { 0 };
int completedFrames[nThreads] = { 0 };
double totalExecutionTime[nThreads] = { 0 };
int threadStartingFrame[nThreads] = { 0 };
int lastThreadFrame[nThreads] = { 0 };

//multithreading safety mutexs
std::mutex completed, execTime;