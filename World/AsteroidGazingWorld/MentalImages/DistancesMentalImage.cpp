#include "DistancesMentalImage.h"
#include "decoders.h"
#include "../../../Brain/LayeredBrain/topology.h"

#include <fstream>
#include <cstdlib>
#include <cmath>
#include <algorithm>

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

template<class NumType>
void printMap(const std::map<std::string,NumType>& mymap) {
	unsigned st = 0;
	for(const auto& mpair : mymap)
		std::cout << (st++==0?"":" ") << mpair.first << ":" << mpair.second;
	std::cout << std::endl;
}

template<class NumType>
void printMap(const std::map<std::pair<std::string,std::string>,NumType>& mymap) {
	unsigned st = 0;
	for(const auto& mpair : mymap)
		std::cout << (st++==0?"(":" (") << mpair.first.first << "," << mpair.first.second << "):" << mpair.second;
	std::cout << std::endl;
}

template<class NumType>
void printPatternConditionals(const std::map<std::pair<std::string,std::string>,NumType>& joint) {
	std::map<std::string,std::map<std::string,NumType>> patternConditionals;
	for(const auto& mpair : joint) {
		std::string pattern = mpair.first.second;
		auto itCurConditional = patternConditionals.find(pattern);
		if(itCurConditional==patternConditionals.end()) {
			patternConditionals[pattern] = {};
			itCurConditional = patternConditionals.find(pattern);
		}
		std::string label = mpair.first.first;
		auto itCurField = itCurConditional->second.find(label);
		if(itCurField==itCurConditional->second.end()) {
			itCurConditional->second.emplace(label, 0);
			itCurField = itCurConditional->second.find(label);
		}
		itCurField->second += mpair.second;
	}

	for(const auto& patpair : patternConditionals) {
		std::cout << patpair.first << ":";
		for(const auto& labpair : patpair.second)
			std::cout << " " << labpair.first << "-" << labpair.second;
		std::cout << std::endl;
	}
}

double computeSharedEntropyOneMoreTime(const std::map<std::pair<std::string,std::string>,unsigned>& jointCounts,
                            const std::map<std::string,unsigned>& patternCounts,
                            const std::map<std::string,unsigned>& labelCounts,
                            unsigned numSamples) {
	std::map<std::string,double> labelDistribution;
	for(const auto& lpair : labelCounts)
		labelDistribution[lpair.first] = static_cast<double>(lpair.second)/static_cast<double>(numSamples);
//	printMap(labelDistribution); std::cout << std::endl;
	std::map<std::string,double> patternDistribution;
	for(const auto& ppair : patternCounts)
		patternDistribution[ppair.first] = static_cast<double>(ppair.second)/static_cast<double>(numSamples);
//	printMap(patternDistribution); std::cout << std::endl;
//	printMap(jointCounts); std::cout << std::endl;
	double patternLabelInfo = 0.;
	for(const auto& labpatpair : jointCounts) {
		std::string label, pattern;
		std::tie(label, pattern) = labpatpair.first;
		double jp = static_cast<double>(labpatpair.second)/static_cast<double>(numSamples);
		patternLabelInfo += jp*log10(jp/(labelDistribution[label]*patternDistribution[pattern]));
	}
	return patternLabelInfo;
}

std::map<std::string,double> computeLabelConditionalEntropyOneMoreTime(const std::map<std::pair<std::string,std::string>,unsigned>& jointCounts,
                                                            const std::map<std::string,unsigned>& labelCounts) {
	std::map<std::string,double> lcEntropy;
	for(const auto& lcpair : labelCounts) {
		std::map<std::string,unsigned> condPatternCounts;
		for(const auto& jcpair : jointCounts)
			if(jcpair.first.first==lcpair.first)
				incrementMapField(condPatternCounts, jcpair.first.second, jcpair.second);
		lcEntropy[lcpair.first] = 0.;
		for(const auto& cpcpair : condPatternCounts) {
			double patProbGivenLabel = static_cast<double>(cpcpair.second)/static_cast<double>(lcpair.second);
			lcEntropy[lcpair.first] -= patProbGivenLabel*log10(patProbGivenLabel);
		}
	}
	return lcEntropy;
}

double computeAverageLabelConditionalEntropyOneMoreTime(const std::map<std::pair<std::string,std::string>,unsigned>& jointCounts,
                                          const std::map<std::string,unsigned>& labelCounts,
                                          unsigned numSamples) {
	std::map<std::string,double> labelConditionalEntropy = computeLabelConditionalEntropyOneMoreTime(jointCounts, labelCounts);
	double fullLabelConditional = 0.;
	for(const auto& lcpair : labelCounts)
		fullLabelConditional += static_cast<double>(lcpair.second)*labelConditionalEntropy[lcpair.first]/static_cast<double>(numSamples);
	return fullLabelConditional;
}

std::vector<std::pair<unsigned,unsigned>> layerByLayerRanges() {
	std::vector<std::pair<unsigned,unsigned>> infoRanges;
	unsigned curpos = 0;
	for(auto js : BRAIN_COMPONENT_JUNCTION_SIZES) {
		infoRanges.push_back(std::make_pair(curpos, curpos+js));
		curpos += js;
	}

  // Ranges for the phi-like info
//	for(unsigned rstart=0; rstart<124; rstart+=4)
//		infoRanges.push_back(std::make_pair(rstart, rstart+4));
	return infoRanges;
}

unsigned totalInputs() {
	unsigned out = 0;
	for(auto js : BRAIN_COMPONENT_JUNCTION_SIZES)
		out += js;
	return out;
}

