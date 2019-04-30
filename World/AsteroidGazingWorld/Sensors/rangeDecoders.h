#pragma once

#include <utility>
#include <memory>
#include <iostream>
#include <cstdlib>

typedef std::pair<int,int> Range1d;
typedef std::pair<Range1d,Range1d> Range2d;

/********** Auxiliary funcs **********/

inline std::string range1dToStr(const Range1d& r) { return std::string("[") + std::to_string(r.first) + ", " + std::to_string(r.second) + ")"; };
inline std::string range2dToStr(const Range2d& r) { return std::string("X: ") + range1dToStr(r.first) + " Y: " + range1dToStr(r.second); };
inline void printBitRange(std::vector<bool>::iterator controlsStart, std::vector<bool>::iterator controlsEnd) { for(auto it=controlsStart; it!=controlsEnd; it++) std::cout << *it; };

inline unsigned ceilLog2(unsigned val) {
	unsigned upper = 1; unsigned bits = 0;
	while(upper < val) {
		upper *= 2;
		bits++;
	}
	return bits;
};

/********** Range decoder classes **********/

class AbstractRangeDecoder {
public:
	virtual Range2d decode2dRangeJump(const Range2d& start, std::vector<bool>::iterator controlsStart, std::vector<bool>::iterator controlsEnd) = 0;
	virtual unsigned numControls() = 0;
};

class ChrisCompactDecoder : public AbstractRangeDecoder {
private:
	const int frameSize;
	const int outSize;
	const int gradations;
	const int numberOfControls;
	const int maxJumpSize;
	const int gradationSize;

public:
	ChrisCompactDecoder(unsigned frameRangeSize, unsigned outRangeSize, unsigned numGradations, unsigned gradSize) :
		frameSize(frameRangeSize), outSize(outRangeSize), gradations(numGradations),
		numberOfControls(3+ceilLog2(gradations)), maxJumpSize(frameSize-outSize), gradationSize(gradSize) {};

	Range2d decode2dRangeJump(const Range2d& start, std::vector<bool>::iterator controlsStart, std::vector<bool>::iterator controlsEnd) override {
    // bitmasks for main directions
		// 0 0 0   1 1 0   1 0 1
		// 1   0   1   0   0   0
		// 1 1 1   1 0 0   1 0 1
		int rx, ry;
		if(*controlsStart) {
			if(*(controlsStart+1)) {
				rx = -1;
				ry = *(controlsStart+2) ? -1 : 0;
			}
			else {
				ry = -1;
				rx = *(controlsStart+2) ? 1 : 0;
			}
		}
		else {
			if(*(controlsStart+1)) {
				ry = 1;
				rx = *(controlsStart+2) ? -1 : 0;
			}
			else {
				rx = 1;
				ry = *(controlsStart+2) ? 1 : 0;
			}
		}

		unsigned ushift = 0;
		auto it = controlsStart+3;
		while(it != controlsEnd) {
			ushift <<= 1;
			ushift += static_cast<unsigned>(*it);
			it++;
		}

		Range1d xrange = shiftAndClip(start.first, rx*static_cast<int>(ushift));
		Range1d yrange = shiftAndClip(start.second, ry*static_cast<int>(ushift));
//		std::cout << "Decoded ";
//		printBitRange(controlsStart, controlsEnd);
//		std::cout << " into jump from " << range2dToStr(start) << " to " << range2dToStr(Range2d(xrange, yrange)) << std::endl;
		return Range2d(xrange, yrange);
	};

	unsigned numControls() override { return numberOfControls; };

private:
	Range1d shiftAndClip(const Range1d& start, int shift) {
		int x0, x1;
		std::tie(x0, x1) = start;
		x0 += shift*gradationSize;
		x1 += shift*gradationSize;
		if(x1 > frameSize) {
			x1 = frameSize;
			x0 = x1 - outSize;
		}
		if(x0 < 0) {
			x0 = 0;
			x1 = x0 + outSize;
		}
		return Range1d(x0, x1);
	};
};

class AbstractSymmetricRangeDecoder : public AbstractRangeDecoder {
public:
	virtual Range1d decode1dRangeJump(const Range1d& start, std::vector<bool>::iterator controlsStart, std::vector<bool>::iterator controlsEnd) = 0;
	virtual unsigned numControlsPerDim() = 0;

	Range2d decode2dRangeJump(const Range2d& start, std::vector<bool>::iterator controlsStart, std::vector<bool>::iterator controlsEnd) override {
//		if(controlsStart+2*numControlsPerDim() != controlsEnd) { // TODO: comment out this check once the classes are debugged
//			std::cout << "AbstractSymmetricRangeDecoder: Wrong number of controls supplied to decode a 2d range" << std::endl;
//			exit(EXIT_FAILURE);
//		}
		Range1d xrange = decode1dRangeJump(start.first, controlsStart, controlsStart+numControlsPerDim());
		Range1d yrange = decode1dRangeJump(start.second, controlsStart+numControlsPerDim(), controlsEnd);
//		std::cout << "Decoded ";
//		printBitRange(controlsStart, controlsEnd);
//		std::cout << " into " << range2dToStr(Range2d(xrange, yrange)) << std::endl;
		return Range2d(xrange, yrange);
	};
	unsigned numControls() override {
		return 2*numControlsPerDim();
	};
};

