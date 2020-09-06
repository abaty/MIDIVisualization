#pragma once
#include "include/trackList2Image.h"

inline void setColor(unsigned char R, unsigned char G, unsigned char B, int x, int y, std::vector< std::vector< std::vector < unsigned char > > >& imageValues) {
	std::vector< unsigned char > RGBColorTemp{ B,G,R };
	imageValues.at(x).at(y) = RGBColorTemp;
}

inline bool isBackground(int x, int y, std::vector< std::vector< std::vector < unsigned char > > >& imageValues, Settings &s) {
	//always returns true for now... if not there is highlighting bug because isAnotherNote is imperfect

	if (imageValues.at(x).at(y).at(0) == 0 && imageValues.at(x).at(y).at(1) == 0 && imageValues.at(x).at(y).at(2) == 0) return true;
	if (imageValues.at(x).at(y).at(0) == s.staffColor[0] && imageValues.at(x).at(y).at(1) == s.staffColor[1] && imageValues.at(x).at(y).at(2) == s.staffColor[2]) return true;
	return true;
}

inline bool isAnotherNote(int x, int y, std::vector< std::vector< std::vector < unsigned char > > >& imageValues, int colorIndx, Settings &s) {
	//old implementation
	//if (imageValues.at(x).at(y).at(0) == colorArray[2][colorIndx] && imageValues.at(x).at(y).at(1) == colorArray[1][colorIndx] && imageValues.at(x).at(y).at(2) == colorArray[0][colorIndx]) return false;
	//if (imageValues.at(x).at(y).at(0) == colorArrayLight[2][colorIndx] && imageValues.at(x).at(y).at(1) == colorArrayLight[1][colorIndx] && imageValues.at(x).at(y).at(2) == colorArrayLight[0][colorIndx]) return false;

	if (fabs(RGBToHSL(imageValues.at(x).at(y).at(2), imageValues.at(x).at(y).at(1), imageValues.at(x).at(y).at(0)).at(0) - s.hueArray[colorIndx]) < 0.01) return false;

	return true;
}


