#pragma once
#include "include/stdafx.h"
#include "include/Settings.h"
#include "include/midiTrack.h"
#include <iostream> 
#include <fstream>
#include <vector>
#include <string>
#include <algorithm>


void readInputFile(std::string inputFileName, std::vector< MidiTrack >& trackList, std::vector< int >& barLines, double& duration, Settings & s);