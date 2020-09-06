#pragma once
#include "include/stdafx.h"
#include "include/Settings.h"
#include "include/midiTrack.h"
#include <iostream>  
#include <limits>
#include <vector>
#include <cmath>
#include "include/colorTools.h"

inline void setColor(unsigned char R, unsigned char G, unsigned char B, int x, int y, std::vector< std::vector< std::vector < unsigned char > > >& imageValues);

inline bool isBackground(int x, int y, std::vector< std::vector< std::vector < unsigned char > > >& imageValues, Settings &s);

inline bool isAnotherNote(int x, int y, std::vector< std::vector< std::vector < unsigned char > > >& imageValues, int colorIndx, Settings &s);

void trackList2Image(std::vector< MidiTrack >& trackList, std::vector< std::vector< std::vector < unsigned char > > >& imageValues, std::vector< int >& barLines, int nPixX_, int nPixY_, Settings &s);

void trackList2Video(std::vector< MidiTrack >& trackList, std::vector< std::vector< std::vector < unsigned char > > >& imageValues, int n, int nFrames, double midiUnitsPerFrame, std::vector< int >& barLines, Settings &s, bool renderFullFrame = false);