void trackList2Image(std::vector< MidiTrack >& trackList, std::vector< std::vector< std::vector < unsigned char > > >& imageValues, std::vector< int >& barLines, int nPixX_, int nPixY_, Settings &s) {
	//get usable area inside margins
	//std::cout << "Image Mode!" << std::endl;
	const int xRange = nPixX_ - 2 * s.xBufferSize;
	const int yRange = nPixY_ - s.yBufferSizeBottom - s.yBufferSizeTop;

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
	double pitchRange = 2 * fmax(abs(lowestNote - 2 - 60), abs(highestNote + 2 - 60)) + 1;
	double pitchOffset = 60 - fmax(abs(lowestNote - 2 - 60), abs(highestNote + 2 - 60));

	//calculate the width of each note in Y (128.0 notes in MIDI)
	double yNoteWidth = yRange / pitchRange;
	if (s.usedFixedYNoteWidth) {
		yNoteWidth = yRange / 120.0;
		pitchOffset = 0;
	}

	//calculate staff lines
	std::vector< int > staffLines;
	//std::cout << "Calculating Staff Lines" << std::endl;
	for (int n = 0; n < 10; n++) {
		staffLines.push_back((int)(nPixY_ - (((s.stavePitch[n] - pitchOffset) * yNoteWidth) + s.yBufferSizeBottom) - yNoteWidth / 2.0));
	}

	//set the staff lines
	if (s.doImageStaffLines) {
		for (int y = 0; y < nPixY_; y++) {
			if (std::find(staffLines.begin(), staffLines.end(), y) != staffLines.end()) {//check if is a staffline
				//std::cout << "Putting Staff Line..." << std::endl;
				for (int x = 0; x < nPixX_; x++) {
					imageValues.at(x).at(y).at(0) = s.staffColor[0];
					imageValues.at(x).at(y).at(1) = s.staffColor[1];
					imageValues.at(x).at(y).at(2) = s.staffColor[2];
				}
			}
		}
	}

	if (s.doImageBarLines) {
		for (size_t t = 0; t < barLines.size(); t++) {//set BarLines
			if (firstSoundTime >= barLines.at(t)) continue;
			int barX = (int)(barLines.at(t) * tScale + s.xBufferSize);
			for (int y = staffLines.at(9); y < staffLines.at(5); y++) {
				setColor(s.staffColor[0], s.staffColor[1], s.staffColor[2], barX, y, imageValues);
			}
			for (int y = staffLines.at(4); y < staffLines.at(0); y++) {
				setColor(s.staffColor[0], s.staffColor[1], s.staffColor[2], barX, y, imageValues);
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
			int xStart = (int)(trackList.at(i).GetNote(j).beginning * tScale) + s.xBufferSize;
			int xEnd = (int)(trackList.at(i).GetNote(j).end * tScale) + s.xBufferSize;
			xEnd = xEnd - (((xEnd - xStart) > 2) ? 1 : 0); //subtract 1 pixel to put some space if there are repeated notes, but don't delete 1 pixel notes
			int yEnd = nPixY_ - ((int)((trackList.at(i).GetNote(j).pitch - pitchOffset) * yNoteWidth) + s.yBufferSizeBottom);
			int yStart = yEnd - (int)yNoteWidth;
			unsigned char colorIndx = (trackList.at(i).GetNote(j).pitch + s.colorArrayOffset) % 12;

			//scale for velocities
			double volScale = 0;// units of yNoteWidth, level off under 32 or over 96 
			if (s.doImageVolumeScaling) volScale = std::min(s.volScaleMax * s.volMaxModifier, std::max(-2 * s.volScaleMax + 6 * s.volScaleMax * trackList.at(i).GetNote(j).velocity / 128.0, -s.volScaleMax));
			//std::cout << yNoteWidth << " " << volScale << " " << yStart - (int)(volScale*yNoteWidth) << " " << yEnd + (int)volScale*yNoteWidth << std::endl;

			for (int x = xStart; x < xEnd; x++) {
				for (int y = yStart - (int)(volScale * yNoteWidth); y < yEnd + (int)(volScale * yNoteWidth); y++) {
					if ((y < yStart || y >= yEnd) && isAnotherNote(x, y, imageValues, colorIndx,s) && !isBackground(x, y, imageValues,s)) continue;//avoid color collisions when loud
					setColor(s.colorArray[0][colorIndx], s.colorArray[1][colorIndx], s.colorArray[2][colorIndx], x, y, imageValues);
				}
			}
		}
	}

	return;
}

//render full frame should be true the first time this is called, but then can be set to false to speed up stuff
void trackList2Video(std::vector< MidiTrack >& trackList, std::vector< std::vector< std::vector < unsigned char > > >& imageValues, int n, int nFrames, double midiUnitsPerFrame, std::vector< int >& barLines, Settings &s, bool renderFullFrame) {
	//get usable area inside margins
	const int xRange = s.nPixX;
	const int yRange = s.nPixY - s.yBufferSizeBottom - s.yBufferSizeTop;

	//find the total time of the song and the scaling of time to pixels
	int lastSoundTime = 0;
	int firstSoundTime = 10000000000;
	for (unsigned int i = 0; i < trackList.size(); i++) {
		if (lastSoundTime < trackList.at(i).GetEnd()) lastSoundTime = trackList.at(i).GetEnd();
		if (firstSoundTime > trackList.at(i).GetStart()) firstSoundTime = trackList.at(i).GetStart();
	}
	float midi2pix = s.scrollSpeedFactor / s.tempoScale;

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
	if (s.usedFixedYNoteWidth) {
		yNoteWidth = yRange / 120.0;
		pitchOffset = 0;
	}

	//calculate staff lines
	//0 is highest one
	std::vector< int > staffLines;
	for (int n = 0; n < 10; n++) {
		staffLines.push_back((int)(s.nPixY - (((s.stavePitch[n] - pitchOffset) * yNoteWidth) + s.yBufferSizeBottom) - yNoteWidth / 2.0));
	}


	//if we are on rendering frame n, then we have shifted right by a total of (n-1)*midiUnitsPerFrame*midi2pix pixels
	//so this frame we need to black out pixels (n-1)*midiUnitsPerFrame*tScale to (n*midiUnitsPerFrame*midi2pix)-1 pixels
	//because we are doing this in a scanning way through our array, this becomes (n-1)*midiUnitsPerFrame*midi2pix % (nPixX) to (n*midiUnitsPerFrame*midi2pix)-1 % (nPixX)
	//must take wrap-around into account here!
	//blackout a set of columns that have gone out the back of the frame, and use them to specify new information at the front of the frame (keep rest unchanged)


	if (!renderFullFrame) {//not needed for first frame
		int boundary1 = (int)((n - 1) * midiUnitsPerFrame * midi2pix) % (s.nPixX);//inclusive
		int boundary2 = (int)((n * midiUnitsPerFrame * midi2pix) - 1) % (s.nPixX);//inclusive

		unsigned char c = 0;
		if (s.doAlternateFrameBGColor) c = (n % s.FPS) * 255 / (s.FPS - 1);
		if (boundary1 <= boundary2) {
			for (int x = boundary1; x <= boundary2; x++) {
				for (int y = 0; y < s.nPixY; y++) {
					imageValues.at(x).at(y).at(0) = c;
					imageValues.at(x).at(y).at(1) = c;
					imageValues.at(x).at(y).at(2) = c;
				}
			}
		}
		else {//wrap around case
			for (int x = 0; x <= boundary2; x++) {
				for (int y = 0; y < s.nPixY; y++) {
					imageValues.at(x).at(y).at(0) = c;
					imageValues.at(x).at(y).at(1) = c;
					imageValues.at(x).at(y).at(2) = c;
				}
			}
			for (int x = boundary1; x < s.nPixX; x++) {
				for (int y = 0; y < s.nPixY; y++) {
					imageValues.at(x).at(y).at(0) = c;
					imageValues.at(x).at(y).at(1) = c;
					imageValues.at(x).at(y).at(2) = c;
				}
			}
		}
	}

	if (s.doVideoBarLines) {
		for (size_t t = 0; t < barLines.size(); t++) {//set BarLines
			if (firstSoundTime >= barLines.at(t)) continue;
			int barX = (int)(barLines.at(t) * midi2pix + s.nPixX / 2.0);
			if (barX >= (int)(n * midiUnitsPerFrame * midi2pix) + s.nPixX) break;
			if ((barX >= (int)(n * midiUnitsPerFrame * midi2pix) && renderFullFrame) || ((barX >= (int)((n - 1) * midiUnitsPerFrame * midi2pix) + s.nPixX) && !renderFullFrame)) {
				barX = barX % s.nPixX;
				for (int y = staffLines.at(9); y < staffLines.at(5); y++) {
					if (y > -1 && y < s.vidnPixY - (s.doMiniScore ? s.miniScoreYPix : 0)) setColor(s.staffColor[0], s.staffColor[1], s.staffColor[2], barX, y, imageValues);
				}
				for (int y = staffLines.at(4); y < staffLines.at(0); y++) {
					if (y > -1 && y < s.vidnPixY - (s.doMiniScore ? s.miniScoreYPix : 0)) setColor(s.staffColor[0], s.staffColor[1], s.staffColor[2], barX, y, imageValues);
				}
			}
		}
	}

	//set the staff lines
	if (s.doVideoStaffLines) {
		for (int y = 0; y < s.nPixY; y++) {
			if (std::find(staffLines.begin(), staffLines.end(), y) != staffLines.end()) {//check if is a staffline
				//std::cout << "Putting Staff Line..." << std::endl;
				for (int x = 0; x < s.nPixX; x++) {
					if (imageValues.at(x).at(y).at(0) == 0 && imageValues.at(x).at(y).at(1) == 0 && imageValues.at(x).at(y).at(2) == 0) {//only overwrite black space
						imageValues.at(x).at(y).at(0) = s.staffColor[0];
						imageValues.at(x).at(y).at(1) = s.staffColor[1];
						imageValues.at(x).at(y).at(2) = s.staffColor[2];
					}
				}
			}
		}
	}
	//std::cout << "Initialization Done!" << std::endl;

	if (!s.doMultiThreading)  std::cout << "Starting to put tracks! into frame..." << std::endl;
	for (unsigned int i = 0; i < trackList.size(); i++) {
		//for (unsigned int i = 2; i < 3; i++) {//for debug
		int nNotes = trackList.at(i).GetNumberOfNotes();
		if (nNotes > 0 && !s.doMultiThreading) std::cout << "     Making image for track " << i << " which has " << nNotes << " remaining notes." << std::endl;
		int deletedNotes = 0;
		for (int j = 0; j < nNotes - deletedNotes; j++) {
			//for (int j = 0; j < 10; j++) {//for debug
			//if (j % 100 == 0) std::cout << " On note " << j+1 << " out of " << nNotes << std::endl;

			//define length and width of a rectangle to be the note
			int xStart = (int)(trackList.at(i).GetNote(j).beginning * midi2pix + s.nPixX / 2.0);//in absolute pixels
			int xEnd = (int)(trackList.at(i).GetNote(j).end * midi2pix + s.nPixX / 2.0);//in absolute pixels
			xEnd = xEnd - (((xEnd - xStart) > 2) ? 1 : 0); //subtract 1 pixel to put some space if there are repeated notes, but don't delete 1 pixel notes
			//xEnd = xEnd - (((xEnd - xStart) > 5) ? 1 : 0); //subtract 1 pixel to put some space if there are repeated notes, but don't delete 1 pixel notes
			//xEnd = xEnd - (((xEnd - xStart) > 10) ? 1 : 0); //subtract 1 extra pixel to put some space for longer notes

			bool isStartNotInNewFrame = (xStart > ((int)((n * midiUnitsPerFrame * midi2pix) - 1) + s.nPixX));//continue if note is not in new part of frame yet
			bool isEndNotInNewFrame = (xEnd < ((int)((n - 1) * midiUnitsPerFrame * midi2pix) + s.nPixX));

			bool doHighlightNote = (xStart <= s.nPixX / 2 + n * midiUnitsPerFrame * midi2pix) && (xEnd > s.nPixX / 2 + n * midiUnitsPerFrame * midi2pix);//highlight if spans the central column of pixels
			bool doHighlightNote_lastFrame = (xStart <= s.nPixX / 2 + (n - 1) * midiUnitsPerFrame * midi2pix) && (xEnd > s.nPixX / 2 + (n - 1) * midiUnitsPerFrame * midi2pix);
			bool undoHighlightNote = (doHighlightNote_lastFrame) && !doHighlightNote;//check if highlighted last frame

			//delete here, but if outlining notes, don't delete until later
			if (!s.doOutlineNotes) {
				if (xEnd < ((int)(n * midiUnitsPerFrame * midi2pix))) {//assuming time-ordered notes, delete notes from memory that have already been fully used; modify indices in order to take array size change into account
					trackList.at(i).DeleteNote(j);
					deletedNotes++;
					j--;
					continue;
				}
			}
			else if(xEnd < (int)(n * midiUnitsPerFrame * midi2pix)) continue;

			if (isStartNotInNewFrame) break;//assuming time-ordered notes, stop processing this track after get to notes in future
			//if (!renderFullFrame && !doHighlightNote && !undoHighlightNote && isEndNotInNewFrame && trackList.at(i).GetNote(j).isOverlap == 0) continue; //highlight control
			//if (!renderFullFrame && doHighlightNote && doHighlightNote_lastFrame) continue;//don't spend time re-highlighting

			//prevent overflow if the note starts in the new frame but isn't over before end of new frame
			if (xEnd > ((int)((n * midiUnitsPerFrame * midi2pix) - 1) + s.nPixX)) xEnd = ((int)((n * midiUnitsPerFrame * midi2pix) - 1) + s.nPixX);
			//prevent overflow other way if the note starts in the new frame but isn't over before end of new frame
			if (xStart < (int)(n * midiUnitsPerFrame * midi2pix)) xStart = (int)(n * midiUnitsPerFrame * midi2pix);

			int xStartInFrame = xStart % (s.nPixX);
			int xEndInFrame = xEnd % (s.nPixX);

			//y values no different than static image case
			int yEnd = s.nPixY - ((int)((trackList.at(i).GetNote(j).pitch - pitchOffset) * yNoteWidth) + s.yBufferSizeBottom);
			int yStart = yEnd - (int)yNoteWidth;

			//colors and highlights
			unsigned char colorIndx = (trackList.at(i).GetNote(j).pitch + s.colorArrayOffset) % 12;
			std::vector< unsigned char > highLightColor(3, 0);
			highLightColor.at(0) = s.colorArrayLight[0][colorIndx];
			highLightColor.at(1) = s.colorArrayLight[1][colorIndx];
			highLightColor.at(2) = s.colorArrayLight[2][colorIndx];

			//scale for velocities
			double volScale = 0;// units of yNoteWidth, level off under 32 or over 112 
			if (s.doVideoVolumeScaling) volScale = std::min(s.volScaleMax * s.volMaxModifier, std::max(-2 * s.volScaleMax + 6 * s.volScaleMax * trackList.at(i).GetNote(j).velocity / 128.0, -s.volScaleMax));
			int volOffset = volScale * yNoteWidth;

			//put new notes into frame
			if (xStartInFrame <= xEndInFrame) {
				for (int x = xStartInFrame; x <= xEndInFrame; x++) {
					for (int y = yStart - volOffset; y < yEnd + volOffset; y++) {
						if ((y < yStart || y >= yEnd) && isAnotherNote(x, y, imageValues, colorIndx,s) && !isBackground(x, y, imageValues,s)) continue;//avoid color collisions when loud
						if (!doHighlightNote) {
							//overlaps if one note is softer (put black border around top and bottom)
							/*if (trackList.at(i).GetNote(j).isSofter && (y == (yStart - (int)(volScale*yNoteWidth)))) {
								setColor(0, 0, 0, x, y - 1, imageValues);
							}
							if (trackList.at(i).GetNote(j).isSofter && (y == (yEnd + (int)(volScale*yNoteWidth) - 1))) {
								setColor(0, 0, 0, x, y + 1, imageValues);
							}*/
							setColor(s.colorArray[0][colorIndx], s.colorArray[1][colorIndx], s.colorArray[2][colorIndx], x, y, imageValues);
							//if(x == xStartInFrame && ( trackList.at(i).GetNote(j).isOverlap==1 || trackList.at(i).GetNote(j).isOverlap == 3) && (abs(y- (yStart - (int)(volScale*yNoteWidth)))<5 || abs(y - (yEnd + (int)(volScale*yNoteWidth)))<5)) setColor(0, 0, 0, x, y, imageValues);
							//if(x == xEndInFrame && trackList.at(i).GetNote(j).isOverlap >= 2 && (abs(y - (yStart - (int)(volScale*yNoteWidth)))<5 || abs(y - (yEnd + (int)(volScale*yNoteWidth)))<5)) setColor(0, 0, 0, x, y, imageValues);
						}
						else {
							if (s.doHighlightDecay) highLightColor = decayRGBValue(s.colorArray[0][colorIndx], s.colorArray[1][colorIndx], s.colorArray[2][colorIndx], (s.nPixX / 2 + n * midiUnitsPerFrame * midi2pix - xStart), (double)(xEnd - xStart), s.doHighlightAttenuationInY ? 1 - fabs(1 - (y - (yStart - volOffset)) / (double)((1 + volScale) * yNoteWidth / 2.0)) : 1, 1, 1);
							//overlaps if one note is softer (put black border around top and bottom)
							/*if (trackList.at(i).GetNote(j).isSofter && (y == (yStart - (int)(volScale*yNoteWidth)))) {
								setColor(0, 0, 0, x, y - 1, imageValues);
							}
							if (trackList.at(i).GetNote(j).isSofter && (y == (yEnd + (int)(volScale*yNoteWidth) - 1))) {
								setColor(0, 0, 0, x, y + 1, imageValues);
							}*/
							setColor(highLightColor.at(0), highLightColor.at(1), highLightColor.at(2), x, y, imageValues);
							//if (x == xStartInFrame && (trackList.at(i).GetNote(j).isOverlap == 1 || trackList.at(i).GetNote(j).isOverlap == 3) && (abs(y - (yStart - (int)(volScale*yNoteWidth)))<5 || abs(y - (yEnd + (int)(volScale*yNoteWidth)))<5)) setColor(0, 0, 0, x, y, imageValues);
							//if (x == xEndInFrame && trackList.at(i).GetNote(j).isOverlap >= 2 && (abs(y - (yStart - (int)(volScale*yNoteWidth)))<5 || abs(y - (yEnd + (int)(volScale*yNoteWidth)))<5)) setColor(0, 0, 0, x, y, imageValues);
						}
					}
				}
			}
			else {//wrap around case
				for (int x = 0; x <= xEndInFrame; x++) {
					for (int y = yStart - volOffset; y < yEnd + volOffset; y++) {
						if ((y < yStart || y >= yEnd) && isAnotherNote(x, y, imageValues, colorIndx,s) && !isBackground(x, y, imageValues,s)) continue;//avoid color collisions when loud
						if (!doHighlightNote) {
							//overlaps if one note is softer (put black border around top and bottom)
							/*if (trackList.at(i).GetNote(j).isSofter && (y == (yStart - (int)(volScale*yNoteWidth)))) {
								setColor(0, 0, 0, x, y - 1, imageValues);
							}
							if (trackList.at(i).GetNote(j).isSofter && (y == (yEnd + (int)(volScale*yNoteWidth) - 1))) {
								setColor(0, 0, 0, x, y + 1, imageValues);
							}*/
							setColor(s.colorArray[0][colorIndx], s.colorArray[1][colorIndx], s.colorArray[2][colorIndx], x, y, imageValues);
							//if (x == xEndInFrame && trackList.at(i).GetNote(j).isOverlap >= 2 && (abs(y - (yStart - (int)(volScale*yNoteWidth)))<5 || abs(y - (yEnd + (int)(volScale*yNoteWidth)))<5)) setColor(0, 0, 0, x, y, imageValues);
						}
						else {
							//overlaps if one note is softer (put black border around top and bottom)
							/*if (trackList.at(i).GetNote(j).isSofter && (y == (yStart - (int)(volScale*yNoteWidth)))) {
								setColor(0, 0, 0, x, y - 1, imageValues);
							}
							if (trackList.at(i).GetNote(j).isSofter && (y == (yEnd + (int)(volScale*yNoteWidth) - 1))) {
								setColor(0, 0, 0, x, y + 1, imageValues);
							}*/
							if (s.doHighlightDecay) highLightColor = decayRGBValue(s.colorArray[0][colorIndx], s.colorArray[1][colorIndx], s.colorArray[2][colorIndx], (s.nPixX / 2 + n * midiUnitsPerFrame * midi2pix - xStart), (double)(xEnd - xStart), s.doHighlightAttenuationInY ? 1 - fabs(1 - (y - (yStart - volOffset)) / (double)((1 + volScale) * yNoteWidth / 2.0)) : 1, 1, 1);
							setColor(highLightColor.at(0), highLightColor.at(1), highLightColor.at(2), x, y, imageValues);
							//if (x == xEndInFrame && trackList.at(i).GetNote(j).isOverlap >= 2 && (abs(y - (yStart - (int)(volScale*yNoteWidth)))<5 || abs(y - (yEnd + (int)(volScale*yNoteWidth))<5))) setColor(0, 0, 0, x, y, imageValues);
						}
					}
				}
				for (int x = xStartInFrame; x < s.nPixX; x++) {
					for (int y = yStart - volOffset; y < yEnd + volOffset; y++) {
						if ((y < yStart || y >= yEnd) && isAnotherNote(x, y, imageValues, colorIndx,s) && !isBackground(x, y, imageValues,s)) continue;//avoid color collisions when loud
						if (!doHighlightNote) {
							//overlaps if one note is softer (put black border around top and bottom)
							/*if (trackList.at(i).GetNote(j).isSofter && (y == (yStart - (int)(volScale*yNoteWidth)))) {
								setColor(0, 0, 0, x, y - 1, imageValues);
							}
							if (trackList.at(i).GetNote(j).isSofter && (y == (yEnd + (int)(volScale*yNoteWidth) - 1))) {
								setColor(0, 0, 0, x, y + 1, imageValues);
							}*/
							setColor(s.colorArray[0][colorIndx], s.colorArray[1][colorIndx], s.colorArray[2][colorIndx], x, y, imageValues);
							//if (x == xStartInFrame && (trackList.at(i).GetNote(j).isOverlap == 1 || trackList.at(i).GetNote(j).isOverlap == 3) && (abs(y - (yStart - (int)(volScale*yNoteWidth)))<5 || abs(y - (yEnd + (int)(volScale*yNoteWidth)))<5)) setColor(0, 0, 0, x, y, imageValues);
						}
						else {
							//overlaps if one note is softer (put black border around top and bottom)
							/*if (trackList.at(i).GetNote(j).isSofter && (y == (yStart - (int)(volScale*yNoteWidth)))) {
								setColor(0, 0, 0, x, y - 1, imageValues);
							}
							if (trackList.at(i).GetNote(j).isSofter && (y == (yEnd + (int)(volScale*yNoteWidth) - 1))) {
								setColor(0, 0, 0, x, y + 1, imageValues);
							}*/
							if (s.doHighlightDecay) highLightColor = decayRGBValue(s.colorArray[0][colorIndx], s.colorArray[1][colorIndx], s.colorArray[2][colorIndx], (s.nPixX / 2 + n * midiUnitsPerFrame * midi2pix - xStart), (double)(xEnd - xStart), s.doHighlightAttenuationInY ? 1 - fabs(1 - (y - (yStart - volOffset)) / (double)((1 + volScale) * yNoteWidth / 2.0)) : 1, 1, 1);
							setColor(highLightColor.at(0), highLightColor.at(1), highLightColor.at(2), x, y, imageValues);
							//if (x == xStartInFrame && (trackList.at(i).GetNote(j).isOverlap == 1 || trackList.at(i).GetNote(j).isOverlap == 3) && (abs(y - (yStart - (int)(volScale*yNoteWidth)))<5 || abs(y - (yEnd + (int)(volScale*yNoteWidth))<5))) setColor(0, 0, 0, x, y, imageValues);
						}
					}
				}
			}
		}
	}

	//outlining
	//making colors darker
	if (s.doOutlineNotes) {
		int outlineColorArray[3][12];
		for (int i = 0; i < 12; i++) {
			std::vector< double > HSLvalues = RGBToHSL(s.colorArray[0][i], s.colorArray[1][i], s.colorArray[2][i]);
			std::vector< unsigned char > outlineRGBvalues = HSLToRGB(HSLvalues.at(0), HSLvalues.at(1), HSLvalues.at(2) / 2);
			outlineColorArray[0][i] = outlineRGBvalues.at(0);
			outlineColorArray[1][i] = outlineRGBvalues.at(1);
			outlineColorArray[2][i] = outlineRGBvalues.at(2);
		}

		for (unsigned int i = 0; i < trackList.size(); i++) {
			int nNotes = trackList.at(i).GetNumberOfNotes();
			if (nNotes > 0 && !s.doMultiThreading) std::cout << "     Making image for track " << i << " which has " << nNotes << " remaining notes." << std::endl;
			int deletedNotes = 0;
			for (int j = 0; j < nNotes - deletedNotes; j++) {
				//for (int j = 0; j < 10; j++) {//for debug
				//if (j % 100 == 0) std::cout << " On note " << j+1 << " out of " << nNotes << std::endl;

				//define length and width of a rectangle to be the note
				int xStart = (int)(trackList.at(i).GetNote(j).beginning * midi2pix + s.nPixX / 2.0);//in absolute pixels
				int xEnd = (int)(trackList.at(i).GetNote(j).end * midi2pix + s.nPixX / 2.0);//in absolute pixels
				xEnd = xEnd - (((xEnd - xStart) > 2) ? 1 : 0); //subtract 1 pixel to put some space if there are repeated notes, but don't delete 1 pixel notes
				//xEnd = xEnd - (((xEnd - xStart) > 5) ? 1 : 0); //subtract 1 pixel to put some space if there are repeated notes, but don't delete 1 pixel notes
				//xEnd = xEnd - (((xEnd - xStart) > 10) ? 1 : 0); //subtract 1 extra pixel to put some space for longer notes

				bool isStartNotInNewFrame = (xStart > ((int)((n * midiUnitsPerFrame * midi2pix) - 1) + s.nPixX));//continue if note is not in new part of frame yet
				bool isEndNotInNewFrame = (xEnd < ((int)((n - 1) * midiUnitsPerFrame * midi2pix) + s.nPixX));

				bool doHighlightNote = (xStart <= s.nPixX / 2 + n * midiUnitsPerFrame * midi2pix) && (xEnd > s.nPixX / 2 + n * midiUnitsPerFrame * midi2pix);//highlight if spans the central column of pixels
				bool doHighlightNote_lastFrame = (xStart <= s.nPixX / 2 + (n - 1) * midiUnitsPerFrame * midi2pix) && (xEnd > s.nPixX / 2 + (n - 1) * midiUnitsPerFrame * midi2pix);
				bool undoHighlightNote = doHighlightNote_lastFrame && !doHighlightNote;//check if highlighted last frame

				if (xEnd < ((int)(n * midiUnitsPerFrame * midi2pix))) {//assuming time-ordered notes, delete notes from memory that have already been fully used; modify indices in order to take array size change into account
					trackList.at(i).DeleteNote(j);
					deletedNotes++;
					j--;
					continue;
				}

				if (isStartNotInNewFrame) break;//assuming time-ordered notes, stop processing this track after get to notes in future
				//if (!renderFullFrame && !doHighlightNote && !undoHighlightNote && isEndNotInNewFrame) continue; //highlight control
				//if (!renderFullFrame && doHighlightNote && doHighlightNote_lastFrame) continue;//don't spend time re-highlighting

				//prevent overflow if the note starts in the new frame but isn't over before end of new frame
				if (xEnd > ((int)((n * midiUnitsPerFrame * midi2pix) - 1) + s.nPixX)) xEnd = ((int)((n * midiUnitsPerFrame * midi2pix) - 1) + s.nPixX);
				//prevent overflow other way if the note starts in the new frame but isn't over before end of new frame
				if (xStart < (int)(n * midiUnitsPerFrame * midi2pix)) xStart = (int)(n * midiUnitsPerFrame * midi2pix);

				int xStartInFrame = xStart % (s.nPixX);
				int xEndInFrame = xEnd % (s.nPixX);

				//y values no different than static image case
				int yEnd = s.nPixY - ((int)((trackList.at(i).GetNote(j).pitch - pitchOffset) * yNoteWidth) + s.yBufferSizeBottom);
				int yStart = yEnd - (int)yNoteWidth;

				//colors and highlights
				unsigned char colorIndx = (trackList.at(i).GetNote(j).pitch + s.colorArrayOffset) % 12;

				//scale for velocities
				double volScale = 0;// units of yNoteWidth, level off under 32 or over 112 
				if (s.doVideoVolumeScaling) volScale = std::min(s.volScaleMax * s.volMaxModifier, std::max(-2 * s.volScaleMax + 6 * s.volScaleMax * trackList.at(i).GetNote(j).velocity / 128.0, -s.volScaleMax));
				int volOffset = volScale * yNoteWidth;

				if (xStartInFrame <= xEndInFrame) {
					for (int x = xStartInFrame; x <= xEndInFrame; x++) {
						if (x == xStartInFrame || x == xEndInFrame) {
							for (int y = yStart - volOffset; y < yEnd + volOffset; y++) setColor(outlineColorArray[0][colorIndx], outlineColorArray[1][colorIndx], outlineColorArray[2][colorIndx], x, y, imageValues);
						}
						setColor(outlineColorArray[0][colorIndx], outlineColorArray[1][colorIndx], outlineColorArray[2][colorIndx], x, yStart - volOffset, imageValues);
						setColor(outlineColorArray[0][colorIndx], outlineColorArray[1][colorIndx], outlineColorArray[2][colorIndx], x, yEnd + volOffset - 1, imageValues);
					}
				}
				else {//wrap around case
					for (int x = 0; x <= xEndInFrame; x++) {
						if (x == xEndInFrame) {
							for (int y = yStart - volOffset; y < yEnd + volOffset; y++) setColor(outlineColorArray[0][colorIndx], outlineColorArray[1][colorIndx], outlineColorArray[2][colorIndx], x, y, imageValues);
						}
						setColor(outlineColorArray[0][colorIndx], outlineColorArray[1][colorIndx], outlineColorArray[2][colorIndx], x, yStart - volOffset, imageValues);
						setColor(outlineColorArray[0][colorIndx], outlineColorArray[1][colorIndx], outlineColorArray[2][colorIndx], x, yEnd + volOffset - 1, imageValues);
					}
					for (int x = xStartInFrame; x < s.nPixX; x++) {
						if (x == xStartInFrame) {
							for (int y = yStart - volOffset; y < yEnd + volOffset; y++) setColor(outlineColorArray[0][colorIndx], outlineColorArray[1][colorIndx], outlineColorArray[2][colorIndx], x, y, imageValues);
						}
						setColor(outlineColorArray[0][colorIndx], outlineColorArray[1][colorIndx], outlineColorArray[2][colorIndx], x, yStart - volOffset, imageValues);
						setColor(outlineColorArray[0][colorIndx], outlineColorArray[1][colorIndx], outlineColorArray[2][colorIndx], x, yEnd + volOffset - 1, imageValues);
					}
				}
			}
		}
	}
	return;
}