class LinearBitScaleDecoder : public AbstractSymmetricRangeDecoder {
private:
	const int frameSize;
	const int outSize;
	const int gradations;
	const int controlsPerDim;
	const int maxJumpSize;

public:
	LinearBitScaleDecoder(unsigned frameRangeSize, unsigned outRangeSize, unsigned numGradations) :
		frameSize(frameRangeSize), outSize(outRangeSize), gradations(numGradations),
		controlsPerDim(2*gradations), maxJumpSize(frameSize-outSize) {};

	Range1d decode1dRangeJump(const Range1d& start, std::vector<bool>::iterator controlsStart, std::vector<bool>::iterator controlsEnd) {
//		if(controlsStart+controlsPerDim != controlsEnd) { // TODO: comment out this check once the classes are debugged
//			std::cout << "LinearBitScaleDecoder: Wrong number of controls supplied to decode a 1d range" << std::endl;
//			exit(EXIT_FAILURE);
//		}
		int left = std::count(controlsStart, controlsStart+gradations, true);
		int right = std::count(controlsStart+gradations, controlsEnd, true);
		int shift = right - left;
		int jump = maxJumpSize*shift/gradations;
//		std::cout << "Right " << right << " left " << left << " shift " << shift << " jump " << jump << std::endl;

		int x0, x1;
		std::tie(x0, x1) = start;
		x0 += jump; x1 += jump;
//		std::cout << "Target coords befor clipping: " << x0 << " " << x1 << std::endl;
		if(x1 > frameSize) {
			x1 = frameSize;
			x0 = x1 - outSize;
		}
		if(x0 < 0) {
			x0 = 0;
			x1 = x0 + outSize;
		}
//		std::cout << "Decoded ";
//		printBitRange(controlsStart, controlsEnd);
//		std::cout << " into jump from " << range1dToStr(start) << " to " << range1dToStr(Range1d(x0, x1)) << std::endl;
		return Range1d(x0, x1);
	};

	unsigned numControlsPerDim() override {
		return controlsPerDim;
	};
};

class SignAndMagnitudeDecoder : public AbstractSymmetricRangeDecoder {
private:
	const int frameSize;
	const int outSize;
	const int gradations;
	const int controlsPerDim;
	const int maxJumpSize;

public:
	SignAndMagnitudeDecoder(unsigned frameRangeSize, unsigned outRangeSize, unsigned numGradations) :
		frameSize(frameRangeSize), outSize(outRangeSize), gradations(numGradations),
		controlsPerDim(ceilLog2(gradations)+1), maxJumpSize(frameSize-outSize) {};

	Range1d decode1dRangeJump(const Range1d& start, std::vector<bool>::iterator controlsStart, std::vector<bool>::iterator controlsEnd) {
		unsigned ushift = 0;
		auto it = controlsStart+1;
		while(it != controlsEnd) {
			ushift <<= 1;
			ushift += static_cast<unsigned>(*it);
			it++;
		}
		int shift = (*controlsStart ? 1 : -1) * static_cast<int>(ushift);
		int jump = maxJumpSize*shift/gradations;
//		std::cout << "Shift " << shift << " jump " << jump << std::endl;

		int x0, x1;
		std::tie(x0, x1) = start;
		x0 += jump; x1 += jump;
//		std::cout << "Target coords befor clipping: " << x0 << " " << x1 << std::endl;
		if(x1 > frameSize) {
			x1 = frameSize;
			x0 = x1 - outSize;
		}
		if(x0 < 0) {
			x0 = 0;
			x1 = x0 + outSize;
		}
//		std::cout << "Decoded ";
//		printBitRange(controlsStart, controlsEnd);
//		std::cout << " into jump from " << range1dToStr(start) << " to " << range1dToStr(Range1d(x0, x1)) << std::endl;
		return Range1d(x0, x1);
	};

	unsigned numControlsPerDim() override {
		return controlsPerDim;
	};
};

inline std::shared_ptr<AbstractRangeDecoder> constructRangeDecoder(unsigned type, unsigned gradations, unsigned frameSize, unsigned outSize) {
	// Directory of decoders by type:
	// 0 - LinearBitScale
	// 1 - Sign-and-magnitude
	// 2 - Chris's compact decoder
	// everything else - not implemented
	switch(type) {
		case 0: return std::make_shared<LinearBitScaleDecoder>(frameSize, outSize, gradations);
		case 1: return std::make_shared<SignAndMagnitudeDecoder>(frameSize, outSize, gradations);
		case 2: return std::make_shared<ChrisCompactDecoder>(frameSize, outSize, gradations, outSize);
		default: { std::cerr << "Range decoder type " << type << " not understood" << std::endl;
		           exit(EXIT_FAILURE);
		         }
	}
};
