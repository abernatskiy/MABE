#include "PeripheralAndRelativeSaccadingEyesSensors.h"

using namespace std;

std::shared_ptr<ParameterLink<int>> PeripheralAndRelativeSaccadingEyesSensors::frameResolutionPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_RELATIVE_SACCADING_EYE-frameResolution", 16,
                                 "resolution of the frame over which the eye saccades (default: 16)");
std::shared_ptr<ParameterLink<int>> PeripheralAndRelativeSaccadingEyesSensors::peripheralFOVResolutionPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_RELATIVE_SACCADING_EYE-peripheralFOVResolution", 4,
                                 "resolution of the thumbnail preview that's shown alongside the retina picture (default: 4)");
std::shared_ptr<ParameterLink<int>> PeripheralAndRelativeSaccadingEyesSensors::foveaResolutionPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_RELATIVE_SACCADING_EYE-foveaResolution", 2,
                                 "resolution of the fovea (default: 2)");
std::shared_ptr<ParameterLink<int>> PeripheralAndRelativeSaccadingEyesSensors::jumpTypePL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_RELATIVE_SACCADING_EYE-jumpType", 0,
                                 "type of the saccading jump (0 - linear scale, 1 - sign-and-magnitude, 2 - Chris's compact, default: 0)");
std::shared_ptr<ParameterLink<int>> PeripheralAndRelativeSaccadingEyesSensors::jumpGradationsPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_RELATIVE_SACCADING_EYE-jumpGradations", 4,
                                 "number of gradations of the saccade length (default: 4)");

PeripheralAndRelativeSaccadingEyesSensors::PeripheralAndRelativeSaccadingEyesSensors(shared_ptr<string> curAstName,
                                                                                     shared_ptr<AsteroidsDatasetParser> dsParser,
                                                                                     shared_ptr<ParametersTable> PT_) :
	currentAsteroidName(curAstName), datasetParser(dsParser),
	frameRes(frameResolutionPL->get(PT_)),
	peripheralFOVRes(peripheralFOVResolutionPL->get(PT_)),
	foveaRes(foveaResolutionPL->get(PT_)),
	jumpType(jumpTypePL->get(PT_)),
	jumpGradations(jumpGradationsPL->get(PT_)),
	rangeDecoder(constructRangeDecoder(jumpType, jumpGradations, frameRes, foveaRes)),
	foveaPositionControls(rangeDecoder->numControls()),
	numSensors(peripheralFOVRes*peripheralFOVRes+foveaRes*foveaRes),
	numMotors(rangeDecoder->numControls()) {

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

	resetFoveaPosition();
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
	resetFoveaPosition();
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

void PeripheralAndRelativeSaccadingEyesSensors::resetFoveaPosition() {
	foveaPosition.first.first = 0;
	foveaPosition.first.second = foveaRes;
	foveaPosition.second.first = 0;
	foveaPosition.second.second = foveaRes;
}
