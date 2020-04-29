#pragma once

#include <vector>

#include "AbstractMotors.h"
#include "../Brain/AbstractBrain.h"
#include "AbstractMentalImage.h"

class ImaginationMotors : public AbstractMotors {

private:
	std::shared_ptr<AbstractMentalImage> image;
	std::vector<double> currentBrainOutput;
	unsigned offset;

public:
	ImaginationMotors(std::shared_ptr<AbstractMentalImage> img, unsigned inputOffset) :
		image(img),
		offset(inputOffset) {
		currentBrainOutput.resize(numInputs());
	};
	void attachToBrain(std::shared_ptr<AbstractBrain> br) override {
		AbstractMotors::attachToBrain(br);
		image->attachToBrain(br);
	};
	void reset(int visualize) override {
		// this actuator does not have any state by itself, so there's no need to reset anything besides the clock
		AbstractMotors::reset(visualize);
	};
	void update(int visualize) override {
		AbstractMotors::update(visualize);
		for(int i=0; i<numInputs(); i++)
			currentBrainOutput[i] = brain->readOutput(offset+i);
		image->updateWithInputs(currentBrainOutput);
	};
	int numInputs() override { return image->numInputs(); };
};
