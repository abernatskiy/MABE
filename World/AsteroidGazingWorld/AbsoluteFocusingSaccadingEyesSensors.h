#pragma once

#include "../AbstractSensors.h"
#include "../../Brain/AbstractBrain.h"

class AbsoluteFocusingSaccadingEyesSensors : public AbstractSensors {

private:
	const unsigned resolution;
	const unsigned numSensors;
	std::string asteroidsDatasetPath;
	std::shared_ptr<std::string> currentAsteroidName;
	std::shared_ptr<AbstractBrain> brain;

	unsigned getNumSensors() {
		// yes I know this is suboptimal
		unsigned pow = 1;
		for(unsigned i=0; i<resolution; i++)
			pow *= 4;
		return pow;
	};

public:
	AbsoluteFocusingSaccadingEyesSensors(std::shared_ptr<std::string> curAstName,
	                                     std::string datasetPath,
	                                     std::shared_ptr<AbstractBrain> br,
	                                     unsigned res) :
		currentAsteroidName(curAstName), asteroidsDatasetPath(datasetPath), brain(br), resolution(res), numSensors(getNumSensors()) {};
	void reset(int visualize) override { AbstractSensors::reset(visualize); }; // sensors themselves are stateless
	void update(int visualize) override {
		// read the pictures here and write the outputs

		// for now, just write ones to the sensory inputs of the brain
		for(unsigned i=0; i<numSensors; i++)
			brain->setInput(i, 1.);

		AbstractSensors::update(visualize); // increment the clock
	};
	int numOutputs() override { return numSensors; };
};
