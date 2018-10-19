#pragma once

#include <vector>

#include "AbstractMotors.h"
#include "../Brain/AbstractBrain.h"
#include "AbstractMentalImage.h"

class ImaginationMotors : public AbstractMotors {

private:
	std::shared_ptr<AbstractMentalImage> image;
	std::vector<double> currentBrainOutput;

public:
	ImaginationMotors(std::shared_ptr<AbstractMentalImage> img) : image(img) {
		currentBrainOutput.resize(numInputs());
	};
	void reset() override {}; // this actuator does not have any state by itself, so there's no need to reset anything
	void update(int timeStep, int visualize) override {
		for(int i=0; i<numInputs(); i++)
			currentBrainOutput[i] = brain->readOutput(i);
		image->updateWithInputs(currentBrainOutput);
	};
	int numInputs() override { return image->numInputs(); };
};
