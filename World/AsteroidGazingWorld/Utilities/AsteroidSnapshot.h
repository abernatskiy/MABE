#pragma once

#include "boost/multi_array.hpp"
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>
#include <tuple>
#include <png++/png.hpp>

typedef std::uint8_t pixel_value_type; // a.k.a. png::gray_pixel a.k.a. unsigned char - convention as of Nov 5th 2018
typedef boost::multi_array<pixel_value_type,2> texture_type;
typedef boost::multi_array<bool,2> bitmap_type;

class AsteroidSnapshot {

public:
	const std::uint32_t width;
	const std::uint32_t height;

	const std::uint8_t binarizationThreshold;

	AsteroidSnapshot(std::string filePath, std::uint8_t binThreshold);
	AsteroidSnapshot() : width(0), height(0), binarizationThreshold(127) {};

	inline pixel_value_type get(std::uint32_t x, std::uint32_t y) const {
//		if( x >= height || y >= width ) {
//			std::cerr << "A value of pixel outside of the picture frame was requested. Width " << width << ", heigh " << height << ", requested value at x=" << x << ", y=" << y << std::endl;
//			exit(EXIT_FAILURE);
//		}
		return texture[x][y];
	}

	AsteroidSnapshot resampleArea(std::uint32_t x0, std::uint32_t y0,
	                              std::uint32_t x1, std::uint32_t y1,
	                              std::uint32_t newWidth, std::uint32_t newHeight,
	                              std::uint8_t binThresh) const;
	const AsteroidSnapshot& cachingResampleArea(std::uint32_t x0, std::uint32_t y0,
	                                            std::uint32_t x1, std::uint32_t y1,
	                                            std::uint32_t newWidth, std::uint32_t newHeight,
	                                            std::uint8_t binThresh);
	void print(unsigned thumbSize=20, bool shades=true) const;

	inline bool getBinary(std::uint32_t x, std::uint32_t y) const { return binaryTexture[x][y]; };
	bool binaryIsTheSame(const AsteroidSnapshot& other) const;
	void printBinary(bool shades=true) const;
	std::string getPrintedBinary(bool shades=true) const;
	unsigned countBinaryOnes() const;
	std::uint8_t getBestThreshold(unsigned resolution, unsigned numLevels) const; // returns the threshold that results in the highest entropy of the output (that is, the one that makes the output the closest to 50% ones, 50% zeros)

private:
	texture_type texture;
	static unsigned long allocatedPixels;
	const unsigned long maxMebibytes = 10240; // dataset must fit into 10 GiB
	const unsigned long maxPixels = maxMebibytes * 1024 * 1024 / sizeof(pixel_value_type);

	std::map<std::tuple<std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint8_t>,AsteroidSnapshot> areaCache;

	bitmap_type binaryTexture;

	AsteroidSnapshot(const png::image<pixel_value_type>& picture, std::uint8_t binThreshold);
	AsteroidSnapshot(std::uint32_t width, std::uint32_t height, texture_type texture, std::uint8_t binThreshold);

	void fillBinaryTexture();
};
