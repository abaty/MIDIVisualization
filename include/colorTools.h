#pragma once
#include<vector>
#include<iostream>
#include<algorithm>
#include<cmath>

std::vector< double > RGBToHSL(unsigned char R, unsigned char G, unsigned char B);

double hue2rgb(double p, double q, double t);

std::vector< unsigned char > HSLToRGB(double H, double S, double L);

std::vector< unsigned char > decayRGBValue(unsigned char R, unsigned char G, unsigned char B, double elapsedFracNum, double elapsedFracDenom, double otherScaling = 1, double initialL = 1, double nTaus = 2.5);
