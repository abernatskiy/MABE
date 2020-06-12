#include <fstream>
#include <cstdlib>
#include <cmath>
#include <limits>

#include "ShapeMentalImage.h"
#include "decoders.h"
#include "../../../Utilities/texture.h"

using namespace std;

/*****************************************/
/********** Auxiliary functions **********/
/*****************************************/

template<class KeyClass, typename NumType>
void incrementMapField(map<KeyClass,NumType>& mymap, const KeyClass& key, NumType theIncrement = 1) {
	if(mymap.find(key)==mymap.end())
		mymap[key] = theIncrement;
	else
		mymap[key] += theIncrement;
}

template<class NumType>
void printMap(const map<string,NumType>& mymap) {
	unsigned st = 0;
	for(const auto& mpair : mymap)
		cout << (st++==0?"":" ") << mpair.first << ":" << mpair.second;
	cout << endl;
}

template<class NumType>
void printMap(const map<pair<string,string>,NumType>& mymap) {
	unsigned st = 0;
	for(const auto& mpair : mymap)
		cout << (st++==0?"(":" (") << mpair.first.first << "," << mpair.first.second << "):" << mpair.second;
	cout << endl;
}

template<class NumType>
void printPatternConditionals(const map<pair<string,string>,NumType>& joint) {
	map<string,map<string,NumType>> patternConditionals;
	for(const auto& mpair : joint) {
		string pattern = mpair.first.second;
		auto itCurConditional = patternConditionals.find(pattern);
		if(itCurConditional==patternConditionals.end()) {
			patternConditionals[pattern] = {};
			itCurConditional = patternConditionals.find(pattern);
		}
		string label = mpair.first.first;
		auto itCurField = itCurConditional->second.find(label);
		if(itCurField==itCurConditional->second.end()) {
			itCurConditional->second.emplace(label, 0);
			itCurField = itCurConditional->second.find(label);
		}
		itCurField->second += mpair.second;
	}

	for(const auto& patpair : patternConditionals) {
		cout << patpair.first << ":";
		for(const auto& labpair : patpair.second)
			cout << " " << labpair.first << "-" << labpair.second;
		cout << endl;
	}
}

template<class NumType>
void printLabelConditionals(const map<pair<string,string>,NumType>& joint) {
	map<string,map<string,NumType>> patternConditionals;
	for(const auto& mpair : joint) {
		string pattern = mpair.first.first; // beware - this is a lazy transformation of the function above, labels are patterns and vice versa
		auto itCurConditional = patternConditionals.find(pattern);
		if(itCurConditional==patternConditionals.end()) {
			patternConditionals[pattern] = {};
			itCurConditional = patternConditionals.find(pattern);
		}
		string label = mpair.first.second; // beware - this is a lazy transformation of the function above, labels are patterns and vice versa
		auto itCurField = itCurConditional->second.find(label);
		if(itCurField==itCurConditional->second.end()) {
			itCurConditional->second.emplace(label, 0);
			itCurField = itCurConditional->second.find(label);
		}
		itCurField->second += mpair.second;
	}

	NumType totalMeasurements = 0;
	for(const auto& patpair : patternConditionals)
		for(const auto& labpair : patpair.second)
			totalMeasurements += labpair.second;

	cout.precision(2);
	cout << std::scientific;

	for(const auto& patpair : patternConditionals) {
		NumType patternMeasurements = 0;
		for(const auto& labpair : patpair.second)
			patternMeasurements += labpair.second;

		cout << patpair.first << " (w" << (static_cast<double>(patternMeasurements)/static_cast<double>(totalMeasurements)) << ") :";
		for(const auto& labpair : patpair.second)
			cout << " " << labpair.first << "-" << (static_cast<double>(labpair.second)/static_cast<double>(patternMeasurements));
		cout << endl;
	}
}

