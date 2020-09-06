#pragma once

//note struct
class noteDat {
public:
	int beginning;
	int end;
	unsigned char pitch;
	unsigned char velocity;
	int isOverlap = 0;
	int isSofter = 0;
	bool operator< (const noteDat& other) const {
		return beginning < other.beginning;
	}
};