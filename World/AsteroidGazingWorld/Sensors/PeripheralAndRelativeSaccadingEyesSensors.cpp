#include "PeripheralAndRelativeSaccadingEyesSensors.h"

using namespace std;

PeripheralAndRelativeSaccadingEyesSensors::PeripheralAndRelativeSaccadingEyesSensors(shared_ptr<string> curAstName,
                                                                                     shared_ptr<AsteroidsDatasetParser> dsParser,
                                                                                     unsigned frRes, unsigned periFOVRes, unsigned fovRes,
                                                                                     unsigned jType, unsigned jGradations) :
	currentAsteroidName(curAstName), datasetParser(dsParser),
	frameRes(frRes), peripheralFOVRes(periFOVRes), foveaRes(fovRes),
	jumpType(jType), jumpGradations(jGradations),
	rangeDecoder(constructRangeDecoder(jType, jGradations, frRes, fovRes)),
	foveaPositionControls(rangeDecoder->numControls()),
	numSensors(periFOVRes*periFOVRes+fovRes*fovRes),
	numMotors(rangeDecoder->numControls()),
	foveaPosition(pair<int,int>(0, fovRes), pair<int,int>(0, fovRes)) {

	// Caching asteroid snapshots
	set<string> asteroidNames = datasetParser->getAsteroidsNames();
	for(const string& an : asteroidNames) {
		map<string,set<unsigned>> parameterValuesSets = datasetParser->getAllParameterValues(an);
		for(const auto& pvs : parameterValuesSets) {
			if(pvs.second.size()!=1) {
				cerr << "PeripheralAndRelativeSaccadingEyesSensors: Only asteroids with single available snapshot are supported at this moment" << endl;
				exit(EXIT_FAILURE);
			}
		}
		unsigned condition = *(parameterValuesSets["condition"].begin());
		unsigned distance = *(parameterValuesSets["distance"].begin());
		unsigned phase = *(parameterValuesSets["phase"].begin());
		string snapshotPath = datasetParser->getPicturePath(an, condition, distance, phase);
		asteroidSnapshots.emplace(pair<AsteroidViewParameters,AsteroidSnapshot>(make_tuple(an),
		                                                                        AsteroidSnapshot(snapshotPath, baseThreshold)));
	}

	analyzeDataset();
}

void PeripheralAndRelativeSaccadingEyesSensors::update(int visualize) {
	std::vector<bool> controls(numMotors);
	for(unsigned i=0; i<numMotors; i++)
		controls[i] = brain->readOutput(i) > 0.5;

	AsteroidSnapshot& astSnap = asteroidSnapshots.at(make_tuple(*currentAsteroidName));
	savedPercept.clear();

	const AsteroidSnapshot& peripheralView = astSnap.cachingResampleArea(0, 0, astSnap.width, astSnap.height, peripheralFOVRes, peripheralFOVRes, baseThreshold);
	for(unsigned i=0; i<peripheralFOVRes; i++)
		for(unsigned j=0; j<peripheralFOVRes; j++)
			savedPercept.push_back(peripheralView.getBinary(i, j));

	foveaPosition = rangeDecoder->decode2dRangeJump(foveaPosition, controls.begin(), controls.end());
	const AsteroidSnapshot& fovealView = astSnap.cachingResampleArea(foveaPosition.first.first, foveaPosition.second.first,
	                                                                foveaPosition.first.second, foveaPosition.second.second,
	                                                                foveaRes, foveaRes, baseThreshold);
	for(unsigned i=0; i<foveaRes; i++)
		for(unsigned j=0; j<foveaRes; j++)
			savedPercept.push_back(fovealView.getBinary(i, j));

	for(unsigned k=0; k<numSensors; k++)
		brain->setInput(k, savedPercept[k]);

	foveaPositionTimeSeries.push_back(foveaPosition);

	AbstractSensors::update(visualize); // increment the clock
}

void PeripheralAndRelativeSaccadingEyesSensors::reset(int visualize) {
	AbstractSensors::reset(visualize);
	foveaPositionTimeSeries.clear();
}

void* PeripheralAndRelativeSaccadingEyesSensors::logTimeSeries(const string& label) {
	ofstream ctrlog(string("foveaPosition_") + label + string(".log"));
	for(const auto& fp : foveaPositionTimeSeries)
		ctrlog << fp.first.first << fp.second.first << fp.first.second << fp.second.second << endl;
	ctrlog.close();
	return nullptr;
}

unsigned PeripheralAndRelativeSaccadingEyesSensors::numSaccades() {
	set<Range2d> differentPositions;
	for(const auto& fp : foveaPositionTimeSeries)
		differentPositions.insert(fp);
	return differentPositions.size();
}

/********** Private PeripheralAndRelativeSaccadingEyesSensors definitions **********/

void PeripheralAndRelativeSaccadingEyesSensors::analyzeDataset() {
	cout << "WARNING: PeripheralAndRelativeSaccadingEyesSensors do not implement dataset analysis!" << endl;
	cout << "If you need this functionality, rerun the simulation with AbsoluteFocusingSaccadingEyesSensors" << endl;
}
