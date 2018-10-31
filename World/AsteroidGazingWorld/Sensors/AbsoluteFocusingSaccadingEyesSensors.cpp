#include "AbsoluteFocusingSaccadingEyesSensors.h"

//	AbsoluteFocusingSaccadingEyesSensors(std::shared_ptr<std::string> curAstName,
//	                                     std::string datasetPath,
//	                                     unsigned res) :
//		currentAsteroidName(curAstName), asteroidsDatasetPath(datasetPath), resolution(res), numSensors(getNumSensors()) {};

void AbsoluteFocusingSaccadingEyesSensors::update(int visualize) {
		// read the pictures here and write the outputs

		// for now, just write ones to the sensory inputs of the brain
		for(unsigned i=0; i<numSensors; i++)
			brain->setInput(i, 1.);

		AbstractSensors::update(visualize); // increment the clock
}
