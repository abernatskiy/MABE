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

class AsteroidSnapshot {

public:
	const std::uint32_t width;
	const std::uint32_t height;

	AsteroidSnapshot(std::string filePath);
	AsteroidSnapshot() : width(0), height(0) {};

	pixel_value_type get(std::uint32_t x, std::uint32_t y) const;
	AsteroidSnapshot resampleArea(std::uint32_t x0, std::uint32_t y0,
	                              std::uint32_t x1, std::uint32_t y1,
	                              std::uint32_t newWidth, std::uint32_t newHeight) const;
	const AsteroidSnapshot& cachingResampleArea(std::uint32_t x0, std::uint32_t y0,
	                                            std::uint32_t x1, std::uint32_t y1,
	                                            std::uint32_t newWidth, std::uint32_t newHeight);
	void print(unsigned thumbSize=20) const;

private:
	texture_type texture;
	static unsigned long allocatedPixels;
	const unsigned long maxMebibytes = 1024; // dataset must fit into 1 GiB
	const unsigned long maxPixels = maxMebibytes * 1024 * 1024 / sizeof(pixel_value_type);

	std::map<std::tuple<std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t>,AsteroidSnapshot> areaCache;

	AsteroidSnapshot(const png::image<pixel_value_type>& picture);
	AsteroidSnapshot(std::uint32_t width, std::uint32_t height, texture_type texture);
};
