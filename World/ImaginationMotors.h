#pragma once

#include <vector>

#include "AbstractMotors.h"
#include "../Brain/AbstractBrain.h"
#include "AbstractMentalImage.h"

class ImaginationMotors : public AbstractMotors {

private:
	std::shared_ptr<AbstractMentalImage> image;
	std::shared_ptr<AbstractBrain> brain;
	std::vector<double> brainOutput;

public:
	ImaginationMotors(std::shared_ptr<AbstractMentalImage> img) : image(img) { brainOutput.resize(numInputs()); };
	void reset() override {}; // this actuator does not have any state by itself, so there's no need to reset anything
	void attachToBrain(std::shared_ptr<AbstractBrain> br) override { brain=br; };
	void update(int timeStep, int visualize) override {
		for(int i=0; i<numInputs(); i++)
			brainOutput[i] = brain->readOutput(i);
		image->updateWithInputs(brainOutput);
	};
	int numInputs() { return image->numInputs(); };
};
