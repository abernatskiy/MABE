#include "AsteroidSnapshot.h"

// Public member definitions

AsteroidSnapshot::AsteroidSnapshot(std::string filePath) :
	AsteroidSnapshot(png::image<pixel_value_type>(filePath)) {}

pixel_value_type AsteroidSnapshot::get(std::uint32_t x, std::uint32_t y) const {

	if( x >= height || y >= width ) {
		std::cerr << "A value of pixel outside of the picture frame was requested. Width " << width << ", heigh " << height << ", requested value at x=" << x << ", y=" << y << std::endl;
		exit(EXIT_FAILURE);
	}
	return texture[x][y];
}

AsteroidSnapshot AsteroidSnapshot::resampleArea(std::uint32_t x0, std::uint32_t y0,
                                                std::uint32_t x1, std::uint32_t y1,
                                                std::uint32_t newWidth, std::uint32_t newHeight) const {

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

			xpp0 = (i*oldWidth) / newWidth;
			xpp1 = ((i+1)*oldWidth) / newWidth;
			if( xpp1==xpp0 ) xpp1++;

			ypp0 = (j*oldHeight) / newHeight;
			ypp1 = ((j+1)*oldHeight) / newHeight;
			if( ypp1==ypp0 ) ypp1++;
//			std::cout << "i=" << i << " j=" << j << " ypp0=" << ypp0 << " ypp1=" << ypp1 << std::endl;

			accumulator = 0;
			for(unsigned x=xpp0; x<xpp1; x++)
				for(unsigned y=ypp0; y<ypp1; y++)
					accumulator += texture[x][y];

			areaTexture[i][j] = static_cast<pixel_value_type>( accumulator / ((xpp1-xpp0)*(ypp1-ypp0)) );
		}

	return AsteroidSnapshot(newWidth, newHeight, areaTexture);
}

// Private member definitions

unsigned long AsteroidSnapshot::allocatedPixels = 0;

AsteroidSnapshot::AsteroidSnapshot(const png::image<pixel_value_type>& picture) :
	width(picture.get_width()), height(picture.get_height()), texture(boost::extents[width][height]) {

	for(std::uint32_t i=0; i<width; i++)
		for(std::uint32_t j=0; j<height; j++)
			texture[i][j] = static_cast<pixel_value_type>(picture[i][j]);
	allocatedPixels += height*width;

	if( allocatedPixels > maxPixels) {
		std::cerr << "Too much memory allocated for asteroid snaphots, exiting to avoid running out" << std::endl;
		exit(EXIT_FAILURE);
	}
}

AsteroidSnapshot::AsteroidSnapshot(std::uint32_t wt, std::uint32_t ht, texture_type txtr) :
	width(wt), height(ht), texture(txtr) {}
