#pragma once

#include <utility>
#include <memory>
#include <iostream>
#include <cstdlib>

typedef std::pair<int,int> Range1d;
typedef std::pair<Range1d,Range1d> Range2d;

class AbstractRangeDecoder {
public:
	virtual Range2d decode2dRangeJump(const Range2d& start, std::vector<bool>::iterator controlsStart, std::vector<bool>::iterator controlsEnd) = 0;
	virtual unsigned numControls() = 0;
};

class AbstractSymmetricRangeDecoder : public AbstractRangeDecoder {
public:
	virtual Range1d decode1dRangeJump(const Range1d& start, std::vector<bool>::iterator controlsStart, std::vector<bool>::iterator controlsEnd) = 0;
	virtual unsigned numControlsPerDim() = 0;

	Range2d decode2dRangeJump(const Range2d& start, std::vector<bool>::iterator controlsStart, std::vector<bool>::iterator controlsEnd) override {
		if(controlsStart+2*numControlsPerDim() != controlsEnd) { // TODO: comment out this check once the classes are debugged
			std::cout << "AbstractSymmetricRangeDecoder: Wrong number of controls supplied to decode a 2d range" << std::endl;
			exit(EXIT_FAILURE);
		}
		Range1d xrange = decode1dRangeJump(start.first, controlsStart, controlsStart+numControlsPerDim());
		Range1d yrange = decode1dRangeJump(start.second, controlsStart+numControlsPerDim(), controlsEnd);
		return Range2d(xrange, yrange);
	};
	unsigned numControls() override {
		return 2*numControlsPerDim();
	};
};

class LinearBitScaleDecoder : public AbstractSymmetricRangeDecoder {
private:
	const unsigned frameSize;
	const unsigned outSize;
	const unsigned gradations;
	const unsigned controlsPerDim;
	const unsigned maxJumpSize;

public:
	LinearBitScaleDecoder(unsigned frameRangeSize, unsigned outRangeSize, unsigned numGradations) :
		frameSize(frameRangeSize), outSize(outRangeSize), gradations(numGradations),
		controlsPerDim(2*gradations), maxJumpSize(frameSize-outSize) {};

	Range1d decode1dRangeJump(const Range1d& start, std::vector<bool>::iterator controlsStart, std::vector<bool>::iterator controlsEnd) {
		if(controlsStart+controlsPerDim != controlsEnd) { // TODO: comment out this check once the classes are debugged
			std::cout << "LinearBitScaleDecoder: Wrong number of controls supplied to decode a 1d range" << std::endl;
			exit(EXIT_FAILURE);
		}
		int left = std::count(controlsStart, controlsStart+gradations, true);
		int right = std::count(controlsStart+gradations, controlsEnd, true);
		int shift = right - left;
		int jump = maxJumpSize*shift/gradations;

		int x0, x1;
		std::tie(x0, x1) = start;
		x0 += jump; x1 += jump;
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

	unsigned numControlsPerDim() override {
		return controlsPerDim;
	};
};

inline std::shared_ptr<AbstractRangeDecoder> constructRangeDecoder(unsigned type, unsigned gradations, unsigned frameSize, unsigned outSize) {
	// Directory of decoders by type:
	// 0 - LinearBitScale
	// everything else - not implemented
	switch(type) {
		case 0: return std::make_shared<LinearBitScaleDecoder>(frameSize, outSize, gradations);
		default: { std::cerr << "Range decoder type " << type << " not understood" << std::endl;
		           exit(EXIT_FAILURE);
		         }
	}
};
