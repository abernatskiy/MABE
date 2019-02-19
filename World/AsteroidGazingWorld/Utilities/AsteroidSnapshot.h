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

	AsteroidSnapshot(std::string filePath, unsigned binarizationThreshold);
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
	                              std::uint32_t newWidth, std::uint32_t newHeight) const;
	const AsteroidSnapshot& cachingResampleArea(std::uint32_t x0, std::uint32_t y0,
	                                            std::uint32_t x1, std::uint32_t y1,
	                                            std::uint32_t newWidth, std::uint32_t newHeight);
	void print(unsigned thumbSize=20, bool shades=true) const;

	inline bool getBinary(std::uint32_t x, std::uint32_t y) const { return binaryTexture[x][y]; };
	bool binaryIsTheSame(const AsteroidSnapshot& other) const;
	void printBinary(bool shades=true) const;
	std::string getPrintedBinary(bool shades=true) const;

private:
	texture_type texture;
	static unsigned long allocatedPixels;
	const unsigned long maxMebibytes = 1024; // dataset must fit into 1 GiB
	const unsigned long maxPixels = maxMebibytes * 1024 * 1024 / sizeof(pixel_value_type);

	std::map<std::tuple<std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t>,AsteroidSnapshot> areaCache;

	bitmap_type binaryTexture;

	AsteroidSnapshot(const png::image<pixel_value_type>& picture, unsigned binarizationThreshold);
	AsteroidSnapshot(std::uint32_t width, std::uint32_t height, texture_type texture, unsigned binarizationThreshold);

	void fillBinaryTexture();
};
