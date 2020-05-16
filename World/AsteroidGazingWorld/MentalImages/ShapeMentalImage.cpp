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

double computeSharedEntropy(const map<pair<string,string>,unsigned>& jointCounts,
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
		patternLabelInfo += jp*log10(jp/(labelDistribution[label]*patternDistribution[pattern]));
	}
	return patternLabelInfo;
}

map<string,double> computeLabelConditionalEntropy(const map<pair<string,string>,unsigned>& jointCounts,
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
			lcEntropy[lcpair.first] -= patProbGivenLabel*log10(patProbGivenLabel);
		}
	}
	return lcEntropy;
}

double computeAverageLabelConditionalEntropy(const map<pair<string,string>,unsigned>& jointCounts,
                                          const map<string,unsigned>& labelCounts,
                                          unsigned numSamples) {
	map<string,double> labelConditionalEntropy = computeLabelConditionalEntropy(jointCounts, labelCounts);
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
                                   shared_ptr<AsteroidsDatasetParser> dsParserPtr)
	currentAsteroidNamePtr(curAstNamePtr),
	datasetParserPtr(dsParserPtr),
	numMatches(0),
	mVisualize(Global::modePL->get() == "visualize") {

	readLabelCache();
}

void ShapeMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	numMatches = 0;

	stateStrings.clear();
	labeledStateStrings.clear();

	labelCounts.clear();
	patternCounts.clear();
	jointCounts.clear();
	numSamples = 0;
}

void ShapeMentalImage::recordRunningScoresWithinState(shared_ptr<Organism> org, int stateTime, int statePeriod) {
	if(stateTime == statePeriod-1) {
		string curLabelString = labelCache(*currentAsteroidNamePtr);
		string curStateString = textureToHexStr(reinterpret_cast<Texture*>(brain->getDataForMotors()));
		if(curLabelString==curStateString)
			numMatches++;

		stateStrings.insert(curStateString);
		labeledStateStrings.insert(curStateString + curLabelString);

		incrementMapField(labelCounts, curLabelString);
		incrementMapField(patternCounts, curStateString);
		incrementMapField(jointCounts, make_pair(curLabelString, curStateString));
		numSamples++;
	}
}

void ShapeMentalImage::recordSampleScores(shared_ptr<Organism> org,
                                          shared_ptr<DataMap> sampleScoresMap,
                                          shared_ptr<DataMap> runningScoresMap,
                                          int evalTime,
                                          int visualize) {
	sampleScoresMap->append("numPatterns", static_cast<double>(stateStrings.size()));
	sampleScoresMap->append("numLabeledPatterns", static_cast<double>(labeledStateStrings.size()));
	sampleScoresMap->append("exactMatches", static_cast<double>(numMatches));
	sampleScoresMap->append("patternLabelInformation", computeSharedEntropy(jointCounts, patternCounts, labelCounts, numSamples));
	sampleScoresMap->append("averageLabelConditionalEntropy", computeAverageLabelConditionalEntropy(jointCounts, labelCounts, numSamples));

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
	org->dataMap.append("lostStates", sampleScoresMap->getAverage("lostStates"));
	org->dataMap.append("lostLabels", sampleScoresMap->getAverage("lostLabels"));
	org->dataMap.append("numPatterns", sampleScoresMap->getAverage("numPatterns"));
	org->dataMap.append("numLabeledPatterns", sampleScoresMap->getAverage("numLabeledPatterns"));

	org->dataMap.append("patternLabelInformation", sampleScoresMap->getAverage("patternLabelInformation"));
	org->dataMap.append("averageLabelConditionalEntropy", sampleScoresMap->getAverage("averageLabelConditionalEntropy"));
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
				throw invalid_argument("ShapeMentalImage::readLabelCache found a command with number of digits other than four");
			double curMagnitude = abs(perturbations[i][3]);
			if(curMagnitude>maxMagnitude) {
				maxMagniture = curMagnitude;
				maxMagnitudePos = i;
			}
		}

		const char digits[16] = {'0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};
		size_t thepat = 0;
		thepat |= (perturbations[maxMagnitudePos][0] > M_PI_2); // theta range is [0, pi]
		thepat <<= 1; thepat |= (perturbations[maxMagnitudePos][1] > M_PI); // phi [0, 2*pi]
		thepat <<= 1; thepat |= (perturbations[maxMagnitudePos][2] > 0.7); // radius [0.4, 1]
		thepat <<= 1; thepat |= (perturbations[maxMagnitudePos][3] > 0); // magnitude [-0.33, 0.33]

		labelCache(asteroedName, string(1, digits[thepat]));
	}
}

unsigned ShapeMentalImage::countExactMatches() {
	

}

