#pragma once

#include <vector>
#include <iostream>

class MidiTrack {
public:
	int GetEnd();
	void AddNote(int begin, int end, unsigned char pitch, unsigned char velocity);
	noteDat GetNote(int i);

	unsigned char getHighestNote() { return highestNote; };
	unsigned char getLowestNote() { return lowestNote; };
	int getShortestNoteLength() { return shortestNoteLength; };

	int GetNumberOfNotes();
	int LastTime();
	void ClearTrack();

	MidiTrack();
	~MidiTrack();
private:
	std::vector< noteDat > notes; //should be sorted first to last by when notes TURN OFF
	unsigned char highestNote;
	unsigned char lowestNote;
	int shortestNoteLength;
};

void MidiTrack::AddNote(int begin, int end, unsigned char pitch, unsigned char velocity) {
	noteDat n;
	n.beginning = begin;
	n.end = end;
	n.pitch = pitch;
	n.velocity = velocity;
	notes.push_back(n);

	if (n.end - n.beginning < shortestNoteLength) shortestNoteLength = n.end - n.beginning;
	if (n.pitch < lowestNote) lowestNote = n.pitch;
	if (n.pitch > highestNote) highestNote = n.pitch;
}

noteDat MidiTrack::GetNote(int i) {
	return notes.at(i);
}

int MidiTrack::GetNumberOfNotes() {
	return notes.size();
}

int MidiTrack::LastTime() {
	return this->GetNote(this->GetNumberOfNotes()).end;
}

int MidiTrack::GetEnd() {
	int lastTime = 0;
	for (unsigned int i = 0; i < notes.size(); i++) {
		if (notes.at(i).end > lastTime) lastTime = notes.at(i).end;
	}
	return lastTime;
}

void MidiTrack::ClearTrack() {
	notes.clear();
}

MidiTrack::MidiTrack() {
	shortestNoteLength = 1000000;
	highestNote = 0;
	lowestNote = 255;
}

MidiTrack::~MidiTrack() {}