#include <cstdlib>

#include "AsteroidSnapshot.h"
#include "shades.h"

// Public member definitions

AsteroidSnapshot::AsteroidSnapshot(std::string filePath, std::uint8_t binThreshold) :
	AsteroidSnapshot(png::image<pixel_value_type>(filePath), binThreshold) {}

AsteroidSnapshot AsteroidSnapshot::resampleArea(std::uint32_t x0, std::uint32_t y0,
                                                std::uint32_t x1, std::uint32_t y1,
                                                std::uint32_t newWidth, std::uint32_t newHeight,
                                                std::uint8_t binThresh) const {

	// Validating the arguments - comment out for slight increase in performance
	get(x0, y0);
	get(x1-1, y1-1);

	texture_type areaTexture(boost::extents[newWidth][newHeight]);

	const std::uint32_t oldWidth = x1-x0;
	const std::uint32_t oldHeight = y1-y0;
	std::uint32_t xpp0, ypp0, xpp1, ypp1; // coordinates of the sub-area that will serve as a prototype for a pixel in the new texture
	unsigned long accumulator;
	for(unsigned i=0; i<newWidth; i++)
		for(unsigned j=0; j<newHeight; j++) {

			xpp0 = x0 + (i*oldWidth) / newWidth;
			xpp1 = x0 + ((i+1)*oldWidth) / newWidth;
			if( xpp1==xpp0 ) xpp1++;

			ypp0 = y0 + (j*oldHeight) / newHeight;
			ypp1 = y0 + ((j+1)*oldHeight) / newHeight;
			if( ypp1==ypp0 ) ypp1++;
//			std::cout << "i=" << i << " j=" << j << " ypp0=" << ypp0 << " ypp1=" << ypp1 << std::endl;

			accumulator = 0;
			for(unsigned x=xpp0; x<xpp1; x++)
				for(unsigned y=ypp0; y<ypp1; y++)
					accumulator += texture[x][y];

			areaTexture[i][j] = static_cast<pixel_value_type>( accumulator / ((xpp1-xpp0)*(ypp1-ypp0)) );
		}

	return AsteroidSnapshot(newWidth, newHeight, areaTexture, binThresh);
}

const AsteroidSnapshot& AsteroidSnapshot::cachingResampleArea(std::uint32_t x0, std::uint32_t y0,
                                                       std::uint32_t x1, std::uint32_t y1,
                                                       std::uint32_t newWidth, std::uint32_t newHeight,
                                                       std::uint8_t binThresh) {

	std::tuple<std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint8_t> callParams = std::make_tuple(x0, y0, x1, y1, newWidth, newHeight, binThresh);

	auto callit = areaCache.find(callParams);
	if(callit == areaCache.end()) {
		areaCache.emplace(callParams, resampleArea(x0, y0, x1, y1, newWidth, newHeight, binThresh));
		callit = areaCache.find(callParams);
	}
	return areaCache[callParams];
}


void AsteroidSnapshot::print(unsigned thumbSize, bool shades) const {

	std::cout << "Asteroid snapshot of width " << width << " and height " << height << std::endl;
	AsteroidSnapshot thumb = resampleArea(0, 0, width, height, thumbSize, thumbSize, binarizationThreshold);
	for(unsigned i=0; i<thumbSize; i++) {
		for(unsigned j=0; j<thumbSize; j++)
			if(shades)
				std::cout << shade128(thumb.get(i, j));
			else
				std::cout << static_cast<unsigned>(thumb.get(i, j)) << '\t';
		std::cout << std::endl;
	}
}

bool AsteroidSnapshot::binaryIsTheSame(const AsteroidSnapshot& other) const {

	for(std::uint32_t i=0; i<width; i++)
		for(std::uint32_t j=0; j<height; j++)
			if(binaryTexture[i][j] != other.getBinary(i, j))
				return false;
	return true;
}

void AsteroidSnapshot::printBinary(bool shades) const {
	std::cout << getPrintedBinary();
}

std::string AsteroidSnapshot::getPrintedBinary(bool shades) const {

	std::ostringstream ss;
	ss << "Asteroid binary snapshot of width " << width << " and height " << height << " (thresholded at " << static_cast<unsigned>(binarizationThreshold) << ")" << std::endl;
	for(unsigned i=0; i<width; i++) {
		for(unsigned j=0; j<height; j++)
			if(shades)
				ss << shadeBinary(binaryTexture[i][j]);
			else
				ss << static_cast<unsigned>(binaryTexture[i][j]);
		ss << std::endl;
	}
	return ss.str();
}

// Private member definitions

unsigned long AsteroidSnapshot::allocatedPixels = 0;

AsteroidSnapshot::AsteroidSnapshot(const png::image<pixel_value_type>& picture, std::uint8_t binThreshold) :
	width(picture.get_width()),
	height(picture.get_height()),
	texture(boost::extents[width][height]),
	binarizationThreshold(binThreshold),
	binaryTexture(boost::extents[width][height]) {

	for(std::uint32_t i=0; i<width; i++)
		for(std::uint32_t j=0; j<height; j++)
			texture[i][j] = static_cast<pixel_value_type>(picture[i][j]);
	allocatedPixels += height*width;

	if( allocatedPixels > maxPixels) {
		std::cerr << "Too much memory allocated for asteroid snaphots, exiting to avoid running out" << std::endl;
		exit(EXIT_FAILURE);
	}

	if(binThreshold > 255) {
		std::cerr << "Binarization threshold cannot exceed 255 (requested " << binThreshold << ")" << std::endl;
		exit(EXIT_FAILURE);
	}

	fillBinaryTexture();
}

AsteroidSnapshot::AsteroidSnapshot(std::uint32_t wt, std::uint32_t ht, texture_type txtr, std::uint8_t binThreshold) :
	width(wt),
	height(ht),
	texture(txtr),
	binarizationThreshold(binThreshold),
	binaryTexture(boost::extents[wt][ht]) {

	if(binThreshold > 255) {
		std::cerr << "Binarization threshold cannot exceed 255 (requested " << binThreshold << ")" << std::endl;
		exit(EXIT_FAILURE);
	}

	fillBinaryTexture();
}

void AsteroidSnapshot::fillBinaryTexture() {
	for(std::uint32_t i=0; i<width; i++)
		for(std::uint32_t j=0; j<height; j++)
			binaryTexture[i][j] = texture[i][j] > binarizationThreshold;
}
