// Visualization.cpp : Defines the entry point for the console application.
//

#include "include/stdafx.h"
#include "midi2ascii.cpp"//exclude from compilation unit
#include "include/Input.h"
#include "include/Settings.h"
#include "include/midiTrack.h"
#include "include/trackList2Image.h"
#include "include/colorTools.h"
#include "include/CImg.h"
#include <iostream>  
#include <limits>
#include <vector>
#include <string>
#include "LodePng.cpp" //exclude from compilation unit
#include "decodeBMP.cpp" //exclude from compilation unit
#include <ctime>
#include <thread>
#include <mutex>
#include <iomanip>
#include <chrono>

//multithreading safety mutexs
std::mutex completed, execTime;

using namespace cimg_library;

void renderThread(int processID, std::vector< MidiTrack >& trackList, int nFrames, int midiUnitsPerFrame, std::vector< int >& barLines, std::vector< std::vector< std::vector < unsigned char > > >& miniScoreImageValues, Settings & s, int startingFrame = 0, int endingFrame = 1) {
	std::clock_t start = std::clock();

	std::vector< std::vector< std::vector < unsigned char > > > imageValues(s.nPixX, std::vector< std::vector < unsigned char > >(s.nPixY, std::vector < unsigned char >(3, 0)));
	for (int n = startingFrame; n < endingFrame; n++) {
		//std::cout << "Thread " << processID << " rendering frame " << n+1 << " out of " << endingFrame  << ".   (" << (double)(n - startingFrame) / ((std::clock() - start) / (double)CLOCKS_PER_SEC) << " frames/s)" << std::endl;

		if (n == startingFrame) trackList2Video(trackList, imageValues, n, nFrames, midiUnitsPerFrame, barLines, s, true);
		else     trackList2Video(trackList, imageValues, n, nFrames, midiUnitsPerFrame, barLines, s, false);

		//std::clock_t start2 = std::clock();
		//handing data over to Image library
		CImg<unsigned char> img(s.nPixX, (s.doMiniScore ? (s.nPixY + s.miniScoreYPix) : s.nPixY), 1, 3);
		for (int y = 0; y < (s.doMiniScore ? (s.nPixY + s.miniScoreYPix) : s.nPixY); y++) {
			if (s.doMiniScore && y >= s.nPixY) {//for miniScore
				if (y == s.nPixY) {
					for (int x = 0; x < s.nPixX; x++) {
						img(x, y, 0) = s.miniScoreHighlight[0];
						img(x, y, 1) = s.miniScoreHighlight[1];
						img(x, y, 2) = s.miniScoreHighlight[2];
					}
				}
				else {
					int firstX = -9999;
					double fracDone = n / (double)nFrames;//highlighting position in mini score
					double fracWindow = (double)(s.nPixX - 2 * s.xBufferSize) / (midiUnitsPerFrame * s.scrollSpeedFactor / s.tempoScale) / 2.0 / (double)nFrames;
					for (int x = 0; x < s.nPixX; x++) {
						unsigned char red = 0, green = 0, blue = 0;
						img(x, y, 0) = miniScoreImageValues.at(x).at(y - 1 - s.nPixY).at(2);//R
						img(x, y, 1) = miniScoreImageValues.at(x).at(y - 1 - s.nPixY).at(1);//G
						img(x, y, 2) = miniScoreImageValues.at(x).at(y - 1 - s.nPixY).at(0);//B
						if (firstX != -9999 && ((firstX != 0 && x >= firstX + fracWindow * 2 * (s.nPixX - 2 * s.xBufferSize)) || (firstX == 0 && x >= s.xBufferSize + fracWindow * (s.nPixX - 2 * s.xBufferSize)))) continue;
						if ((x - s.xBufferSize) / (double)(s.nPixX - 2 * s.xBufferSize) >= fracDone - fracWindow)
						{
							if (firstX == -9999) firstX = x;
							if (miniScoreImageValues.at(x).at(y - 1 - s.nPixY).at(2) == 0 && miniScoreImageValues.at(x).at(y - 1 - s.nPixY).at(1) == 0 && miniScoreImageValues.at(x).at(y - 1 - s.nPixY).at(0) == 0) {
								img(x, y, 0) = s.miniScoreHighlight[0];
								img(x, y, 1) = s.miniScoreHighlight[1];
								img(x, y, 2) = s.miniScoreHighlight[2];
							}
						}
					}
				}
			}
			else {
				for (int x = 0; x < s.nPixX; x++) {
					unsigned char red = 0, green = 0, blue = 0;
					img(x, y, 0) = imageValues.at(((int)(n * midiUnitsPerFrame * s.scrollSpeedFactor / s.tempoScale + x) % s.nPixX)).at(y).at(2);//R
					img(x, y, 1) = imageValues.at(((int)(n * midiUnitsPerFrame * s.scrollSpeedFactor / s.tempoScale + x) % s.nPixX)).at(y).at(1);//G
					img(x, y, 2) = imageValues.at(((int)(n * midiUnitsPerFrame * s.scrollSpeedFactor / s.tempoScale + x) % s.nPixX)).at(y).at(0);//B
				}
			}
		}

		//double runTime2 = (std::clock() - start2) / (double)CLOCKS_PER_SEC;
		//std::cout << "processing img time before save:" << runTime2 << std::endl;

		std::string temporaryFileName = "temporary";
		temporaryFileName.append(std::to_string(processID));
		temporaryFileName.append(".bmp");
		img.save(temporaryFileName.c_str());

		//convert to png
		std::vector<unsigned char> bmp;
		lodepng::load_file(bmp, temporaryFileName.c_str());
		std::vector<unsigned char> image;
		unsigned w, h;
		unsigned error = decodeBMP(image, w, h, bmp);
		std::vector<unsigned char> png;
		error = lodepng::encode(png, image, w, h);
		temporaryFileName = s.inputFile;
		temporaryFileName += std::to_string(100000 + n);//add 100000 just to ensure all have same number of sig figs (poor man's way)
		lodepng::save_file(png, temporaryFileName.append(".png").data());

		//runTime2 = (std::clock() - start2) / (double)CLOCKS_PER_SEC;
		//std::cout << "processing img time after save:" << runTime2 << std::endl;

		completed.lock();
		s.completedFrames[processID]++;
		completed.unlock();
		execTime.lock();
		s.totalExecutionTime[processID] = ((std::clock() - start) / (double)CLOCKS_PER_SEC);
		execTime.unlock();
	}
}