double ShapeMentalImage::computeRepellingSharedEntropy() {

//	cout << "Raw pattern conditional label distributions:" << endl; printPatternConditionals(jointCounts); cout << endl;

	map<string,double> labelDistribution;
	for(const auto& lpair : labelCounts)
		labelDistribution[lpair.first] = static_cast<double>(lpair.second)/static_cast<double>(numSamples);
//	printMap(labelDistribution); cout << endl;

	map<pair<string,string>,double> repellingJoint;
	double normalizationConstant = 0.;
	for(const auto& jcpair : jointCounts) {
		string label, pattern;
		tie(label, pattern) = jcpair.first;

		incrementMapField(repellingJoint, jcpair.first, static_cast<double>(jcpair.second));
		normalizationConstant += static_cast<double>(jcpair.second);

		for(const auto& otherJCPair : jointCounts) {
			string otherLabel, otherPattern;
			tie(otherLabel, otherPattern) = otherJCPair.first;
			if(otherPattern==pattern && otherLabel==label) continue;

			double dist = static_cast<double>(hexStringHammingDistance(pattern, otherPattern));
			double crosstalk = static_cast<double>(jcpair.second) * leakBaseMultiplier * exp(-static_cast<double>(dist)/leakDecayRadius);
//			cout << "Crosstalk from (" << label << ", " << pattern << ") to (" << otherLabel << ", " << otherPattern << ") is " << crosstalk << " (added to " << label << ", " << otherPattern << ")" << endl;
			incrementMapField(repellingJoint, make_pair(label, otherPattern), crosstalk);
			normalizationConstant += crosstalk;
		}
	}

//	cout << "Leaky pattern conditional label distributions:" << endl; printPatternConditionals(repellingJoint); cout << endl;

//	cout << "Normalization const is " << scientific << normalizationConstant << " while num samples is " << numSamples << endl;
	for(auto& rjpair : repellingJoint)
		rjpair.second /= normalizationConstant;
//	printMap(repellingJoint); cout << endl;

	map<string,double> repellingPatterns;
	for(const auto& rjpair : repellingJoint)
		incrementMapField(repellingPatterns, rjpair.first.second, rjpair.second);
//	printMap(repellingPatterns); cout << endl;

	double patternLabelInfo = 0.;
	for(const auto& rjpair : repellingJoint) {
		string label, pattern;
		tie(label, pattern) = rjpair.first;
		patternLabelInfo += rjpair.second*log10(rjpair.second/(labelDistribution[label]*repellingPatterns[pattern]));
	}
	return patternLabelInfo;
}

double ShapeMentalImage::computeFastRepellingSharedEntropy() {
	using namespace std;

//	cout << "Raw pattern conditional label distributions:" << endl; printPatternConditionals(jointCounts); cout << endl;

	map<string,double> labelDistribution;
	for(const auto& lpair : labelCounts)
		labelDistribution[lpair.first] = static_cast<double>(lpair.second)/static_cast<double>(numSamples);

	neighborsdb.index(jointCounts);
	map<pair<string,string>,double> repellingJoint;
	double normalizationConstant = 0.;

	for(const auto& jcpair : jointCounts) {
		string label, pattern;
		tie(label, pattern) = jcpair.first;

		incrementMapField(repellingJoint, jcpair.first, static_cast<double>(jcpair.second));
		normalizationConstant += static_cast<double>(jcpair.second);

		for(auto neighborInfo : neighborsdb.getSomeNeighbors(pattern, numNeighbors+1)) {
			string otherLabel, otherPattern;
			unsigned otherCount;
			size_t otherDistance;
			tie(otherLabel, otherPattern, otherCount, otherDistance) = neighborInfo;
			if(otherPattern==pattern && otherLabel==label) continue;

			double dist = static_cast<double>(otherDistance);
			double crosstalk = static_cast<double>(otherCount) * leakBaseMultiplier * exp(-static_cast<double>(dist)/leakDecayRadius);
//			cout << "Crosstalk from (" << label << ", " << pattern << ") to (" << otherLabel << ", " << otherPattern << ") is " << crosstalk << " (added to " << label << ", " << otherPattern << ")" << endl;
			incrementMapField(repellingJoint, make_pair(otherLabel, pattern), crosstalk);
			normalizationConstant += crosstalk;
		}
	}

//	cout << "Leaky pattern conditional label distributions:" << endl; printPatternConditionals(repellingJoint); cout << endl;

	for(auto& rjpair : repellingJoint)
		rjpair.second /= normalizationConstant;

	map<string,double> repellingPatterns;
	for(const auto& rjpair : repellingJoint)
		incrementMapField(repellingPatterns, rjpair.first.second, rjpair.second);

	double patternLabelInfo = 0.;
	for(const auto& rjpair : repellingJoint) {
		string label, pattern;
		tie(label, pattern) = rjpair.first;
		patternLabelInfo += rjpair.second*log10(rjpair.second/(labelDistribution[label]*repellingPatterns[pattern]));
	}
	return patternLabelInfo;
}
