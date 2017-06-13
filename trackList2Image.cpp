#include "stdafx.h"
#include "Settings.h"
#include "midiTrack.h"
#include <iostream>  
#include <limits>
#include <vector>
#include <cmath>

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

	//calculate range of music
	int lowestNote = 128;
	int highestNote = 0;
	for (unsigned int i = 0; i < trackList.size(); i++) {
		if (lowestNote > trackList.at(i).getLowestNote()) lowestNote = trackList.at(i).getLowestNote();
		if (highestNote < trackList.at(i).getHighestNote()) highestNote = trackList.at(i).getHighestNote();
	}
	//symmetrize around middle C (60)
	double pitchRange = 2*fmax(abs(lowestNote - 2 - 60), abs(highestNote + 2 - 60))+1;
	double pitchOffset = 60 - fmax(abs(lowestNote - 2 - 60), abs(highestNote + 2 - 60));

	//calculate the width of each note in Y (128.0 notes in MIDI)
	double yNoteWidth = yRange / pitchRange;
	if (usedFixedYNoteWidth) {
		yNoteWidth = yRange / 120.0;
		pitchOffset = 0;
	}

	//calculate staff lines
	std::vector< int > staffLines;
	for (int n = 0; n < 10; n++) {
		staffLines.push_back((int)(nPixY - (((stavePitch[n] - pitchOffset) * yNoteWidth) + yBufferSizeBottom) - yNoteWidth / 2.0));
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
			int yEnd = nPixY-((int)((trackList.at(i).GetNote(j).pitch - pitchOffset)*yNoteWidth) + yBufferSizeBottom);
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

void trackList2Video(std::vector< MidiTrack >& trackList, std::vector< std::vector< std::vector < unsigned char > > >& imageValues, int n, int nFrames, double midiUnitsPerFrame) {
	//get usable area inside margins
	const int xRange = nPixX;
	const int yRange = nPixY - yBufferSizeBottom - yBufferSizeTop;

	//find the total time of the song and the scaling of time to pixels
	int lastSoundTime = 0;
	for (unsigned int i = 0; i < trackList.size(); i++) {
		if (lastSoundTime < trackList.at(i).GetEnd()) lastSoundTime = trackList.at(i).GetEnd();
	}
	float midi2pix = scrollSpeedFactor/ tempoScale;

	//calculate range of music
	int lowestNote = 128;
	int highestNote = 0;
	for (unsigned int i = 0; i < trackList.size(); i++) {
		if (lowestNote > trackList.at(i).getLowestNote()) lowestNote = trackList.at(i).getLowestNote();
		if (highestNote < trackList.at(i).getHighestNote()) highestNote = trackList.at(i).getHighestNote();
	}
	//symmetrize around middle C (60)
	double pitchRange = 2 * fmax(abs(lowestNote - 2 - 60), abs(highestNote + 2 - 60)) + 1;
	double pitchOffset = 60 - fmax(abs(lowestNote - 2 - 60), abs(highestNote + 2 - 60));

	//calculate the width of each note in Y (128.0 notes in MIDI)
	double yNoteWidth = yRange / pitchRange;
	if (usedFixedYNoteWidth) {
		yNoteWidth = yRange / 120.0;
		pitchOffset = 0;
	}

	//calculate staff lines
	std::vector< int > staffLines;
	for (int n = 0; n < 10; n++) {
		staffLines.push_back((int)(nPixY - (((stavePitch[n] - pitchOffset) * yNoteWidth) + yBufferSizeBottom) - yNoteWidth / 2.0));
	}

	//if we are on rendering frame n, then we have shifted right by a total of (n-1)*midiUnitsPerFrame*midi2pix pixels
	//so this frame we need to black out pixels (n-1)*midiUnitsPerFrame*tScale to (n*midiUnitsPerFrame*midi2pix)-1 pixels
	//because we are doing this in a scanning way through our array, this becomes (n-1)*midiUnitsPerFrame*midi2pix % (nPixX) to (n*midiUnitsPerFrame*midi2pix)-1 % (nPixX)
	//must take wrap-around into account here!
	//blackout a set of columns that have gone out the back of the frame, and use them to specify new information at the front of the frame (keep rest unchanged)
	if (n != 0) {//not needed for first frame
		int boundary1 = (int)((n - 1)*midiUnitsPerFrame*midi2pix) % (nPixX);//inclusive
		int boundary2 = (int)((n*midiUnitsPerFrame*midi2pix) - 1) % (nPixX);//inclusive

		unsigned char c = 0;
		if (doAlternateFrameBGColor) c = (n % FPS) * 255 / (FPS-1);
		if (boundary1 < boundary2) {
			for (int x = boundary1; x <= boundary2; x++) {
				for (int y = 0; y < nPixY; y++) {
					imageValues.at(x).at(y).at(0) = c;
					imageValues.at(x).at(y).at(1) = c;
					imageValues.at(x).at(y).at(2) = c;
				}
			}
		}
		else {//wrap around case
			for (int x = 0; x <= boundary2; x++) {
				for (int y = 0; y < nPixY; y++) {
					imageValues.at(x).at(y).at(0) = c;
					imageValues.at(x).at(y).at(1) = c;
					imageValues.at(x).at(y).at(2) = c;
				}
			}
			for (int x = boundary1; x < nPixX; x++) {
				for (int y = 0; y < nPixY; y++) {
					imageValues.at(x).at(y).at(0) = c;
					imageValues.at(x).at(y).at(1) = c;
					imageValues.at(x).at(y).at(2) = c;
				}
			}
		}
	}

	//set the staff lines
	for (int y = 0; y < nPixY; y++) {
		if (std::find(staffLines.begin(), staffLines.end(), y) != staffLines.end()) {//check if is a staffline
			//std::cout << "Putting Staff Line..." << std::endl;
			for (int x = 0; x < nPixX; x++) {
				if (imageValues.at(x).at(y).at(0) == 0 && imageValues.at(x).at(y).at(1) == 0 && imageValues.at(x).at(y).at(2) == 0) {//only overwrite black space
					imageValues.at(x).at(y).at(0) = staffColor[0];
					imageValues.at(x).at(y).at(1) = staffColor[1];
					imageValues.at(x).at(y).at(2) = staffColor[2];
				}
			}
		}
	}
	//std::cout << "Initialization Done!" << std::endl;

	std::cout << "Starting to put tracks into frame..." << std::endl;
	for (unsigned int i = 0; i < trackList.size(); i++) {
	//for (unsigned int i = 2; i < 3; i++) {//for debug
		int nNotes = trackList.at(i).GetNumberOfNotes();
		if( nNotes > 0 ) std::cout << "     Making image for track " << i << " which has " << nNotes << " remaining notes." << std::endl;
		for (int j = 0; j < nNotes; j++) {
			//for (int j = 0; j < 10; j++) {//for debug
			//if (j % 100 == 0) std::cout << " On note " << j+1 << " out of " << nNotes << std::endl;
		
			//define length and width of a rectangle to be the note
			int xStart = (int)(trackList.at(i).GetNote(j).beginning*midi2pix +  nPixX/2.0);//in absolute pixels
			int xEnd = (int)(trackList.at(i).GetNote(j).end*midi2pix + nPixX / 2.0);//in absolute pixels
			xEnd = xEnd - (((xEnd - xStart)>1) ? 1 : 0); //subtract 1 pixel to put some space if there are repeated notes, but don't delete 1 pixel notes
			xEnd = xEnd - (((xEnd - xStart)>15) ? 1 : 0); //subtract 1 extra pixel to put some space for longer notes
			xEnd = xEnd - (((xEnd - xStart)>30) ? 1 : 0); //subtract 1 extra pixel to put some space for longer notes

			
			bool isStartNotInNewFrame = (n != 0) && (xStart > ((int)((n*midiUnitsPerFrame*midi2pix) - 1) + nPixX));//continue if note is not in new part of frame yet
			isStartNotInNewFrame = isStartNotInNewFrame || ((n == 0) && (xStart >= nPixX));// slightly larger range for first frame
			bool isEndNotInNewFrame = (n != 0) && (xEnd < ((int)((n - 1)*midiUnitsPerFrame*midi2pix) + nPixX));

			bool doHighlightNote = (xStart <= nPixX / 2 + n*midiUnitsPerFrame*midi2pix) && (xEnd > nPixX / 2 + n*midiUnitsPerFrame*midi2pix);//highlight if spans the central column of pixels
			bool undoHighlightNote = (xStart <= nPixX / 2 + (n-1)*midiUnitsPerFrame*midi2pix) && (xEnd > nPixX / 2 + (n-1)*midiUnitsPerFrame*midi2pix) && !doHighlightNote;//check if highlighted last frame

			if (xEnd < ((int)(n*midiUnitsPerFrame*midi2pix))) {//assuming time-ordered notes, delete notes from memory that have already been fully used
				trackList.at(i).DeleteNote(j);
				continue;
			}

			if (isStartNotInNewFrame) break;//assuming time-ordered notes, stop processing this track after get to notes in future
			if (n!=0 && !doHighlightNote && !undoHighlightNote && isEndNotInNewFrame) continue; //highlight control

            //prevent overflow if the note starts in the new frame but isn't over before end of new frame
			if (xEnd > ((int)((n*midiUnitsPerFrame*midi2pix) - 1) + nPixX)) xEnd = ((int)((n*midiUnitsPerFrame*midi2pix) - 1) + nPixX);
			
			int xStartInFrame = xStart % (nPixX);
			int xEndInFrame = xEnd % (nPixX);

			//y values no different than static image case
			int yEnd = nPixY - ((int)((trackList.at(i).GetNote(j).pitch - pitchOffset)*yNoteWidth) + yBufferSizeBottom);
			int yStart = yEnd - (int)yNoteWidth;
			unsigned char colorIndx = (trackList.at(i).GetNote(j).pitch + colorArrayOffset) % 12;

			//put new notes into frame
			if (xStartInFrame <= xEndInFrame) {
				for (int x = xStartInFrame; x <= xEndInFrame; x++) {
					for (int y = yStart; y < yEnd; y++) {
						if(!doHighlightNote) setColor(colorArray[0][colorIndx], colorArray[1][colorIndx], colorArray[2][colorIndx], x, y, imageValues);
						else setColor(colorArrayLight[0][colorIndx], colorArrayLight[1][colorIndx], colorArrayLight[2][colorIndx], x, y, imageValues);
					}
				}
			}
			else {//wrap around case
				for (int x = 0; x <= xEndInFrame; x++) {
					for (int y = yStart; y < yEnd; y++) {
						if (!doHighlightNote) setColor(colorArray[0][colorIndx], colorArray[1][colorIndx], colorArray[2][colorIndx], x, y, imageValues);
						else setColor(colorArrayLight[0][colorIndx], colorArrayLight[1][colorIndx], colorArrayLight[2][colorIndx], x, y, imageValues);
					}
				}
				for (int x = xStartInFrame; x < nPixX; x++) {
					for (int y = yStart; y < yEnd; y++) {
						if (!doHighlightNote) setColor(colorArray[0][colorIndx], colorArray[1][colorIndx], colorArray[2][colorIndx], x, y, imageValues);
						else setColor(colorArrayLight[0][colorIndx], colorArrayLight[1][colorIndx], colorArrayLight[2][colorIndx], x, y, imageValues);
					}
				}
			}
		}
	}

	return;
}