double computeSharedEntropyWeGonnaCelebrate(const map<pair<string,string>,unsigned>& jointCounts,
                            const map<string,unsigned>& patternCounts,
                            const map<string,unsigned>& labelCounts,
                            unsigned numSamples) {
	map<string,double> labelDistribution;
	for(const auto& lpair : labelCounts)
		labelDistribution[lpair.first] = static_cast<double>(lpair.second)/static_cast<double>(numSamples);
	map<string,double> patternDistribution;
	for(const auto& ppair : patternCounts)
		patternDistribution[ppair.first] = static_cast<double>(ppair.second)/static_cast<double>(numSamples);
	double patternLabelInfo = 0.;
	for(const auto& labpatpair : jointCounts) {
		string label, pattern;
		tie(label, pattern) = labpatpair.first;
		double jp = static_cast<double>(labpatpair.second)/static_cast<double>(numSamples);
		patternLabelInfo += jp*log2(jp/(labelDistribution[label]*patternDistribution[pattern]));
	}
	return patternLabelInfo;
}

map<string,double> computeLabelConditionalEntropyWeGonnaCelebrate(const map<pair<string,string>,unsigned>& jointCounts,
                                                            const map<string,unsigned>& labelCounts) {
	map<string,double> lcEntropy;
	for(const auto& lcpair : labelCounts) {
		map<string,unsigned> condPatternCounts;
		for(const auto& jcpair : jointCounts)
			if(jcpair.first.first==lcpair.first)
				incrementMapField(condPatternCounts, jcpair.first.second, jcpair.second);
		lcEntropy[lcpair.first] = 0.;
		for(const auto& cpcpair : condPatternCounts) {
			double patProbGivenLabel = static_cast<double>(cpcpair.second)/static_cast<double>(lcpair.second);
			lcEntropy[lcpair.first] -= patProbGivenLabel*log2(patProbGivenLabel);
		}
	}
	return lcEntropy;
}

double computeAverageLabelConditionalEntropyWeGonnaCelebrate(const map<pair<string,string>,unsigned>& jointCounts,
                                          const map<string,unsigned>& labelCounts,
                                          unsigned numSamples) {
	map<string,double> labelConditionalEntropy = computeLabelConditionalEntropyWeGonnaCelebrate(jointCounts, labelCounts);
	double fullLabelConditional = 0.;
	for(const auto& lcpair : labelCounts)
		fullLabelConditional += static_cast<double>(lcpair.second)*labelConditionalEntropy[lcpair.first]/static_cast<double>(numSamples);
	return fullLabelConditional;
}

/*
map<string,string> makeNaiveClassifier(const map<pair<string,string>,unsigned>& jointCounts,
                                       const map<string,unsigned>& patternCounts) {
	map<string,string> decipherer;
	long unsigned tieBreaker = 0;
	for(const auto& ppair : patternCounts) {
		string pattern = ppair.first;
		map<string,unsigned> condLabelCounts;
		for(const auto& jppair : jointCounts)
			if(jppair.first.second == pattern)
				incrementMapField(condLabelCounts, jppair.first.first, jppair.second);

//		cout << pattern << ":" << endl;
//		printMap(condLabelCounts);
//		cout << endl;

		unsigned maxCount = 0;
		string bestLabel;
		for(const auto& clcpair : condLabelCounts) {
			if(clcpair.second>maxCount) {
				maxCount = clcpair.second;
				bestLabel = clcpair.first;
			}
			else if(tieBreaker%2==0 && clcpair.second==maxCount) { // I alternate the direction of tie breaking to minimize biases while preserving determinism
				bestLabel = clcpair.first;
				tieBreaker++;
			}
		}
		decipherer[pattern] = bestLabel;

	}
	return decipherer;
}

void saveClassifier(const map<string,string>& patternsToLabels,
                    string filename) {
	ofstream classifierFile(filename, ofstream::out);
	for(const auto& plpair : patternsToLabels)
		classifierFile << plpair.first << " " << plpair.second << endl;
	classifierFile.close();
}

map<string,string> loadClassifier(string filename) {
	map<string,string> theClassifier;
	string pattern, label;
	ifstream classifierFile(filename);
	while(classifierFile >> pattern >> label)
		theClassifier[pattern] = label;
	return theClassifier;
}

string labelOfClosestNeighbor(string pattern, const map<string,string>& patternsToLabels) {
	// Hamming distance is used
	unsigned minDist = pattern.size()*4;
	string outPattern;
	for(const auto& plpair : patternsToLabels) {
		unsigned curDist = hexStringHammingDistance(pattern, plpair.first);
		if(curDist<=minDist) { // TODO: maybe add meaningful tie breaking here, idk
			minDist = curDist;
			outPattern = plpair.second;
		}
	}
	return outPattern;
}
*/

