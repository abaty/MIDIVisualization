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
#include <thread>
#include <mutex>
#include <iomanip>
#include <chrono>

using namespace cimg_library;

void renderThread(int processID, std::vector< MidiTrack >& trackList, int nFrames, int midiUnitsPerFrame, int startingFrame = 0, int endingFrame = 1) {
	std::clock_t start = std::clock();
	std::vector< std::vector< std::vector < unsigned char > > > imageValues(nPixX, std::vector< std::vector < unsigned char > >(nPixY, std::vector < unsigned char >(3, 0)));
	for (int n = startingFrame; n < endingFrame; n++) {
		//std::cout << "Thread " << processID << " rendering frame " << n+1 << " out of " << endingFrame  << ".   (" << (double)(n - startingFrame) / ((std::clock() - start) / (double)CLOCKS_PER_SEC) << " frames/s)" << std::endl;

		if (n == startingFrame) trackList2Video(trackList, imageValues, n, nFrames, midiUnitsPerFrame, true);
		else     trackList2Video(trackList, imageValues, n, nFrames, midiUnitsPerFrame, false);

		//handing data over to Image library
		CImg<unsigned char> img(nPixX, nPixY, 1, 3);
		for (int y = 0; y < nPixY; y++) {
			for (int x = 0; x < nPixX; x++) {
				unsigned char red = 0, green = 0, blue = 0;
				img(x, y, 0) = imageValues.at(((int)(n*midiUnitsPerFrame*scrollSpeedFactor / tempoScale + x) % nPixX)).at(y).at(2);//R
				img(x, y, 1) = imageValues.at(((int)(n*midiUnitsPerFrame*scrollSpeedFactor / tempoScale + x) % nPixX)).at(y).at(1);//G
				img(x, y, 2) = imageValues.at(((int)(n*midiUnitsPerFrame*scrollSpeedFactor / tempoScale + x) % nPixX)).at(y).at(0);//B
			}
		}
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
		temporaryFileName = inputFile;
		temporaryFileName += std::to_string(100000 + n);//add 100000 just to ensure all have same number of sig figs (poor man's way)
		lodepng::save_file(png, temporaryFileName.append(".png").data());

		completed.lock();
		completedFrames[processID]++;
		completed.unlock();
		execTime.lock();
		totalExecutionTime[processID] = ((std::clock() - start) / (double)CLOCKS_PER_SEC);
		execTime.unlock();
	}
}