/********************************************************************/
/********** Public DistancesMentalImage class definitions **********/
/********************************************************************/

DistancesMentalImage::DistancesMentalImage(std::shared_ptr<std::string> curAstNamePtr,
                                           std::shared_ptr<AsteroidsDatasetParser> dsParserPtr,
                                           std::shared_ptr<AbstractSensors> sPtr,
                                           bool overwriteEvals):
	currentAsteroidNamePtr(curAstNamePtr),
	datasetParserPtr(dsParserPtr),
	sensorsPtr(sPtr),
	infoRanges(layerByLayerRanges()),
	numSamples(0),
	mVisualize(Global::modePL->get() == "visualize"),
	numBits(totalInputs()),
	overwriteEvaluations(overwriteEvals) {

	// std::cout << "Total inputs: " << numBits << std::endl;
	// std::cout << "Information processing ranges:"; for(auto ipr : infoRanges) { unsigned st, en; std::tie(st, en) = ipr; std::cout << " (" << st << "," << en << ")"; }; std::cout << std::endl;

	for(unsigned iri=0; iri<infoRanges.size(); iri++) {
		rangesPatternCounts.push_back({});
		rangesJointCounts.push_back({});
	}
}

void DistancesMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	//stateStrings.clear();
	//labelStrings.clear();
	//labeledStateStrings.clear();
	sensorActivityStateScores.clear();
	labelCounts.clear();
	for(auto& rpc : rangesPatternCounts)
		rpc.clear();
	for(auto& rjc : rangesJointCounts)
		rjc.clear();
	numSamples = 0;
}

void DistancesMentalImage::resetAfterWorldStateChange(int visualize) { // called after each discrete world state change
}

void DistancesMentalImage::updateWithInputs(std::vector<double> inputs) {
	curBits = inputs;
	//curStateString = bitRangeToHexStr(inputs.begin(), inputs.size());
}

void DistancesMentalImage::recordRunningScoresWithinState(std::shared_ptr<Organism> org, int stateTime, int statePeriod) {
	if(stateTime == 0)
		readLabel();

	if(stateTime == statePeriod-1) {
		//stateStrings.push_back(curStateString);
		//labelStrings.push_back(curLabelString);
		//labeledStateStrings.push_back(curStateString + curLabelString);

		for(unsigned iri=0; iri<infoRanges.size(); iri++) {
			std::string rangeSubstr = bitRangeToHexStr(curBits.begin()+infoRanges[iri].first, infoRanges[iri].second-infoRanges[iri].first);
			incrementMapField(rangesPatternCounts[iri], rangeSubstr);
			incrementMapField(rangesJointCounts[iri], std::make_pair(curLabelString, rangeSubstr)); // note that the convention is inverted compared to CompressedMentalImage
		}
		incrementMapField(labelCounts, curLabelString);
		numSamples++;

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

	for(unsigned iri=0; iri<infoRanges.size(); iri++) {
		std::string infoName = "plInfo_range" + std::to_string(iri);
		double info = computeSharedEntropyOneMoreTime(rangesJointCounts.at(iri), rangesPatternCounts.at(iri), labelCounts, numSamples);
		sampleScoresMap->append(infoName, info);

		std::string entroName = "lcpe_range" + std::to_string(iri);
		//std::cout << "Computing lcpe for layer " << iri << std::endl;
		//printPatternConditionals(rangesJointCounts.at(iri));
		double entro = computeAverageLabelConditionalEntropyOneMoreTime(rangesJointCounts.at(iri), labelCounts, numSamples);
		sampleScoresMap->append(entroName, entro);
	}

//	sampleScoresMap->append("patternLabelInformation", computeSharedEntropy(jointCounts, patternCounts, labelCounts, numSamples));

}

void DistancesMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
//	std::cout << "Writing evals for org " << org->ID << std::endl;
	updateOrgDatamap(org, "totalCrossLabelDistance", sampleScoresMap->getAverage("totalCrossLabelDistance"));
	updateOrgDatamap(org, "totalIntraLabelDistance", sampleScoresMap->getAverage("totalIntraLabelDistance"));
	double irs = 0.;
	double ers = 0.;
	for(unsigned iri=0; iri<infoRanges.size(); iri++) {
		std::string infoName = "plInfo_range" + std::to_string(iri);
		updateOrgDatamap(org, infoName, sampleScoresMap->getAverage(infoName));
		irs += sampleScoresMap->getAverage(infoName);

		std::string entroName = "lcpe_range" + std::to_string(iri);
		updateOrgDatamap(org, entroName, sampleScoresMap->getAverage(entroName));
		ers += sampleScoresMap->getAverage(entroName);
	}
	updateOrgDatamap(org, "plInfo_allRanges", irs);
	updateOrgDatamap(org, "lcpe_allRanges", ers);
	double sensorActivity = sampleScoresMap->getAverage("sensorActivity");
	unsigned tieredSensorActivity = static_cast<unsigned>(sensorActivity*10);
	updateOrgDatamap(org, "sensorActivity", sensorActivity);
	updateOrgDatamap(org, "tieredSensorActivity", static_cast<double>(tieredSensorActivity));
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
/*
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
*/
}

void DistancesMentalImage::updateOrgDatamap(std::shared_ptr<Organism> org, std::string entryName, double entryValue) {
	if(overwriteEvaluations)
		org->dataMap.set(entryName, std::vector<double>({entryValue}));
	else
		org->dataMap.append(entryName, entryValue);
}
