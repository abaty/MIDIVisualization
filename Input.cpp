#include "stdafx.h"
#include "Settings.h"
#include "midiTrack.h"
#include <iostream> 
#include <fstream>
#include <vector>
#include <string>

void readInputFile(std::string inputFileName, std::vector< MidiTrack >& trackList) {

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
		int T = 0; //absolute time of the track

		while (std::getline(inFile, buffer))
		{
			//std::cout << buffer << std::endl; //for debug
			if (buffer.find("# TRACK") != std::string::npos) {  //open new track (only 1 open at a time assumed)
				std::cout << "Reading track number " << trackList.size() + 1 << std::endl;
				isTrackOpen = true;
				T = 0;
			}

			if (buffer.find("Tempo") != std::string::npos) {
				if (buffer.find("# ") != std::string::npos) continue;
				T += std::stoi(buffer.substr(0, buffer.find("Tempo")));
				continue;
			}
			if (buffer.find("Key") != std::string::npos) {
				if (buffer.find("# ") != std::string::npos) continue;
				T += std::stoi(buffer.substr(0, buffer.find("Key")));
				continue;
			}

			if (buffer.find(" n=") != std::string::npos) {                //line designating a note turning on or off
				if (buffer.find("v=") == std::string::npos) {                //if it is turning off (no velocity specified)

					for (unsigned int i = 0; i < notesTurnedOn.size(); i++){ //check to see if this note is turned on
						std::string pitchAndVelocity = buffer.substr(buffer.find("n=") + 2);
						unsigned char pitch = (unsigned char)std::stoi(pitchAndVelocity);
						if (notesTurnedOn.at(i).first.first == pitch) {            //if it is, record it to track and delete from temporary list
							track.AddNote(notesTurnedOn.at(i).first.second, T+std::stoi(buffer.substr(0, buffer.find(" n="))), notesTurnedOn.at(i).first.first, notesTurnedOn.at(i).second);
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

					std::pair< unsigned char, int > pair(pitch, T+start);
					std::pair< std::pair< unsigned char, int >, unsigned char > onNote(pair,velocity);
					notesTurnedOn.push_back(onNote);
					T += start;//add dt of this event to total time
				}
			}

			if (buffer.find("End of track") != std::string::npos) {  //close track and put it in list
				std::cout << "Track Length: " << T << " ticks." << std::endl;
				trackList.push_back(track);
				track.ClearTrack();
				isTrackOpen = false;
			}
		}
	}
	//debug for scanning through tracks
	/*
	for (unsigned int i = 0; i < trackList.size(); i++) {
		std::cout << trackList.at(i).GetNumberOfNotes() << " " << trackList.at(i).GetEnd() << std::endl;
		std::cout << trackList.at(i).GetNote(trackList.at(i).GetNumberOfNotes()-1).beginning << " " << trackList.at(i).GetNote(trackList.at(i).GetNumberOfNotes()-1).end << " " << (int)trackList.at(i).GetNote(trackList.at(i).GetNumberOfNotes()-1).pitch << " " << (int)trackList.at(i).GetNote(trackList.at(i).GetNumberOfNotes()-1).velocity << std::endl;
	}*/
}