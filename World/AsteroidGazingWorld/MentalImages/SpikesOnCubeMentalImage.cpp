#include "SpikesOnCubeMentalImage.h"
#include "decoders.h"

#include <algorithm>
#include <fstream>
#include <sstream>

/***** Public SpikesOnCubeMentalImage class definitions *****/

SpikesOnCubeMentalImage::SpikesOnCubeMentalImage(std::shared_ptr<std::string> curAstNamePtr,
	                                               std::shared_ptr<AsteroidsDatasetParser> dsParserPtr) :
		currentAsteroidNamePtr(curAstNamePtr),
		datasetParserPtr(dsParserPtr)
//		bitsForFace(),
//		bitsForCoordinate()
{}

void SpikesOnCubeMentalImage::reset(int visualize) { // called in the beginning of each evaluation cycle
	stateScores.clear();
	currentCommands.clear();
	// originalCommands are taken care of in readOriginalCommands()
}

void SpikesOnCubeMentalImage::resetAfterWorldStateChange(int visualize) { // called after each discrete world state change
	currentCommands.clear();
	// originalCommands are taken care of in readOriginalCommands()
}

void SpikesOnCubeMentalImage::updateWithInputs(std::vector<double> inputs) {
	unsigned face, i, j;
	auto it = inputs.begin();

	face = decodeUInt(it, it+bitsForFace);
	it += bitsForFace;

	i = decodeUInt(it, it+bitsForCoordinate);
	it += bitsForCoordinate;

	j = decodeUInt(it, it+bitsForCoordinate);

	currentCommands.insert(std::make_tuple(face,i,j));
}

void SpikesOnCubeMentalImage::recordRunningScoresWithinState(int stateTime, int statePeriod) {
	if(stateTime == statePeriod-1) {
		readOriginalCommands();
		unsigned numCorrectCommands = 0;
		for(auto it=originalCommands.begin(); it!=originalCommands.end(); it++) {
			if(currentCommands.count(*it) != 0)
				numCorrectCommands++;
		}
		stateScores.push_back(numCorrectCommands);
	}
}

void SpikesOnCubeMentalImage::recordRunningScores(std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {}

void SpikesOnCubeMentalImage::recordSampleScores(std::shared_ptr<DataMap> sampleScoresMap, std::shared_ptr<DataMap> runningScoresMap, int evalTime, int visualize) {
	sampleScoresMap->append("score", static_cast<double>(std::accumulate(stateScores.begin(), stateScores.end(), 0)));
}

void SpikesOnCubeMentalImage::evaluateOrganism(std::shared_ptr<Organism> org, std::shared_ptr<DataMap> sampleScoresMap, int visualize) {
	double score = sampleScoresMap->getAverage("score");
	if(visualize) std::cout << "Assigning score of " << score << std::endl;
	org->dataMap.append("score", score);
}

int SpikesOnCubeMentalImage::numInputs() {
	return bitsForFace + 2*bitsForCoordinate;
}

/***** Private SpikesOnCubeMentalImage class definitions *****/

void SpikesOnCubeMentalImage::readOriginalCommands() {
	originalCommands.clear();
	std::string commandsFilePath = datasetParserPtr->getDescriptionPath(*currentAsteroidNamePtr);
	std::ifstream commandsFstream(commandsFilePath);
	std::string cline;
	while( std::getline(commandsFstream, cline) ) {
		unsigned face, i, j;
		std::stringstream cstream(cline);
		cstream >> face >> i >> j; // TODO: edit to match Lindsey commands format
		originalCommands.insert(std::make_tuple(face,i,j));
	}
	commandsFstream.close();
}
