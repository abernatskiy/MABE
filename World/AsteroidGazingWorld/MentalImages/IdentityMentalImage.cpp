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
	hits = sensoryChannels;
}

void IdentityMentalImage::recordRunningScoresWithinState(int stateTime, int statePeriod) {
	stateRunningScores.push_back(static_cast<double>(hits)/static_cast<double>(sensoryChannels));
	if(stateTime == statePeriod-1)
		stateScores.push_back(std::accumulate(stateRunningScores.begin(), stateRunningScores.end(), 0.)/static_cast<double>(stateRunningScores.size()));
}

void IdentityMentalImage::recordSampleScores(std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {
	sampleScoresMap->append("score", std::accumulate(stateScores.begin(), stateScores.end(), 0.)/static_cast<double>(stateScores.size()));
}

void IdentityMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
	org->dataMap.append("score", sampleScoresMap->getAverage("score"));
}
