#include "stdafx.h"
#include "Settings.h"
#include "midiTrack.h"
#include <iostream>  
#include <limits>
#include <vector>
#include <cmath>
#include <algorithm>
#include "colorTools.h"

inline void setColor(unsigned char R, unsigned char G, unsigned char B, int x, int y, std::vector< std::vector< std::vector < unsigned char > > >& imageValues) {
	std::vector< unsigned char > RGBColorTemp{ B,G,R };
	imageValues.at(x).at(y) = RGBColorTemp;
}

inline bool isBackground(int x, int y, std::vector< std::vector< std::vector < unsigned char > > >& imageValues) {
	if (imageValues.at(x).at(y).at(0) == 0 && imageValues.at(x).at(y).at(1) == 0 && imageValues.at(x).at(y).at(2) == 0) return true;
	if (imageValues.at(x).at(y).at(0) == staffColor[0] && imageValues.at(x).at(y).at(1) == staffColor[1] && imageValues.at(x).at(y).at(2) == staffColor[2]) return true;
	return true;
}

inline bool isAnotherNote(int x, int y, std::vector< std::vector< std::vector < unsigned char > > >& imageValues, int colorIndx) {
	//old implementation
	//if (imageValues.at(x).at(y).at(0) == colorArray[2][colorIndx] && imageValues.at(x).at(y).at(1) == colorArray[1][colorIndx] && imageValues.at(x).at(y).at(2) == colorArray[0][colorIndx]) return false;
	//if (imageValues.at(x).at(y).at(0) == colorArrayLight[2][colorIndx] && imageValues.at(x).at(y).at(1) == colorArrayLight[1][colorIndx] && imageValues.at(x).at(y).at(2) == colorArrayLight[0][colorIndx]) return false;

	if (fabs(RGBToHSL(imageValues.at(x).at(y).at(2), imageValues.at(x).at(y).at(1), imageValues.at(x).at(y).at(0)).at(0)-hueArray[colorIndx])<0.001) return false;

	return true;
}


void trackList2Image(std::vector< MidiTrack >& trackList, std::vector< std::vector< std::vector < unsigned char > > >& imageValues, std::vector< int >& barLines, int nPixX_, int nPixY_) {
	//get usable area inside margins
	//std::cout << "Image Mode!" << std::endl;
	const int xRange = nPixX_ - 2 * xBufferSize;
	const int yRange = nPixY_ - yBufferSizeBottom - yBufferSizeTop;

	//find the total time of the song and the scaling of time to pixels
	int lastSoundTime = 0;
	int firstSoundTime = 10000000000;
	for (unsigned int i = 0; i < trackList.size(); i++) {
		if (lastSoundTime < trackList.at(i).GetEnd()) lastSoundTime = trackList.at(i).GetEnd();
		if (firstSoundTime > trackList.at(i).GetStart()) firstSoundTime = trackList.at(i).GetStart();
	}
	float tScale = (float)xRange / (float)lastSoundTime;

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
	//std::cout << "Calculating Staff Lines" << std::endl;
	for (int n = 0; n < 10; n++) {
		staffLines.push_back((int)(nPixY_ - (((stavePitch[n] - pitchOffset) * yNoteWidth) + yBufferSizeBottom) - yNoteWidth / 2.0));
	}
	
	//set the staff lines
	if (doImageStaffLines) {
		for (int y = 0; y < nPixY_; y++) {
			if (std::find(staffLines.begin(), staffLines.end(), y) != staffLines.end()) {//check if is a staffline
				//std::cout << "Putting Staff Line..." << std::endl;
				for (int x = 0; x < nPixX_; x++) {
					imageValues.at(x).at(y).at(0) = staffColor[0];
					imageValues.at(x).at(y).at(1) = staffColor[1];
					imageValues.at(x).at(y).at(2) = staffColor[2];
				}
			}
		}
	}

	if (doImageBarLines) {
		for (size_t t = 0; t < barLines.size(); t++) {//set BarLines
			if (firstSoundTime >= barLines.at(t)) continue;
			int barX = (int)(barLines.at(t)*tScale + xBufferSize);
			for (int y = staffLines.at(9); y < staffLines.at(5); y++) {
				setColor(staffColor[0], staffColor[1], staffColor[2], barX, y, imageValues);
			}
			for (int y = staffLines.at(4); y < staffLines.at(0); y++) {
				setColor(staffColor[0], staffColor[1], staffColor[2], barX, y, imageValues);
			}
		}
	}

	//std::cout << "Initialization Done!" << std::endl;

	//std::cout << "Starting to put tracks into image..." << std::endl;
	for (unsigned int i = 0; i < trackList.size(); i++) {
	//for (unsigned int i = 1; i < 2; i++) {
		int nNotes = trackList.at(i).GetNumberOfNotes();
		//std::cout << "Making image for track " << i << " which has " << nNotes << " notes." << std::endl;
		for (int j = 0; j < nNotes; j++) {
		//for (int j = 0; j < 10; j++) {
			//if (j % 100 == 0) std::cout << "     On note " << j << " out of " << nNotes << std::endl;

			//define length and width of a rectangle to be the note
			int xStart = (int)(trackList.at(i).GetNote(j).beginning*tScale) + xBufferSize;
			int xEnd = (int)(trackList.at(i).GetNote(j).end*tScale) + xBufferSize;
			xEnd = xEnd - (((xEnd-xStart)>2)?1:0); //subtract 1 pixel to put some space if there are repeated notes, but don't delete 1 pixel notes
			int yEnd = nPixY_-((int)((trackList.at(i).GetNote(j).pitch - pitchOffset)*yNoteWidth) + yBufferSizeBottom);
			int yStart = yEnd - (int)yNoteWidth;
			unsigned char colorIndx = (trackList.at(i).GetNote(j).pitch + colorArrayOffset) % 12;

			//scale for velocities
			double volScale = 0;// units of yNoteWidth, level off under 32 or over 96 
			if (doImageVolumeScaling) volScale = std::min(volScaleMax*volMaxModifier, std::max(-2 * volScaleMax+4 * volScaleMax*trackList.at(i).GetNote(j).velocity / 128.0, -volScaleMax));
			//std::cout << yNoteWidth << " " << volScale << " " << yStart - (int)(volScale*yNoteWidth) << " " << yEnd + (int)volScale*yNoteWidth << std::endl;

			for (int x = xStart; x < xEnd; x++) {
				for (int y = yStart - (int)(volScale*yNoteWidth); y < yEnd + (int)(volScale*yNoteWidth); y++) {
					if ((y < yStart || y >= yEnd) && isAnotherNote(x, y, imageValues, colorIndx) && !isBackground(x, y, imageValues)) continue;//avoid color collisions when loud
					setColor(colorArray[0][colorIndx], colorArray[1][colorIndx], colorArray[2][colorIndx],x,y,imageValues);
				}
			}
		}
	}

	return;
}

