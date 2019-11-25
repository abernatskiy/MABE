#include "DistancesMentalImage.h"
#include "decoders.h"

#include <fstream>
#include <cstdlib>
#include <cmath>

/*****************************************/
/********** Auxiliary functions **********/
/*****************************************/

template<class KeyClass, typename NumType>
void incrementMapField(std::map<KeyClass,NumType>& mymap, const KeyClass& key, NumType theIncrement = 1) {
	if(mymap.find(key)==mymap.end())
		mymap[key] = theIncrement;
	else
		mymap[key] += theIncrement;
}

/********************************************************************/
/********** Public DistancesMentalImage class definitions **********/
/********************************************************************/

DistancesMentalImage::DistancesMentalImage(std::shared_ptr<std::string> curAstNamePtr,
                                             std::shared_ptr<AsteroidsDatasetParser> dsParserPtr,
                                             std::shared_ptr<AbstractSensors> sPtr,
                                             unsigned nBits):
	currentAsteroidNamePtr(curAstNamePtr),
	datasetParserPtr(dsParserPtr),
	sensorsPtr(sPtr),
	mVisualize(Global::modePL->get() == "visualize"),
	numBits(nBits) {}

void DistancesMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	stateStrings.clear();
	labelStrings.clear();
	//labeledStateStrings.clear();
	sensorActivityStateScores.clear();
}

void DistancesMentalImage::resetAfterWorldStateChange(int visualize) { // called after each discrete world state change
}

void DistancesMentalImage::updateWithInputs(std::vector<double> inputs) {
	curStateString = bitRangeToHexStr(inputs.begin(), inputs.size());
}

void DistancesMentalImage::recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int statePeriod) {
	if(stateTime == 0)
		readLabel();

	// Debug "throws"
	if(curStateString.empty()) {
		std::cerr << "Mental image evaluator got an empty state string, exiting" << std::endl;
		exit(EXIT_FAILURE);
	}

	if(stateTime == statePeriod-1) {
		stateStrings.push_back(curStateString);
		labelStrings.push_back(curLabelString);
		//labeledStateStrings.push_back(curStateString + curLabelString);

		//incrementMapField(labelCounts, curLabelString);
		//incrementMapField(patternCounts, curStateString);
		//incrementMapField(jointCounts, std::make_pair(curLabelString, curStateString));
		//numSamples++;

		sensorActivityStateScores.push_back(static_cast<double>(sensorsPtr->numSaccades())/static_cast<double>(statePeriod));
	}
}

void DistancesMentalImage::recordRunningScores(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {}

void DistancesMentalImage::recordSampleScores(std::shared_ptr<Organism> org,
                                              std::shared_ptr<DataMap> sampleScoresMap,
                                              std::shared_ptr<DataMap> runningScoresMap,
                                              int evalTime,
                                              int visualize) {
	double totSensoryActivity = 0;
	for(double sa : sensorActivityStateScores)
		totSensoryActivity += sa;
	totSensoryActivity /= static_cast<double>(sensorActivityStateScores.size());
	sampleScoresMap->append("sensorActivity", totSensoryActivity);

	updateDistanceStats();
	sampleScoresMap->append("totalCrossLabelDistance", totalCrossLabelDistance);
	sampleScoresMap->append("totalIntraLabelDistance", totalIntraLabelDistance);

//	sampleScoresMap->append("patternLabelInformation", computeSharedEntropy(jointCounts, patternCounts, labelCounts, numSamples));

}

void DistancesMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
//	std::cout << "Writing evals for org " << org->ID << std::endl;
	org->dataMap.append("totalCrossLabelDistance", sampleScoresMap->getAverage("totalCrossLabelDistance"));
	org->dataMap.append("totalIntraLabelDistance", sampleScoresMap->getAverage("totalIntraLabelDistance"));
	double sensorActivity = sampleScoresMap->getAverage("sensorActivity");
	unsigned tieredSensorActivity = static_cast<unsigned>(sensorActivity*10);
	org->dataMap.append("sensorActivity", sensorActivity);
	org->dataMap.append("tieredSensorActivity", static_cast<double>(tieredSensorActivity));
}

int DistancesMentalImage::numInputs() { return numBits; }

void* DistancesMentalImage::logTimeSeries(const std::string& label) {
//	std::ofstream statesLog(std::string("states_") + label + std::string(".log"));
////	for(const auto& state : stateTS)
////		stateLog << curLabelStr << bitRangeToHexStr(state.begin(), state.size()) << std::endl;
//	statesLog << "not implemented" << std::endl;
	return nullptr;
}

/***** Private DigitMentalImage class definitions *****/

void DistancesMentalImage::readLabel() {
	const std::vector<std::vector<unsigned>>& commands = datasetParserPtr->cachingGetDescription(*currentAsteroidNamePtr);
	curLabelString = std::to_string(commands[0][0]); // OK for ten digits; for higher number of labels should be fine, too, if less readable
}

void DistancesMentalImage::updateDistanceStats() {
	totalCrossLabelDistance = 0.;
	totalIntraLabelDistance = 0.;
	unsigned numStates = stateStrings.size();
	for(unsigned i=0; i<numStates; i++) {
		for(unsigned j=0; j<numStates; j++) {
			if(i==j)
				continue;
			if(labelStrings[i]==labelStrings[j])
				totalIntraLabelDistance += hexStringHammingDistance(stateStrings[i], stateStrings[j]);
			else
				totalCrossLabelDistance += hexStringHammingDistance(stateStrings[i], stateStrings[j]);
		}
	}
}
