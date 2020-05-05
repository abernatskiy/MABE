#pragma once

#include <sstream>
#include <tuple>
#include "boost/multi_array.hpp"
#include "shades.h"

typedef boost::multi_array<uint8_t,4> Texture; // dimensions: x, y, time, channel
typedef boost::array<boost::multi_array_types::index,4> TextureIndex;

inline std::string readableRepr(const Texture& texture, bool binary=true) {
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

inline TextureIndex operator+(TextureIndex left, TextureIndex right) {
	return {{left[0]+right[0], left[1]+right[1], left[2]+right[2], left[3]+right[3]}};
}

inline std::string readableRepr(TextureIndex idx) {
	return std::string("(") +
	       std::to_string(idx[0]) + "," +
	       std::to_string(idx[1]) + "," +
	       std::to_string(idx[2]) + "," +
	       std::to_string(idx[3]) + ")";
}