void makeVisual(Settings & s, bool doVideo = false)
{
	//std::cin.get();
	//convert MIDI to text file
	std::string temporaryFileName = s.inputFile;
	std::string temporaryFileName2 = s.inputFile;
	std::cout << "Parsing MIDI FILE: " << s.inputFile << std::endl;
	Midi2Ascii(temporaryFileName.append(".mid"), temporaryFileName2.append(".txt"));
	std::cout << "Finished With MIDI File" << std::endl;


	//read in text file as notes
	std::vector< MidiTrack > trackList;
	std::vector< int > barLines;
	double duration = 0;
	readInputFile(temporaryFileName2, trackList, barLines, duration, s);

	//std::cin.get();

	//some formatting
	if (doVideo) {
		s.nPixX = s.vidnPixX;
		s.nPixY = s.vidnPixY;
		if (s.doMiniScore) {
			s.nPixY = s.nPixY - s.miniScoreYPix;
			s.doImageStaffLines = false;
		}
	}

	int lastTime = 0;
	for (unsigned int i = 0; i < trackList.size(); i++) {
		std::cout << "Track " << i << " has " << trackList.at(i).GetNumberOfNotes() << " notes." << std::endl;
		if (trackList.at(i).GetEnd() > lastTime) lastTime = trackList.at(i).GetEnd();
	}
	//checking for overlapping notes
	std::cout << "Checking for overlapping notes" << std::endl;
	for (unsigned int i = 0; i < trackList.size(); i++) {
		for (int k = 0; k < trackList.at(i).GetNumberOfNotes(); k++) {
			for (unsigned int j = i; j < trackList.size(); j++) {
				for (int k2 = 0; k2 < trackList.at(j).GetNumberOfNotes(); k2++) {
					if (trackList.at(i).GetNote(k).end <= trackList.at(j).GetNote(k2).beginning) continue;
					if (trackList.at(i).GetNote(k).beginning >= trackList.at(j).GetNote(k2).end) continue;
					if (trackList.at(i).GetNote(k).pitch != trackList.at(j).GetNote(k2).pitch) continue;
					if (i == j && k == k2) continue;
					//1 = starts after (left border needed)
					//2 = ends before (right border needed)
					//3 = 1+2
					if (trackList.at(i).GetNote(k).velocity < trackList.at(j).GetNote(k2).velocity) trackList.at(i).setIsSofter(k, 1);
					if (trackList.at(i).GetNote(k).velocity > trackList.at(j).GetNote(k2).velocity) trackList.at(j).setIsSofter(k2, 1);
					if (trackList.at(i).GetNote(k).beginning > trackList.at(j).GetNote(k2).beginning) {
						trackList.at(i).setOverlap(k, 1);
					}
					else if (trackList.at(i).GetNote(k).beginning < trackList.at(j).GetNote(k2).beginning) {
						trackList.at(j).setOverlap(k2, 1);
					}

					if (trackList.at(i).GetNote(k).end > trackList.at(j).GetNote(k2).end) {
						trackList.at(j).setOverlap(k2, 2);
					}
					else if (trackList.at(i).GetNote(k).end < trackList.at(j).GetNote(k2).end) {
						trackList.at(i).setOverlap(k, 2);
					}

					if (trackList.at(i).GetNote(k).beginning > trackList.at(j).GetNote(k2).beginning && trackList.at(i).GetNote(k).end < trackList.at(j).GetNote(k2).end) {
						trackList.at(i).setOverlap(k, 3);
					}

					if (trackList.at(i).GetNote(k).beginning < trackList.at(j).GetNote(k2).beginning && trackList.at(i).GetNote(k).end > trackList.at(j).GetNote(k2).end) {
						trackList.at(j).setOverlap(k2, 3);
					}

				}
			}
		}
	}


	if (s.calculateLengthBySelf && !doVideo) s.nPixX = lastTime / (60 * s.tempoScale);

	int nFrames = 1, midiUnitsPerFrame = 0;
	if (doVideo) {
		nFrames = ((int)duration + 1) * s.FPS;//calculate length of video in frames (round to nearest upper second
		std::cout << "duration (s): " << duration << "  Last time in midi units: " << lastTime << "  FPS: " << s.FPS << "\n" << std::endl;
		midiUnitsPerFrame = (double)lastTime / (double)(duration * s.FPS);//calculate the length of each frame in terms of midi units (don't round to nearest here)
	}

	s.nPixX = s.nPixX % 4 + s.nPixX; // make sure divisible by 4 for bitmap
	for (int i = 0; i < 12; i++) s.hueArray[i] = RGBToHSL(s.colorArray[0][i], s.colorArray[1][i], s.colorArray[2][i]).at(0);

	std::vector< std::vector< std::vector < unsigned char > > > miniScoreImageValues(s.nPixX, std::vector< std::vector < unsigned char > >(s.miniScoreYPix - 1, std::vector < unsigned char >(3, 0)));
	if (s.doMiniScore && !s.doMultiThreading) {
		trackList2Image(trackList, miniScoreImageValues, barLines, s.nPixX, s.miniScoreYPix - 1, s);
	}

	//calculate what the image looks like
	if (doVideo && s.doMultiThreading) {//make threads
		//std::thread threads[s.nThreads];
		//std::vector< MidiTrack > midiTrackCopies[s.nThreads];
		std::vector< std::thread > threads(s.nThreads);
		std::vector< std::vector < MidiTrack > > midiTrackCopies(s.nThreads);

		std::vector< std::vector< std::vector < unsigned char > > > miniScoreImageValues(s.nPixX, std::vector< std::vector < unsigned char > >(s.miniScoreYPix - 1, std::vector < unsigned char >(3, 0)));
		if (s.doMiniScore) {
			trackList2Image(trackList, miniScoreImageValues, barLines, s.nPixX, s.miniScoreYPix - 1 ,s);
		}

		for (int i = 0; i < s.nThreads; i++) {
			midiTrackCopies[i] = trackList;//give it its own memory of midiTrack to work with
			s.threadStartingFrame[i] = s.startingFrame + (int)(((s.stopEarly ? s.stopAfterNFrames + s.startingFrame : nFrames) - s.startingFrame) * i / (double)s.nThreads);
			s.lastThreadFrame[i] = s.startingFrame + (int)(((s.stopEarly ? s.stopAfterNFrames + s.startingFrame : nFrames) - s.startingFrame) * (i + 1) / (double)s.nThreads);
			s.assignedFrames[i] = s.lastThreadFrame[i] - s.threadStartingFrame[i];
			std::cout << "Making thread " << i << " to render frames " << s.threadStartingFrame[i] << " to " << s.lastThreadFrame[i] - 1 << std::endl;
			threads[i] = std::thread(renderThread, i, std::ref(midiTrackCopies[i]), nFrames, midiUnitsPerFrame, std::ref(barLines), std::ref(miniScoreImageValues), std::ref(s), s.threadStartingFrame[i], s.lastThreadFrame[i]);
		}

		std::clock_t messageTimer = std::clock();
		std::clock_t jobTimer = std::clock();
		while (true) { //while waiting for all threads to complete, output status messages every 15s
			bool isStillWorking = false;
			for (int i = 0; i < s.nThreads; i++) {
				if (s.completedFrames[i] != s.assignedFrames[i]) isStillWorking = isStillWorking || true;
			}
			if (!isStillWorking) break;
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			if (((std::clock() - messageTimer) / (double)CLOCKS_PER_SEC) > s.timeBetweenThreadUpdates) {
				completed.lock();
				execTime.lock();
				int totalCompleted = 0;
				int totalAssigned = 0;
				for (int i = 0; i < s.nThreads; i++) {
					if (i == 0) {
						std::cout << "\n" << std::setfill('-') << std::setw(101) << "-" << std::setfill(' ') << std::endl;
					}
					totalCompleted += s.completedFrames[i];
					totalAssigned += s.assignedFrames[i];
					int completedPercent = (int)(100 * s.completedFrames[i] / (double)s.assignedFrames[i]);
					std::cout << "Thread " << i << " (Frames " << std::right << std::setw(5) << s.threadStartingFrame[i] << "--" << std::left << std::setw(5) << s.lastThreadFrame[i] - 1 << std::right << "): " << std::setw(6) << s.completedFrames[i] << "/" << std::left << std::setw(6) << s.assignedFrames[i];
					std::cout << std::right << " (" << std::setw(3) << completedPercent << "%)";
					std::cout << "  " << std::setw(4) << (std::isnan(s.totalExecutionTime[i] / s.completedFrames[i]) ? " -- " : std::to_string(s.totalExecutionTime[i] / s.completedFrames[i])).c_str() << " s/frames." << std::endl;
					if (completedPercent != 100) std::cout << std::left << std::setfill('=') << std::setw((completedPercent == 0) ? completedPercent : completedPercent + 1) << "|" << ">" << std::setfill(' ') << std::setw(100 - completedPercent - 1) << std::right << "|" << std::endl;
					else  std::cout << "|" << std::setfill('=') << std::setw(99) << "=" << "|" << std::right << std::setfill(' ') << std::endl;
					if (i == (s.nThreads - 1)) {
						int totalCompletedPercent = (int)(100 * totalCompleted / (double)totalAssigned);
						std::cout << "\nTotal job: " << ((std::clock() - jobTimer) / (double)CLOCKS_PER_SEC) << " s     ";
						std::cout << std::setw(6) << totalCompleted << "/" << std::left << std::setw(6) << totalAssigned;
						std::cout << std::right << " (" << std::setw(3) << totalCompletedPercent << "%)";
						std::cout << "  " << std::setw(4) << (std::isinf(((std::clock() - jobTimer) / (double)CLOCKS_PER_SEC) / (double)totalCompleted) ? " -- " : std::to_string(((std::clock() - jobTimer) / (double)CLOCKS_PER_SEC) / (double)totalCompleted)).c_str() << " s/frame." << std::endl;
					}
				}
				std::cout << std::setfill('-') << std::setw(101) << "-" << std::setfill(' ') << std::endl;
				completed.unlock();
				execTime.unlock();
				messageTimer = std::clock();
			}
		}
		for (int i = 0; i < s.nThreads; i++) { //join threads
			threads[i].join();
		}
	}
	else
	{
		std::clock_t start = std::clock();
		std::vector< std::vector< std::vector < unsigned char > > > imageValues(s.nPixX, std::vector< std::vector < unsigned char > >(s.nPixY, std::vector < unsigned char >(3, 0)));
		for (int n = s.startingFrame; n < (s.stopEarly ? s.stopAfterNFrames + s.startingFrame : nFrames); n++) {
			if (doVideo) std::cout << "Rendering frame " << n + 1 << " out of " << (s.stopEarly ? s.stopAfterNFrames + s.startingFrame : nFrames) << ".   (" << (double)(n - s.startingFrame) / ((std::clock() - start) / (double)CLOCKS_PER_SEC) << " frames/s)" << std::endl;
			if (!doVideo) {
				if (n == s.startingFrame) {
					trackList2Image(trackList, imageValues, barLines, s.nPixX, s.nPixY, s);
				}
				else {
					break;
				}
			}
			else {
				if (n == s.startingFrame) trackList2Video(trackList, imageValues, n, nFrames, midiUnitsPerFrame, barLines, s, true);
				else     trackList2Video(trackList, imageValues, n, nFrames, midiUnitsPerFrame, barLines, s, false);
			}

			//handing data over to Image library
			if (!doVideo) std::cout << "\nMaking Image..." << std::endl;
			bool hasMiniScore = (s.doMiniScore && doVideo);
			CImg<unsigned char> img(s.nPixX, (hasMiniScore ? (s.nPixY + s.miniScoreYPix) : s.nPixY), 1, 3);
			for (int y = 0; y < (hasMiniScore ? (s.nPixY + s.miniScoreYPix) : s.nPixY); y++) {
				if (hasMiniScore && y >= s.nPixY) {//for miniScore
					if (y == s.nPixY) {
						for (int x = 0; x < s.nPixX; x++) {
							img(x, y, 0) = s.miniScoreHighlight[0];
							img(x, y, 1) = s.miniScoreHighlight[1];
							img(x, y, 2) = s.miniScoreHighlight[2];
						}
					}
					else {
						int firstX = -9999;
						double fracDone = n / (double)nFrames;//highlighting position in mini score
						double fracWindow = (double)(s.nPixX - 2 * s.xBufferSize) / (midiUnitsPerFrame * s.scrollSpeedFactor / s.tempoScale) / 2.0 / (double)nFrames;
						for (int x = 0; x < s.nPixX; x++) {
							unsigned char red = 0, green = 0, blue = 0;
							img(x, y, 0) = miniScoreImageValues.at(x).at(y - 1 - s.nPixY).at(2);//R
							img(x, y, 1) = miniScoreImageValues.at(x).at(y - 1 - s.nPixY).at(1);//G
							img(x, y, 2) = miniScoreImageValues.at(x).at(y - 1 - s.nPixY).at(0);//B
							//std::cout << n << " " << nFrames << " " << fracDone << std::endl;
							//std::cout << (x - xBufferSize) / (double)(nPixX - 2 * xBufferSize) << " " << fracDone - fracWindow <<  std::endl;
							//std::cout << (x - xBufferSize) / (double)(nPixX - 2 * xBufferSize) << " " << fracDone + fracWindow << std::endl;
							if (firstX != -9999 && ((firstX != 0 && x >= firstX + fracWindow * 2 * (s.nPixX - 2 * s.xBufferSize)) || (firstX == 0 && x >= s.xBufferSize + fracWindow * (s.nPixX - 2 * s.xBufferSize)))) continue;
							if ((x - s.xBufferSize) / (double)(s.nPixX - 2 * s.xBufferSize) >= fracDone - fracWindow)
							{
								if (firstX == -9999) firstX = x;
								if (miniScoreImageValues.at(x).at(y - 1 - s.nPixY).at(2) == 0 && miniScoreImageValues.at(x).at(y - 1 - s.nPixY).at(1) == 0 && miniScoreImageValues.at(x).at(y - 1 - s.nPixY).at(0) == 0) {
									img(x, y, 0) = s.miniScoreHighlight[0];
									img(x, y, 1) = s.miniScoreHighlight[1];
									img(x, y, 2) = s.miniScoreHighlight[2];
								}
							}
						}
					}
				}
				else {
					for (int x = 0; x < s.nPixX; x++) {
						unsigned char red = 0, green = 0, blue = 0;
						if (!doVideo || s.noFrameScanning) {
							img(x, y, 0) = imageValues.at(x).at(y).at(2);//R
							img(x, y, 1) = imageValues.at(x).at(y).at(1);//G
							img(x, y, 2) = imageValues.at(x).at(y).at(0);//B
						}
						else
						{
							img(x, y, 0) = imageValues.at(((int)(n * midiUnitsPerFrame * s.scrollSpeedFactor / s.tempoScale + x) % s.nPixX)).at(y).at(2);//R
							img(x, y, 1) = imageValues.at(((int)(n * midiUnitsPerFrame * s.scrollSpeedFactor / s.tempoScale + x) % s.nPixX)).at(y).at(1);//G
							img(x, y, 2) = imageValues.at(((int)(n * midiUnitsPerFrame * s.scrollSpeedFactor / s.tempoScale + x) % s.nPixX)).at(y).at(0);//B
						}
					}
				}
			}
			temporaryFileName = s.inputFile;
			img.save("temporary.bmp");

			//convert to png
			std::vector<unsigned char> bmp;
			lodepng::load_file(bmp, "temporary.bmp");
			std::vector<unsigned char> image;
			unsigned w, h;
			unsigned error = decodeBMP(image, w, h, bmp);
			std::vector<unsigned char> png;
			error = lodepng::encode(png, image, w, h);
			temporaryFileName = s.inputFile;
			if (!doVideo) lodepng::save_file(png, temporaryFileName.append(".png").data());
			else {
				temporaryFileName += std::to_string(100000 + n);//add 100000 just to ensure all have same number of sig figs (poor man's way)
				lodepng::save_file(png, temporaryFileName.append(".png").data());
			}
		}
		double runTime = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		std::cout << "\nRendering took " << runTime << " seconds (" << runTime / (double)(s.stopEarly ? s.stopAfterNFrames + s.startingFrame : nFrames) << " s/frame)." << std::endl;
	}
}

int main()
{
	/*std::pair< std::vector< int >, std::pair < std::vector < short >, std::vector < short > > > wavInfo;
	readWav(wavInfo);
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	return 0;*/

	//std::cin.get();

	Settings s = Settings();

	if (!s.makeVideo || s.makeImage) makeVisual(s,false);
	//std::cin.get();

	if (s.makeVideo) makeVisual(s,true);
	std::cout << "FINISHED! \n Press ENTER to continue...";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
	return 0;
}