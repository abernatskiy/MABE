#pragma once

#include <sstream>
#include "boost/multi_array.hpp"
#include "shades.h"

typedef boost::multi_array<uint8_t,4> Texture; // dimensions: x, y, time, channel

inline std::string readableTextureRepr(const Texture& texture, bool binary=true) {
	// X is horizontal, increases left-to-right
	// Y is vertical, increases top-to-bottom
	// Channels increase left-to-right
	// Temporal frames are listed top-to-bottom
	using namespace std;
	stringstream ss;
	for(size_t t=0; t<texture.shape()[2]; t++) {
		ss << "All channels of frame " << t << ':' << endl;
		for(size_t x=0; x<texture.shape()[0]; x++) {
			for(size_t c=0; c<texture.shape()[3]; c++) {
				ss << (c==0 ? "" : "|");
				for(size_t y=0; y<texture.shape()[1]; y++)
					ss << (binary ? shadeBinary(texture[x][y][t][c]) : shade128(texture[x][y][t][c]));
			}
			ss << endl;
		}
	}
	return ss.str();
}
