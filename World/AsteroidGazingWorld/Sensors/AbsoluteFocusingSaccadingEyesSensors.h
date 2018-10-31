#pragma once

#include "../../AbstractSensors.h"
#include "../../../Brain/AbstractBrain.h"

class AbsoluteFocusingSaccadingEyesSensors : public AbstractSensors {

private:
	const unsigned resolution;
	const unsigned numSensors;
	std::string asteroidsDatasetPath;
	std::shared_ptr<std::string> currentAsteroidName;

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
	                                     unsigned res) :
		currentAsteroidName(curAstName), asteroidsDatasetPath(datasetPath), resolution(res), numSensors(getNumSensors()) {};

	void reset(int visualize) override { AbstractSensors::reset(visualize); }; // sensors themselves are stateless
	int numOutputs() override { return numSensors; };

	void update(int visualize) override;
};