/********************************************************************/
/********** Public ShapeMentalImage class definitions **********/
/********************************************************************/

ShapeMentalImage::ShapeMentalImage(shared_ptr<string> curAstNamePtr,
                                   shared_ptr<AsteroidsDatasetParser> dsParserPtr) :
	currentAsteroidNamePtr(curAstNamePtr),
	datasetParserPtr(dsParserPtr),
	numMatches(0),
	numMatchedBits(0),
	numErasures(0),
	mVisualize(Global::modePL->get() == "visualize") {

	readLabelCache();
}

void ShapeMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	numMatches = 0;
	numMatchedBits = 0;

	stateStrings.clear();
	labeledStateStrings.clear();

	labelCounts.clear();
	patternCounts.clear();
	jointCounts.clear();
	numSamples = 0;

	numErasures = 0;
}

void ShapeMentalImage::recordRunningScoresWithinState(shared_ptr<Organism> org, int stateTime, int statePeriod) {
	if(stateTime == statePeriod-1) {
		string curLabelString = labelCache[*currentAsteroidNamePtr];

		Texture* outTexture = reinterpret_cast<Texture*>(brain->getDataForMotors());
		string curStateString = textureToHexStr(outTexture);
		unsigned outTextureSize = outTexture->num_elements();
		unsigned curMatchingBits = outTextureSize - hexStringHammingDistance(curStateString, curLabelString);
		numMatches += (outTextureSize==curMatchingBits);
		numMatchedBits += curMatchingBits;

//		cout << *currentAsteroidNamePtr << " : " << curLabelString << " " << readableRepr(*reinterpret_cast<Texture*>(brain->getDataForMotors())) << endl;

		stateStrings.insert(curStateString);
		labeledStateStrings.insert(curStateString + curLabelString);

		incrementMapField(labelCounts, curLabelString);
		incrementMapField(patternCounts, curStateString);
		incrementMapField(jointCounts, make_pair(curLabelString, curStateString));
		numSamples++;

		nlohmann::json brainStats = brain->getPostEvaluationStats();
		long stateErasures = stol(brainStats["erasures"].get<string>());
		numErasures += stateErasures;
	}
}