//render full frame should be true the first time this is called, but then can be set to false to speed up stuff
void trackList2Video(std::vector< MidiTrack >& trackList, std::vector< std::vector< std::vector < unsigned char > > >& imageValues, int n, int nFrames, double midiUnitsPerFrame, std::vector< int >& barLines, bool renderFullFrame = false) {
	//get usable area inside margins
	const int xRange = nPixX;
	const int yRange = nPixY - yBufferSizeBottom - yBufferSizeTop;

	//find the total time of the song and the scaling of time to pixels
	int lastSoundTime = 0;
	int firstSoundTime = 10000000000;
	for (unsigned int i = 0; i < trackList.size(); i++) {
		if (lastSoundTime < trackList.at(i).GetEnd()) lastSoundTime = trackList.at(i).GetEnd();
		if (firstSoundTime > trackList.at(i).GetStart()) firstSoundTime = trackList.at(i).GetStart();
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
	//0 is highest one
	std::vector< int > staffLines;
	for (int n = 0; n < 10; n++) {
		staffLines.push_back((int)(nPixY - (((stavePitch[n] - pitchOffset) * yNoteWidth) + yBufferSizeBottom) - yNoteWidth / 2.0));
	}

	//if we are on rendering frame n, then we have shifted right by a total of (n-1)*midiUnitsPerFrame*midi2pix pixels
	//so this frame we need to black out pixels (n-1)*midiUnitsPerFrame*tScale to (n*midiUnitsPerFrame*midi2pix)-1 pixels
	//because we are doing this in a scanning way through our array, this becomes (n-1)*midiUnitsPerFrame*midi2pix % (nPixX) to (n*midiUnitsPerFrame*midi2pix)-1 % (nPixX)
	//must take wrap-around into account here!
	//blackout a set of columns that have gone out the back of the frame, and use them to specify new information at the front of the frame (keep rest unchanged)
	
	if (!renderFullFrame) {//not needed for first frame
		int boundary1 = (int)((n - 1)*midiUnitsPerFrame*midi2pix) % (nPixX);//inclusive
		int boundary2 = (int)((n*midiUnitsPerFrame*midi2pix) - 1) % (nPixX);//inclusive

		unsigned char c = 0;
		if (doAlternateFrameBGColor) c = (n % FPS) * 255 / (FPS-1);
		if (boundary1 <= boundary2) {
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

	if(doVideoBarLines){
		for (size_t t = 0; t < barLines.size(); t++) {//set BarLines
			if (firstSoundTime >= barLines.at(t)) continue;
			int barX = (int)(barLines.at(t)*midi2pix + nPixX / 2.0);
			if (barX >= (int)(n*midiUnitsPerFrame*midi2pix)+nPixX) break;
			if ((barX >= (int)(n*midiUnitsPerFrame*midi2pix) && renderFullFrame) || ((barX>= (int)((n-1)*midiUnitsPerFrame*midi2pix) + nPixX) && !renderFullFrame)) {
				barX = barX % nPixX;
				for (int y = staffLines.at(9); y < staffLines.at(5); y++) {
					setColor(staffColor[0], staffColor[1], staffColor[2], barX, y, imageValues);
				}
				for (int y = staffLines.at(4); y < staffLines.at(0); y++) {
					setColor(staffColor[0], staffColor[1], staffColor[2], barX, y, imageValues);
				}
			}
		}
	}

	//set the staff lines
	if (doVideoStaffLines) {
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
	}
	//std::cout << "Initialization Done!" << std::endl;

	if(!doMultiThreading)  std::cout << "Starting to put tracks! into frame..." << std::endl;
	for (unsigned int i = 0; i < trackList.size(); i++) {
	//for (unsigned int i = 2; i < 3; i++) {//for debug
		int nNotes = trackList.at(i).GetNumberOfNotes();
		if( nNotes > 0 && !doMultiThreading) std::cout << "     Making image for track " << i << " which has " << nNotes << " remaining notes." << std::endl;
		int deletedNotes = 0;
		for (int j = 0; j < nNotes - deletedNotes; j++) {
			//for (int j = 0; j < 10; j++) {//for debug
			//if (j % 100 == 0) std::cout << " On note " << j+1 << " out of " << nNotes << std::endl;

			//define length and width of a rectangle to be the note
			int xStart = (int)(trackList.at(i).GetNote(j).beginning*midi2pix + nPixX / 2.0);//in absolute pixels
			int xEnd = (int)(trackList.at(i).GetNote(j).end*midi2pix + nPixX / 2.0);//in absolute pixels
			xEnd = xEnd - (((xEnd - xStart) > 2) ? 1 : 0); //subtract 1 pixel to put some space if there are repeated notes, but don't delete 1 pixel notes
			xEnd = xEnd - (((xEnd - xStart) > 5) ? 1 : 0); //subtract 1 pixel to put some space if there are repeated notes, but don't delete 1 pixel notes
			xEnd = xEnd - (((xEnd - xStart) > 10) ? 1 : 0); //subtract 1 extra pixel to put some space for longer notes

			bool isStartNotInNewFrame = (xStart > ((int)((n*midiUnitsPerFrame*midi2pix) - 1) + nPixX));//continue if note is not in new part of frame yet
			bool isEndNotInNewFrame = (xEnd < ((int)((n - 1)*midiUnitsPerFrame*midi2pix) + nPixX));

			bool doHighlightNote = (xStart <= nPixX / 2 + n*midiUnitsPerFrame*midi2pix) && (xEnd > nPixX / 2 + n*midiUnitsPerFrame*midi2pix);//highlight if spans the central column of pixels
			bool doHighlightNote_lastFrame = (xStart <= nPixX / 2 + (n - 1)*midiUnitsPerFrame*midi2pix) && (xEnd > nPixX / 2 + (n - 1)*midiUnitsPerFrame*midi2pix);
			bool undoHighlightNote = (xStart <= nPixX / 2 + (n - 1)*midiUnitsPerFrame*midi2pix) && (xEnd > nPixX / 2 + (n - 1)*midiUnitsPerFrame*midi2pix) && !doHighlightNote;//check if highlighted last frame

			if (xEnd < ((int)(n*midiUnitsPerFrame*midi2pix))) {//assuming time-ordered notes, delete notes from memory that have already been fully used; modify indices in order to take array size change into account
				trackList.at(i).DeleteNote(j);
				deletedNotes++;
				j--;
				continue;
			}

			if (isStartNotInNewFrame) break;//assuming time-ordered notes, stop processing this track after get to notes in future
			if (!renderFullFrame && !doHighlightNote && !undoHighlightNote && isEndNotInNewFrame) continue; //highlight control
			//if (!renderFullFrame && doHighlightNote && doHighlightNote_lastFrame) continue;//don't spend time re-highlighting

			//prevent overflow if the note starts in the new frame but isn't over before end of new frame
			if (xEnd > ((int)((n*midiUnitsPerFrame*midi2pix) - 1) + nPixX)) xEnd = ((int)((n*midiUnitsPerFrame*midi2pix) - 1) + nPixX);
			//prevent overflow other way if the note starts in the new frame but isn't over before end of new frame
			if (xStart < (int)(n*midiUnitsPerFrame*midi2pix)) xStart = (int)(n*midiUnitsPerFrame*midi2pix);

			int xStartInFrame = xStart % (nPixX);
			int xEndInFrame = xEnd % (nPixX);

			//y values no different than static image case
			int yEnd = nPixY - ((int)((trackList.at(i).GetNote(j).pitch - pitchOffset)*yNoteWidth) + yBufferSizeBottom);
			int yStart = yEnd - (int)yNoteWidth;

			//colors and highlights
			unsigned char colorIndx = (trackList.at(i).GetNote(j).pitch + colorArrayOffset) % 12;
			std::vector< unsigned char > highLightColor(3, 0);
			highLightColor.at(0) = colorArrayLight[0][colorIndx];
			highLightColor.at(1) = colorArrayLight[1][colorIndx];
			highLightColor.at(2) = colorArrayLight[2][colorIndx];

			//scale for velocities
			double volScale = 0;// units of yNoteWidth, level off under 32 or over 112 
			if(doVideoVolumeScaling) volScale = std::min(volScaleMax*volMaxModifier,std::max(-2*volScaleMax+4*volScaleMax*trackList.at(i).GetNote(j).velocity/128.0,-volScaleMax));

			//put new notes into frame
			if (xStartInFrame <= xEndInFrame) {
				for (int x = xStartInFrame; x <= xEndInFrame; x++) {
					for (int y = yStart - (int)(volScale*yNoteWidth); y < yEnd + (int)(volScale*yNoteWidth); y++) {
						if ((y < yStart || y >= yEnd) && isAnotherNote(x, y, imageValues, colorIndx) && !isBackground(x, y, imageValues)) continue;//avoid color collisions when loud
						if(!doHighlightNote) setColor(colorArray[0][colorIndx], colorArray[1][colorIndx], colorArray[2][colorIndx], x, y, imageValues);
						else {
							if (doHighlightDecay) highLightColor = decayRGBValue(colorArray[0][colorIndx], colorArray[1][colorIndx], colorArray[2][colorIndx], (nPixX / 2 + n*midiUnitsPerFrame*midi2pix - xStart) / (double)(xEnd - xStart), doHighlightAttenuationInY?1-fabs(1-(y-(yStart - (int)(volScale*yNoteWidth)))/(double)((1+volScale)*yNoteWidth/2.0)):1, 1, 1);
							setColor(highLightColor.at(0), highLightColor.at(1), highLightColor.at(2), x, y, imageValues);
						}
					}
				}
			}
			else {//wrap around case
				for (int x = 0; x <= xEndInFrame; x++) {
					for (int y = yStart-(int)(volScale*yNoteWidth); y < yEnd + (int)(volScale*yNoteWidth); y++) {
						if ((y < yStart || y >= yEnd) && isAnotherNote(x, y, imageValues, colorIndx) && !isBackground(x, y, imageValues)) continue;//avoid color collisions when loud
						if (!doHighlightNote) setColor(colorArray[0][colorIndx], colorArray[1][colorIndx], colorArray[2][colorIndx], x, y, imageValues);
						else {
							if (doHighlightDecay) highLightColor = decayRGBValue(colorArray[0][colorIndx], colorArray[1][colorIndx], colorArray[2][colorIndx], (nPixX / 2 + n*midiUnitsPerFrame*midi2pix - xStart) / (double)(xEnd - xStart), doHighlightAttenuationInY ? 1 - fabs(1 - (y - (yStart - (int)(volScale*yNoteWidth))) / (double)((1 + volScale)*yNoteWidth / 2.0)) : 1, 1, 1);
							setColor(highLightColor.at(0), highLightColor.at(1), highLightColor.at(2), x, y, imageValues);
						}
					}
				}
				for (int x = xStartInFrame; x < nPixX; x++) {
					for (int y = yStart - (int)(volScale*yNoteWidth); y < yEnd + (int)(volScale*yNoteWidth); y++) {
						if ((y < yStart || y >= yEnd) && isAnotherNote(x, y, imageValues, colorIndx) && !isBackground(x,y,imageValues)) continue;//avoid color collisions when loud
						if (!doHighlightNote) setColor(colorArray[0][colorIndx], colorArray[1][colorIndx], colorArray[2][colorIndx], x, y, imageValues);
						else {
							if (doHighlightDecay) highLightColor = decayRGBValue(colorArray[0][colorIndx], colorArray[1][colorIndx], colorArray[2][colorIndx], (nPixX / 2 + n*midiUnitsPerFrame*midi2pix - xStart) / (double)(xEnd - xStart), doHighlightAttenuationInY ? 1 - fabs(1 - (y - (yStart - (int)(volScale*yNoteWidth))) / (double)((1 + volScale)*yNoteWidth / 2.0)) : 1, 1, 1);
							setColor(highLightColor.at(0), highLightColor.at(1), highLightColor.at(2), x, y, imageValues);
						}
					}
				}
			}
		}
	}

	return;
}