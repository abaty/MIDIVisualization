#include "stdafx.h"
#include "Settings.h"
#include "midiTrack.h"
#include <iostream> 
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>


void readInputFile(std::string inputFileName, std::vector< MidiTrack >& trackList, std::vector< int >& barLines, double& duration) {

	std::string buffer;
	std::vector<std::string> listOfFiles;
	std::ifstream inFile(inputFileName.data());

	if (!inFile.is_open())
	{
		std::cout << "Error opening jet file. Exiting." << std::endl;
	}
	else
	{
		bool isTrackOpen = false;
		MidiTrack track;
		std::vector< std::pair< std::pair< unsigned char, int >, unsigned char > > notesTurnedOn;
		std::vector< std::pair< int, double > > tempoList;// time of tempo change, and tempo in BPM
		int T = 0; //absolute time of the track
		int beatsPerMeasure = 0;//numerator of time signature
		int ticksPerBeat = 480;

		while (std::getline(inFile, buffer))
		{
			//std::cout << buffer << std::endl; //for debug
			if (buffer.find("# Duration") != std::string::npos) {  //record duration of total song in seconds
				duration = std::stod(buffer.substr(buffer.find(" = ") + 3, buffer.find(" seconds")));
				std::cout << "Duration of " << duration << " seconds." << std::endl;
			}

			if (buffer.find("# TRACK") != std::string::npos) {  //open new track (only 1 open at a time assumed)
				std::cout << "Reading track number " << trackList.size() + 1 << std::endl;
				isTrackOpen = true;
				T = 0;
			}

			if (buffer.find("Tempo") != std::string::npos) {
				if (buffer.find("# ") != std::string::npos) continue;
				T += std::stoi(buffer.substr(0, buffer.find("Tempo")));
				std::pair< int, double > tempo(T, std::stod(buffer.substr(buffer.find("Tempo ") + 6)));
				tempoList.push_back(tempo);
				continue;
			}
			if (buffer.find("Time signature") != std::string::npos) {
				if (buffer.find("# ") != std::string::npos) continue;
				T += std::stoi(buffer.substr(0, buffer.find("Time signature")));
				if (T!=0) {
					for (int t = barLines.back()+ ticksPerBeat*beatsPerMeasure; t <= T; t += ticksPerBeat*beatsPerMeasure) {
						barLines.push_back(t);
					}
				}
				else {
					barLines.push_back(0);
				}
				beatsPerMeasure = std::stoi(buffer.substr(buffer.find("signature ") + 10, buffer.find("/")));
				ticksPerBeat = 480 * 4 / (std::stoi(buffer.substr(buffer.find("/") + 1, buffer.find(","))));
				continue;
			}
			/*if (buffer.find("Instrument") != std::string::npos) {//not sure if this is needed later in music w/ instrument changes...
				if (buffer.find("# ") != std::string::npos) continue;
				T += std::stoi(buffer.substr(0, buffer.find("Instrument")));
				continue;
			}*/
			if (buffer.find("Key") != std::string::npos) {
				if (buffer.find("# ") != std::string::npos) continue;
				T += std::stoi(buffer.substr(0, buffer.find("Key")));
				continue;
			}
			if (buffer.find("ST &B") != std::string::npos) {//sustain pedal stuff
				T += std::stoi(buffer.substr(0, buffer.find("ST &B")));
				continue;
			}

			if (buffer.find(" n=") != std::string::npos) {                //line designating a note turning on or off
				if (buffer.find("v=") == std::string::npos) {                //if it is turning off (no velocity specified)

					for (unsigned int i = 0; i < notesTurnedOn.size(); i++) { //check to see if this note is turned on
						std::string pitchAndVelocity = buffer.substr(buffer.find("n=") + 2);
						unsigned char pitch = (unsigned char)std::stoi(pitchAndVelocity);
						if (notesTurnedOn.at(i).first.first == pitch) {            //if it is, record it to track and delete from temporary list
							track.AddNote(notesTurnedOn.at(i).first.second, T + std::stoi(buffer.substr(0, buffer.find(" n="))), notesTurnedOn.at(i).first.first, notesTurnedOn.at(i).second);
							T += std::stoi(buffer.substr(0, buffer.find(" n=")));//adds dT of this event to total time
							notesTurnedOn.erase(notesTurnedOn.begin() + i);
							break;
						}
					}
				}
				else {                                                     //if it is turning on
					std::string pitchAndVelocity = buffer.substr(buffer.find("n=") + 2);
					unsigned char pitch = (unsigned char)std::stoi(pitchAndVelocity.substr(0, pitchAndVelocity.find(" v=")));
					int start = std::stoi(buffer.substr(0, buffer.find(" n=")));
					unsigned char velocity = (unsigned char)std::stoi(buffer.substr(buffer.find("v=") + 2));

					std::pair< unsigned char, int > pair(pitch, T + start);
					std::pair< std::pair< unsigned char, int >, unsigned char > onNote(pair, velocity);
					notesTurnedOn.push_back(onNote);
					T += start;//add dt of this event to total time
				}
			}

			if (buffer.find("End of track") != std::string::npos) {  //close track and put it in list
				std::cout << "Track Length: " << T << " ticks." << std::endl;
				if (trackList.size() == 0) {
					tempoList.push_back(std::pair< int, double >(T + 1, 1));// end of first track (which has the tempos)
					for (int t = barLines.back() + ticksPerBeat*beatsPerMeasure; t <= T; t += ticksPerBeat*beatsPerMeasure) {
						barLines.push_back(t);
					}
				}
				trackList.push_back(track);
				track.ClearTrack();
				isTrackOpen = false;
			}
		}

		//make a map that scales timestamps from tempo-dependent to a 'global time'
		std::cout << "\nRescaling times to account for tempo changes...\n" << std::endl;
		std::vector< unsigned int > tempoMap;
		int newt = (int)-tempoScale;
		for (unsigned int t = 0; t < (unsigned int) tempoList.at(tempoList.size()-1).first; t++) {
			for (unsigned int i = 0; i < (unsigned int) tempoList.size()-1; i++) {
				if (t < (unsigned int)tempoList.at(i + 1).first) {
					newt += (int)(tempoScale * tempoList.at(0).second / tempoList.at(i).second);//normalize to first tempo, scale by tempoScale to keep more accuracy
					break;
				}
			}
			tempoMap.push_back((unsigned int)newt);
		}
		//use map to rescale tempos of tracks
		for (unsigned int i = 0; i < trackList.size(); i++){
			if (trackList.at(i).GetNumberOfNotes() > 0) {
				trackList.at(i).RescaleTimesWithTempo(tempoMap);
				trackList.at(i).sortByBeginning();
			}
		}
		for (unsigned int i = 0; i < barLines.size(); i++) {
			barLines.at(i) = tempoMap.at(barLines.at(i));
			//std::cout << "Barline: " << barLines.at(i) << std::endl;
		}
	}

	/*for (unsigned int i = 0; i < trackList.at(25).GetNumberOfNotes(); i++) {
		std::cout << i << " " << trackList.at(25).GetNote(i).pitch << " " << trackList.at(25).GetNote(i).pitch << " " << trackList.at(25).GetNote(i).beginning << " " << trackList.at(25).GetNote(i).end << std::endl;
	}*/

	return;
}