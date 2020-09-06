#pragma once
#include "include/midiTrack.h"

void MidiTrack::setOverlap(int n, int j) {
	notes.at(n).isOverlap = j;
	return;
}
void MidiTrack::setIsSofter(int n, int j) {
	notes.at(n).isSofter = j;
	return;
}

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

void MidiTrack::DeleteNote(int n) {
	notes.erase(notes.begin() + n);
}

//rescales all times in order to be in terms of the first tempo
void MidiTrack::RescaleTimesWithTempo(std::vector< unsigned int> map) {
	shortestNoteLength = 100000000;
	if (notes.size() == 0) return;
	for (unsigned int i = 0; i < notes.size(); i++) {
		notes.at(i).beginning = map.at(notes.at(i).beginning);
		notes.at(i).end = map.at(notes.at(i).end);
		if (notes.at(i).end - notes.at(i).beginning < shortestNoteLength) shortestNoteLength = notes.at(i).end - notes.at(i).beginning;
	}
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

int MidiTrack::GetStart() {
	if (notes.size() == 0) return 1000000000;
	return notes.front().beginning;
}

void MidiTrack::ClearTrack() {
	notes.clear();
}

void MidiTrack::sortByBeginning() {
	std::sort(notes.begin(), notes.end());
}

MidiTrack::MidiTrack() {
	shortestNoteLength = 1000000;
	highestNote = 0;
	lowestNote = 255;
}

MidiTrack::~MidiTrack() {}