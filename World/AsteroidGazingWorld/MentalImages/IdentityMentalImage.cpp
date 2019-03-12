#include <cstdlib>

#include "IdentityMentalImage.h"

IdentityMentalImage::IdentityMentalImage(std::shared_ptr<AbsoluteFocusingSaccadingEyesSensors> pointerToSensors) :
	sensorsPtr(pointerToSensors),
	justReset(true),
	sensoryChannels(sensorsPtr->numOutputs()),
	hits(0) {

	if(sensorsPtr->numInputs() != 0) {
		std::cerr << "This mental image is not suitable for working with active perception, and sensors seem to have " << sensorsPtr->numInputs() << " inputs" << std::endl;
		exit(EXIT_FAILURE);
	}
}

void IdentityMentalImage::updateWithInputs(std::vector<double> inputs) {
	hits = 0;
	const auto& lastPercept = sensorsPtr->getLastPercept();
	if(lastPercept.size() != sensoryChannels) {
		std::cerr << "Saved percept has wrong number of channels (is " << lastPercept.size() << ", should be " << sensoryChannels << ")" << std::endl;
		exit(EXIT_FAILURE);
	}
	if(inputs.size() != sensoryChannels) {
		std::cerr << "Mental image input has wrong number of channels (is " << inputs.size() << ", should be " << sensoryChannels << ")" << std::endl;
		exit(EXIT_FAILURE);
	}

	for(unsigned i=0; i<sensoryChannels; i++)
		if(lastPercept[i] == (inputs[i]>0))
			hits++;
}

void IdentityMentalImage::recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int statePeriod) {
	stateRunningScores.push_back(static_cast<double>(hits)/static_cast<double>(sensoryChannels));
	if(stateTime == statePeriod-1)
		stateScores.push_back(std::accumulate(stateRunningScores.begin(), stateRunningScores.end(), 0.)/static_cast<double>(stateRunningScores.size()));
}

void IdentityMentalImage::recordSampleScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {
	sampleScoresMap->append("score", std::accumulate(stateScores.begin(), stateScores.end(), 0.)/static_cast<double>(stateScores.size()));
}

void IdentityMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
	org->dataMap.append("score", sampleScoresMap->getAverage("score"));
}
