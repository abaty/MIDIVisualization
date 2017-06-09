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

using namespace cimg_library;

void makeVisual(bool useTestValues = false)
{
	//convert MIDI to text file
	std::string temporaryFileName = inputFile;
	std::string temporaryFileName2 = inputFile;
	std::cout << "Parsing MIDI FILE: " << inputFile << std::endl;
	Midi2Ascii(temporaryFileName.append(".mid"), temporaryFileName2.append(".txt"));
	std::cout << "Finished With MIDI File" << std::endl;

	//read in text file as notes
	std::vector< MidiTrack > trackList;
	readInputFile(temporaryFileName2, trackList);

	//some formatting
	int lastTime = 0;
	if (calculateLengthBySelf) {
		for (unsigned int i = 0; i < trackList.size(); i++){
			std::cout << "Track " << i << " has " << trackList.at(i).GetNumberOfNotes() << " notes." << std::endl;
			if (trackList.at(i).GetEnd() > lastTime) lastTime = trackList.at(i).GetEnd();
		}
		nPixX = lastTime / 60;	
	}
	nPixX = nPixX % 4 + nPixX; // make sure divisible by 4 for bitmap

	//calculate what the image looks like
	std::vector< std::vector< std::vector < unsigned char > > > imageValues(nPixX, std::vector< std::vector < unsigned char > >(nPixY, std::vector < unsigned char >(3)));
	trackList2Image(trackList, imageValues);
	
	//handing data over to Image library
	std::cout << "\nMaking Image..." << std::endl;
	CImg<unsigned char> img(nPixX, nPixY, 1, 3);
	for (int y = 0; y < nPixY; y++){
		for (int x = 0; x < nPixX - 1; x++){
			unsigned char red = 0, green = 0, blue = 0;
			img(x,y,0) = imageValues.at(x).at(y).at(2);//R
			img(x,y,1) = imageValues.at(x).at(y).at(1);//G
			img(x,y,2) = imageValues.at(x).at(y).at(0);//B
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
	lodepng::save_file(png, temporaryFileName.append(".png").data());
}

int main()
{
	makeVisual(false);
	std::cout << "FINISHED! \n Press ENTER to continue...";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return 0;
}

