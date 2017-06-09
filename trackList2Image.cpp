#include "stdafx.h"
#include "Settings.h"
#include "midiTrack.h"
#include <iostream>  
#include <limits>
#include <vector>

void setColor(unsigned char R, unsigned char G, unsigned char B, int x, int y, std::vector< std::vector< std::vector < unsigned char > > >& imageValues) {
	std::vector< unsigned char > RGBColorTemp;
	RGBColorTemp.push_back(B);
	RGBColorTemp.push_back(G);
	RGBColorTemp.push_back(R);
	imageValues.at(x).erase(imageValues.at(x).begin() + y);
	imageValues.at(x).insert(imageValues.at(x).begin() + y,RGBColorTemp);
}

void trackList2Image(std::vector< MidiTrack >& trackList, std::vector< std::vector< std::vector < unsigned char > > >& imageValues) {
	//get usable area inside margins
	const int xRange = nPixX - 2 * xBufferSize;
	const int yRange = nPixY - yBufferSizeBottom - yBufferSizeTop;

	//find the total time of the song and the scaling of time to pixels
	int lastSoundTime = 0;
	for (unsigned int i = 0; i < trackList.size(); i++) {
		if (lastSoundTime < trackList.at(i).GetEnd()) lastSoundTime = trackList.at(i).GetEnd();
	}
	float tScale = (float)xRange / lastSoundTime;

	//calculate the width of each note in Y (128.0 notes in MIDI, but use 120 so middle C is in middle)
	double yNoteWidth = yRange / 120.0;

	//calculate staff lines
	std::vector< int > staffLines;
	for (int n = 0; n < 10; n++) {
		staffLines.push_back((int)(nPixY - ((stavePitch[n] * yNoteWidth) + yBufferSizeBottom) - yNoteWidth / 2.0));
	}
	
	//set the staff lines
	for (int y = 0; y < nPixY; y++) {

		if(std::find(staffLines.begin(), staffLines.end(), y) != staffLines.end()){//check if is a staffline
			std::cout << "Putting Staff Line..." << std::endl;
			for (int x = 0; x < nPixX; x++) {
				imageValues.at(x).at(y).at(0) = staffColor[0];
				imageValues.at(x).at(y).at(1) = staffColor[1];
				imageValues.at(x).at(y).at(2) = staffColor[2];
			}
		}
	}
	std::cout << "Initialization Done!" << std::endl;

	std::cout << "Starting to put tracks into image..." << std::endl;
	for (unsigned int i = 0; i < trackList.size(); i++) {
	//for (unsigned int i = 1; i < 2; i++) {
		int nNotes = trackList.at(i).GetNumberOfNotes();
		std::cout << "Making image for track " << i << " which has " << nNotes << " notes." << std::endl;
		for (int j = 0; j < nNotes; j++) {
		//for (int j = 0; j < 10; j++) {
			if (j % 100 == 0) std::cout << "     On note " << j << " out of " << nNotes << std::endl;

			//define length and width of a rectangle to be the note
			int xStart = (int)(trackList.at(i).GetNote(j).beginning*tScale) + xBufferSize;
			int xEnd = (int)(trackList.at(i).GetNote(j).end*tScale) + xBufferSize;
			xEnd = xEnd - (((xEnd-xStart)>1)?1:0); //subtract 1 pixel to put some space if there are repeated notes, but don't delete 1 pixel notes
			int yEnd = nPixY-((int)(trackList.at(i).GetNote(j).pitch*yNoteWidth) + yBufferSizeBottom);
			int yStart = yEnd - (int)yNoteWidth;
			unsigned char colorIndx = (trackList.at(i).GetNote(j).pitch + colorArrayOffset) % 12;

			for (int x = xStart; x < xEnd; x++) {
				for (int y = yStart; y < yEnd; y++) {
					setColor(colorArray[0][colorIndx], colorArray[1][colorIndx], colorArray[2][colorIndx],x,y,imageValues);
				}
			}
		}
	}

	return;
}