void makeVisual(bool doVideo = false)
{
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
		std::cout << "duration (s): " << duration << "  Last time in midi units: " << lastTime << "  FPS: " << FPS << "\n"<< std::endl;
		midiUnitsPerFrame = (double)lastTime / (double)(duration*FPS);//calculate the length of each frame in terms of midi units (don't round to nearest here)
	}

	nPixX = nPixX % 4 + nPixX; // make sure divisible by 4 for bitmap

	//calculate what the image looks like
	if (doVideo && doMultiThreading) {//make threads
		std::thread threads[nThreads];
		std::vector< MidiTrack > midiTrackCopies[nThreads];
		for (int i = 0; i < nThreads; i++) {
			midiTrackCopies[i] = trackList;//give it its own memory of midiTrack to work with
			threadStartingFrame[i] = startingFrame + (int)(((stopEarly ? stopAfterNFrames + startingFrame : nFrames)-startingFrame)*i / (double)nThreads);
			lastThreadFrame[i] = startingFrame + (int)(((stopEarly ? stopAfterNFrames + startingFrame : nFrames) - startingFrame)*(i+1) / (double)nThreads);
			assignedFrames[i] = lastThreadFrame[i] - threadStartingFrame[i];
			std::cout << "Making thread " << i << " to render frames " << threadStartingFrame[i] << " to " << lastThreadFrame[i] - 1 << std::endl;
			threads[i] = std::thread(renderThread,i, midiTrackCopies[i], nFrames, midiUnitsPerFrame, threadStartingFrame[i], lastThreadFrame[i]);
		}

		std::clock_t messageTimer = std::clock();
		std::clock_t jobTimer = std::clock();
		while (true) { //while waiting for all threads to complete, output status messages every 15s
			bool isStillWorking = false;
			for (int i = 0; i < nThreads; i++) {
				if (completedFrames[i] != assignedFrames[i]) isStillWorking = isStillWorking || true;
			}
			if (!isStillWorking) break;
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			if (((std::clock() - messageTimer) / (double)CLOCKS_PER_SEC) > timeBetweenThreadUpdates) {
				completed.lock();
				execTime.lock();
				int totalCompleted = 0;
				int totalAssigned = 0;
				for (int i = 0; i < nThreads; i++) {
					if (i == 0) {
						std::cout << "\n" << std::setfill('-') << std::setw(101) << "-" << std::setfill(' ') << std::endl;
					}
					totalCompleted += completedFrames[i];
					totalAssigned += assignedFrames[i];
					int completedPercent = (int)(100*completedFrames[i] / (double)assignedFrames[i]);
					std::cout << "Thread " << i << " (Frames " << std::right << std::setw(5) << threadStartingFrame[i] << "--" << std::left << std::setw(5) << lastThreadFrame[i] - 1 << std::right << "): " << std::setw(6) << completedFrames[i] << "/" << std::left << std::setw(6) << assignedFrames[i];
					std::cout << std::right <<" (" << std::setw(3) << completedPercent << "%)";
					std::cout << "  " << std::setw(4) << (std::isnan(totalExecutionTime[i]/ completedFrames[i])?" -- ":std::to_string(totalExecutionTime[i] / completedFrames[i])).c_str() << " s/frames." << std::endl;
					if(completedPercent != 100) std::cout << std::left << std::setfill('=') << std::setw((completedPercent==0)? completedPercent : completedPercent+1) << "|" << ">" << std::setfill(' ') << std::setw(100 - completedPercent - 1) << std::right << "|" << std::endl;
					else  std::cout << "|" << std::setfill('=') << std::setw(99) << "=" << "|" << std::right << std::setfill(' ') << std::endl;
					if (i == (nThreads - 1)) {
						int totalCompletedPercent = (int)(100 * totalCompleted / (double)totalAssigned);
						std::cout << "\nTotal job: " << ((std::clock() - jobTimer) / (double)CLOCKS_PER_SEC) << " s     ";
						std::cout << std::setw(6) << totalCompleted << "/" << std::left << std::setw(6) << totalAssigned;
						std::cout << std::right << " (" << std::setw(3) << totalCompletedPercent << "%)";
						std::cout << "  " << std::setw(4) << (std::isinf(((std::clock() - jobTimer) / (double)CLOCKS_PER_SEC)/ (double)totalCompleted) ? " -- " : std::to_string(((std::clock() - jobTimer) / (double)CLOCKS_PER_SEC) / (double)totalCompleted)).c_str() << " s/frame." << std::endl;
					}
				}
				std::cout << std::setfill('-') << std::setw(101) << "-" << std::setfill(' ') << std::endl;
				completed.unlock();
				execTime.unlock();
				messageTimer = std::clock();
			}
		}
		for (int i = 0; i < nThreads; i++) { //join threads
			threads[i].join();
		}
	}
	else
	{
		std::clock_t start = std::clock();
		std::vector< std::vector< std::vector < unsigned char > > > imageValues(nPixX, std::vector< std::vector < unsigned char > >(nPixY, std::vector < unsigned char >(3, 0)));
		for (int n = startingFrame; n < (stopEarly ? stopAfterNFrames + startingFrame : nFrames); n++) {
			if (doVideo) std::cout << "Rendering frame " << n+1 << " out of " << (stopEarly ? stopAfterNFrames + startingFrame : nFrames) << ".   (" << (double)(n - startingFrame) / ((std::clock() - start) / (double)CLOCKS_PER_SEC) << " frames/s)" << std::endl;

			if (!doVideo) trackList2Image(trackList, imageValues);
			else {
				if (n == startingFrame) trackList2Video(trackList, imageValues, n, nFrames, midiUnitsPerFrame, true);
				else     trackList2Video(trackList, imageValues, n, nFrames, midiUnitsPerFrame, false);
			}

			//handing data over to Image library
			if (!doVideo) std::cout << "\nMaking Image..." << std::endl;
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
						img(x, y, 0) = imageValues.at(((int)(n*midiUnitsPerFrame*scrollSpeedFactor / tempoScale + x) % nPixX)).at(y).at(2);//R
						img(x, y, 1) = imageValues.at(((int)(n*midiUnitsPerFrame*scrollSpeedFactor / tempoScale + x) % nPixX)).at(y).at(1);//G
						img(x, y, 2) = imageValues.at(((int)(n*midiUnitsPerFrame*scrollSpeedFactor / tempoScale + x) % nPixX)).at(y).at(0);//B
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
			if (!doVideo) lodepng::save_file(png, temporaryFileName.append(".png").data());
			else {
				temporaryFileName += std::to_string(100000 + n);//add 100000 just to ensure all have same number of sig figs (poor man's way)
				lodepng::save_file(png, temporaryFileName.append(".png").data());
			}
		}
		double runTime = (std::clock() - start) / (double)CLOCKS_PER_SEC;
		std::cout << "\nRendering took " << runTime << " seconds (" << runTime / (double)(stopEarly ? stopAfterNFrames + startingFrame : nFrames) << " s/frame)." << std::endl;
	}
}

int main()
{
	if(!makeVideo || makeImage) makeVisual(false);
	if(makeVideo) makeVisual(true);
	std::cout << "FINISHED! \n Press ENTER to continue...";
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    return 0;
}