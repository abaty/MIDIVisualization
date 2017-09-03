#pragma once
#include<vector>
#include<iostream>
#include<algorithm>
#include<cmath>

std::vector< double > RGBToHSL(unsigned char R, unsigned char G, unsigned char B) {
	double r = R/255.0, g = G/255.0, b = B/255.0;
	double max = fmax(r, fmax(g, b));
	double minimum = fmin(r, fmin(g, b));
	double h, s;
	double l = (max + minimum) / 2;

	if (max == minimum) {
		h = 0; // achromatic
		s = 0;
	}
	else {
		double d = max - minimum;
		s = l > 0.5 ? d / (2 - max - minimum) : d / (max + minimum);
		if (max == r) {
			h = (g - b) / d + (g < b ? 6 : 0);
		}
		else if (max == g) {
			h = (b - r) / d + 2;
		}
		else if(max==b){ 
			h = (r - g) / d + 4;
		}
		h = h / 6.;
	}

	std::vector< double > HSL;
	HSL.push_back(h);
	HSL.push_back(s);
	HSL.push_back(l);

	return HSL;
}

double hue2rgb(double p, double q, double t) {
	if (t < 0) t += 1;
	if (t > 1) t -= 1;
	if (t < 1. / 6.) return p + (q - p) * 6. * t;
	if (t < 1. / 2.) return q;
	if (t < 2. / 3.) return p + (q - p) * (2. / 3. - t) * 6.;
	return p;
}

std::vector< unsigned char > HSLToRGB(double H, double S, double L) {

	double r, g, b;

	if (S == 0) {
		r = L;
		g = L;
		b = L; // achromatic
	}
	else {
		double q = L < 0.5 ? L * (1 + S) : L + S - L * S;
		double p = 2 * L - q;
		r = hue2rgb(p, q, H + 1. / 3.);
		g = hue2rgb(p, q, H);
		b = hue2rgb(p, q, H - 1. / 3.);
	}

	std::vector< unsigned char > RGB;
	RGB.push_back(round(r * 255));
	RGB.push_back(round(g * 255));
	RGB.push_back(round(b * 255));
	return RGB;
}

std::vector< unsigned char > decayRGBValue(unsigned char R, unsigned char G, unsigned char B, double elapsedFracNum, double elapsedFracDenom, double otherScaling = 1, double initialL = 1, double nTaus = 2.5) {
	//decays the intensity from white to the original value with exponential decay
	//nTau controls the strength of decay (higher=faster)
	std::vector< double > HSL = RGBToHSL(R, G, B);
	double max = fmax(HSL.at(2), initialL);
	//HSL.at(2) += otherScaling*(max - HSL.at(2))*pow(2.718, -elapsedFracNum/elapsedFracDenom*nTaus); //replace L with something lighter, exponential decay
	if (elapsedFracDenom < 10) {
		HSL.at(2) += otherScaling*(max - HSL.at(2));
	}
	else if (elapsedFracDenom < 30) {
		HSL.at(2) += otherScaling*(max - HSL.at(2))*((elapsedFracNum / elapsedFracDenom<0.5)?1:(1 - elapsedFracNum / elapsedFracDenom));
	}
	else {
		HSL.at(2) += otherScaling*(max - HSL.at(2))*(1 - elapsedFracNum / elapsedFracDenom);//replace L with something lighter, linear decay
	}
	return HSLToRGB(HSL.at(0), HSL.at(1), HSL.at(2));
}