void ShapeMentalImage::recordSampleScores(shared_ptr<Organism> org,
                                          shared_ptr<DataMap> sampleScoresMap,
                                          shared_ptr<DataMap> runningScoresMap,
                                          int evalTime,
                                          int visualize) {

//	printLabelConditionals(jointCounts);
	sampleScoresMap->append("numPatterns", static_cast<double>(stateStrings.size()));
	sampleScoresMap->append("numLabeledPatterns", static_cast<double>(labeledStateStrings.size()));
	sampleScoresMap->append("exactMatches", static_cast<double>(numMatches));
	sampleScoresMap->append("matchedBits", static_cast<double>(numMatchedBits));
	sampleScoresMap->append("patternLabelInformation", computeSharedEntropyWeGonnaCelebrate(jointCounts, patternCounts, labelCounts, numSamples));
	sampleScoresMap->append("averageLabelConditionalEntropy", computeAverageLabelConditionalEntropyWeGonnaCelebrate(jointCounts, labelCounts, numSamples));
	sampleScoresMap->append("erasures", static_cast<double>(numErasures));

/*
	if(mVisualize) {
		map<string,string> currentDecipherer = makeNaiveClassifier(jointCounts, patternCounts);

//		saveClassifier(currentDecipherer, "decipherer.log");
//		map<string,string> decipherer = loadClassifier("decipherer.log");
		map<string,string> decipherer = currentDecipherer;
		long unsigned successfulTrials = 0;
		long unsigned totalTrials = 0;
		long unsigned unknownPattern = 0;
		for(const auto& jppair : jointCounts) {
			string label, pattern;
			tie(label, pattern) = jppair.first;
//			if(decipherer.find(pattern)==decipherer.end())
//				unknownPattern += jppair.second;
//			else if(decipherer[pattern]==label)
//				successfulTrials += jppair.second;
			if(labelOfClosestNeighbor(pattern, decipherer)==label)
				successfulTrials += jppair.second;
			totalTrials += jppair.second;
		}
		cout << "Out of " << totalTrials << " trials " << successfulTrials << " were successful and " << unknownPattern << " yielded unknown patterns" << endl;
	}
*/
}

void ShapeMentalImage::evaluateOrganism(shared_ptr<Organism> org, shared_ptr<DataMap> sampleScoresMap, int visualize) {
//	cout << "Writing evals for org " << org->ID << endl;
	org->dataMap.append("numPatterns", sampleScoresMap->getAverage("numPatterns"));
	org->dataMap.append("numLabeledPatterns", sampleScoresMap->getAverage("numLabeledPatterns"));
	org->dataMap.append("exactMatches", sampleScoresMap->getAverage("exactMatches"));
	org->dataMap.append("matchedBits", sampleScoresMap->getAverage("matchedBits"));
	org->dataMap.append("patternLabelInformation", sampleScoresMap->getAverage("patternLabelInformation"));
	org->dataMap.append("averageLabelConditionalEntropy", sampleScoresMap->getAverage("averageLabelConditionalEntropy"));
	org->dataMap.append("erasures", sampleScoresMap->getAverage("erasures"));
}

void* ShapeMentalImage::logTimeSeries(const string& label) {
//	ofstream statesLog(string("states_") + label + string(".log"));
//	for(const auto& state : stateTS)
//		stateLog << curLabelStr << bitRangeToHexStr(state.begin(), state.size()) << endl;
	return nullptr;
}

/******************************************************/
/***** Private DigitMentalImage class definitions *****/
/******************************************************/

void ShapeMentalImage::readLabelCache() {
	for(string asteroidName : datasetParserPtr->getAsteroidsNames()) {
		const vector<vector<double>>& perturbations = datasetParserPtr->cachingGetDescription(asteroidName);
		// the four columns are theta, phi, radius, magnitude
		double maxMagnitude = -1.*numeric_limits<double>::infinity();
		size_t maxMagnitudePos;
		for(size_t i=0; i<perturbations.size(); i++) {
			if(perturbations[i].size()!=4)
				throw invalid_argument("ShapeMentalImage::readLabelCache found a command with number of digits other than four for asteroid " + asteroidName);
			double curMagnitude = abs(perturbations[i][3]);
			if(curMagnitude>maxMagnitude) {
				maxMagnitude = curMagnitude;
				maxMagnitudePos = i;
			}
		}

		const char digits[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
		size_t thepat = 0;
		thepat |= (perturbations[maxMagnitudePos][0] > M_PI_2); // theta range is [0, pi]
		thepat <<= 1; thepat |= (perturbations[maxMagnitudePos][1] > M_PI); // phi [0, 2*pi]
		thepat <<= 1; thepat |= (perturbations[maxMagnitudePos][2] > 0.7); // radius [0.4, 1]
		thepat <<= 1; thepat |= (perturbations[maxMagnitudePos][3] > 0); // magnitude [-0.33, 0.33]

		labelCache.emplace(asteroidName, string(1, digits[thepat]));
	}
}
