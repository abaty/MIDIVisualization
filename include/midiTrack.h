#pragma once

#include <vector>
#include <iostream>
#include <algorithm>
#include "include/noteDat.h"

class MidiTrack {
public:
	int GetEnd();
	int GetStart();
	void AddNote(int begin, int end, unsigned char pitch, unsigned char velocity);
	void DeleteNote(int n);
	noteDat GetNote(int i);

	unsigned char getHighestNote() { return highestNote; };
	unsigned char getLowestNote() { return lowestNote; };
	int getShortestNoteLength() { return shortestNoteLength; };

	int GetNumberOfNotes();
	int LastTime();
	void RescaleTimesWithTempo(std::vector< unsigned int > map);
	void ClearTrack();

	void sortByBeginning();

	void setOverlap(int n, int j);
	void setIsSofter(int n, int j);

	MidiTrack();
	~MidiTrack();
private:
	std::vector< noteDat > notes; //should be sorted first to last by when notes TURN OFF, unless sorted by sortByBeginning
	int nNotes;
	unsigned char highestNote;
	unsigned char lowestNote;
	int shortestNoteLength;
};

