// Visualization.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "midi2ascii.cpp"
#include "Input.cpp"
#include "Settings.h"
#include "midiTrack.h"
#include "trackList2Image.cpp"
#include "CImg-2.0.0\CImg.h"
#include <iostream>  
#include <limits>
#include <vector>
#include <string>
#include "LodePng.cpp"
#include "decodeBMP.cpp"
#include <ctime>

using namespace cimg_library;

void makeVisual(bool doVideo = false)
{
	std::clock_t start;

	//convert MIDI to text file
	std::string temporaryFileName = inputFile;
	std::string temporaryFileName2 = inputFile;
	std::cout << "Parsing MIDI FILE: " << inputFile << std::endl;
	Midi2Ascii(temporaryFileName.append(".mid"), temporaryFileName2.append(".txt"));
	std::cout << "Finished With MIDI File" << std::endl;

	//read in text file as notes
	std::vector< MidiTrack > trackList;
	double duration = 0;
	readInputFile(temporaryFileName2, trackList, duration);

	//some formatting
	if (doVideo) {
		nPixX = vidnPixX;
		nPixY = vidnPixY;
	}

	int lastTime = 0;
	for (unsigned int i = 0; i < trackList.size(); i++){
		std::cout << "Track " << i << " has " << trackList.at(i).GetNumberOfNotes() << " notes." << std::endl;
		if (trackList.at(i).GetEnd() > lastTime) lastTime = trackList.at(i).GetEnd();
	}
	if(calculateLengthBySelf && !doVideo) nPixX = lastTime / 60;

	int nFrames = 1, midiUnitsPerFrame = 0;
	if (doVideo) {
		nFrames = ((int)duration+1)*FPS;//calculate length of video in frames (round to nearest upper second
		std::cout << "duration (s): " << duration << "  Last time in midi units: " << lastTime << "  FPS: " << FPS << std::endl;
		midiUnitsPerFrame = (double)lastTime / (double)(duration*FPS);//calculate the length of each frame in terms of midi units (don't round to nearest here)
	}

	nPixX = nPixX % 4 + nPixX; // make sure divisible by 4 for bitmap

	//calculate what the image looks like
	start = std::clock();
	std::vector< std::vector< std::vector < unsigned char > > > imageValues(nPixX, std::vector< std::vector < unsigned char > >(nPixY, std::vector < unsigned char >(3,0)));
	for (int n = 0; n < (stopEarly? stopAfterNFrames : nFrames); n++){// nFrames; n++) {
		if (doVideo) std::cout << "Rendering frame " << n << " out of " << (stopEarly ? stopAfterNFrames : nFrames) << std::endl;

		if (!doVideo) trackList2Image(trackList, imageValues);
		else          trackList2Video(trackList, imageValues, n, nFrames, midiUnitsPerFrame);

		//handing data over to Image library
		if(!doVideo) std::cout << "\nMaking Image..." << std::endl;
		CImg<unsigned char> img(nPixX, nPixY, 1, 3);
		for (int y = 0; y < nPixY; y++) {
			for (int x = 0; x < nPixX; x++) {
				unsigned char red = 0, green = 0, blue = 0;
				if (!doVideo || noFrameScanning) {
					img(x, y, 0) = imageValues.at(x).at(y).at(2);//R
					img(x, y, 1) = imageValues.at(x).at(y).at(1);//G
					img(x, y, 2) = imageValues.at(x).at(y).at(0);//B
				}
				else
				{
					img(x, y, 0) = imageValues.at(((int)(n*midiUnitsPerFrame*scrollSpeedFactor/ tempoScale + x ) % nPixX)).at(y).at(2);//R
					img(x, y, 1) = imageValues.at(((int)(n*midiUnitsPerFrame*scrollSpeedFactor/ tempoScale + x ) % nPixX)).at(y).at(1);//G
					img(x, y, 2) = imageValues.at(((int)(n*midiUnitsPerFrame*scrollSpeedFactor/ tempoScale + x ) % nPixX)).at(y).at(0);//B
				}
			}
		}
		temporaryFileName = inputFile;
		img.save("temporary.bmp");

		//convert to png
		std::vector<unsigned char> bmp;
		lodepng::load_file(bmp, "temporary.bmp");
		std::vector<unsigned char> image;
		unsigned w, h;
		unsigned error = decodeBMP(image, w, h, bmp);
		std::vector<unsigned char> png;
		error = lodepng::encode(png, image, w, h);
		temporaryFileName = inputFile;
		if(!doVideo) lodepng::save_file(png, temporaryFileName.append(".png").data());
		else {
			temporaryFileName += std::to_string(100000+n);//add 100000 just to ensure all have same number of sig figs (poor man's way)
			lodepng::save_file(png, temporaryFileName.append(".png").data());
		}
	}
	double runTime = (std::clock() - start) / (double)CLOCKS_PER_SEC;
	std::cout << "\nRendering took " << runTime << " seconds (" << runTime/(double)(stopEarly ? stopAfterNFrames : nFrames) << " s/frame)." << std::endl;
}

int main()
{
	if(!makeVideo || makeImage) makeVisual(false);
	if(makeVideo) makeVisual(true);
	std::cout << "FINISHED! \n Press ENTER to continue...";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return 0;
}

