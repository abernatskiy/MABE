#include "PeripheralAndRelativeSaccadingEyesSensors.h"

#include <iostream>
#include <cstdlib>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "../../../Utilities/nlohmann/json.hpp"
#include "misc.h"

using namespace std;
namespace fs = boost::filesystem;

shared_ptr<ParameterLink<int>> PeripheralAndRelativeSaccadingEyesSensors::frameResolutionPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_RELATIVE_SACCADING_EYE-frameResolution", 16,
                                 "resolution of the frame over which the eye saccades (default: 16)");
shared_ptr<ParameterLink<bool>> PeripheralAndRelativeSaccadingEyesSensors::usePeripheralFOVPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_RELATIVE_SACCADING_EYE-usePeripheralFOV", true,
                                 "boolean value indicating whether the peripheral field of view should be used (default: 1)");
shared_ptr<ParameterLink<int>> PeripheralAndRelativeSaccadingEyesSensors::peripheralFOVResolutionPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_RELATIVE_SACCADING_EYE-peripheralFOVResolution", 4,
                                 "resolution of the thumbnail preview that's shown alongside the retina picture (default: 4)");
shared_ptr<ParameterLink<int>> PeripheralAndRelativeSaccadingEyesSensors::peripheralFOVNumThresholdsToTryPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_RELATIVE_SACCADING_EYE-peripheralFOVNumThresholdsToTry", 7,
                                 "number of threshold levels that peripheral FOV entropy maximizer should try, non-positives meaning skip this step and use 127 as the threshold (default: 7)");
shared_ptr<ParameterLink<int>> PeripheralAndRelativeSaccadingEyesSensors::foveaResolutionPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_RELATIVE_SACCADING_EYE-foveaResolution", 2,
                                 "resolution of the fovea (default: 2)");
shared_ptr<ParameterLink<int>> PeripheralAndRelativeSaccadingEyesSensors::jumpTypePL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_RELATIVE_SACCADING_EYE-jumpType", 0,
                                 "type of the saccading jump (0 - linear scale, 1 - sign-and-magnitude, 2 - Chris's compact, default: 0)");
shared_ptr<ParameterLink<int>> PeripheralAndRelativeSaccadingEyesSensors::jumpGradationsPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_RELATIVE_SACCADING_EYE-jumpGradations", 4,
                                 "number of gradations of the saccade length (default: 4)");
shared_ptr<ParameterLink<bool>> PeripheralAndRelativeSaccadingEyesSensors::forbidRestPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_RELATIVE_SACCADING_EYE-forbidRest", false,
                                 "if this is set to one sensors will move fovea at every time step with any input, with direction and magnitude of the move determined as usual. WARNING: ignored by all sensors but Chris's at the moment (default: 0)");

SerializeableArray<char> PeripheralAndRelativeSaccadingEyesSensors::snapshotsCache;

PeripheralAndRelativeSaccadingEyesSensors::PeripheralAndRelativeSaccadingEyesSensors(shared_ptr<string> curAstName,
                                                                                     shared_ptr<AsteroidsDatasetParser> dsParser,
                                                                                     shared_ptr<ParametersTable> PT_) :
	currentAsteroidName(curAstName), datasetParser(dsParser),
	frameRes(frameResolutionPL->get(PT_)),
	usePeripheralFOV(usePeripheralFOVPL->get(PT_)),
	peripheralFOVRes(peripheralFOVResolutionPL->get(PT_)),
	foveaRes(foveaResolutionPL->get(PT_)),
	jumpType(jumpTypePL->get(PT_)),
	jumpGradations(jumpGradationsPL->get(PT_)),
	forbidRest(forbidRestPL->get(PT_)),
	numThresholdsToTry(peripheralFOVNumThresholdsToTryPL->get(PT_)),
	rangeDecoder(constructRangeDecoder(jumpType, jumpGradations, frameRes, foveaRes, forbidRest)),
	foveaPositionControls(rangeDecoder->numControls()),
	numSensors(foveaRes*foveaRes + (usePeripheralFOV ? peripheralFOVRes*peripheralFOVRes : 0 )),
	numMotors(rangeDecoder->numControls()),
	initialFoveaPositionPtr(make_shared<Range2d>(Range2d(Range1d(0, 0), Range1d(0, 0)))) {

	// Some argument validation
	if(foveaRes>frameRes) {
		cerr << "Fovea size (" << foveaRes << ") exceeds frame size, exiting" << endl;
		exit(EXIT_FAILURE);
	}
}

void PeripheralAndRelativeSaccadingEyesSensors::update(int visualize) {
	for(unsigned i=0; i<numMotors; i++)
		controls[i] = brain->readOutput(i) > 0.5;

//	cout << "Read controls:"; for(bool c : controls) cout << c; cout << endl;

	AsteroidSnapshot& astSnap = asteroidSnapshots.at(make_tuple(*currentAsteroidName));
	savedPercept.clear();

	if(usePeripheralFOV) {
		uint8_t periFOVThresh = peripheralFOVThresholds.at(make_tuple(*currentAsteroidName));
		const AsteroidSnapshot& peripheralView = astSnap.cachingResampleArea(0, 0, astSnap.width, astSnap.height, peripheralFOVRes, peripheralFOVRes, periFOVThresh);
		for(unsigned i=0; i<peripheralFOVRes; i++)
			for(unsigned j=0; j<peripheralFOVRes; j++)
				savedPercept.push_back(peripheralView.getBinary(i, j));

//		cout << "Perceiving asteroid " << *currentAsteroidName << endl;
//		peripheralView.printBinary();
	}

	foveaPositionOnGrid = rangeDecoder->decode2dRangeJump(foveaPositionOnGrid, controls.begin(), controls.end());
	foveaPosition.first.first = (foveaPositionOnGrid.first.first*astSnap.width)/frameRes;
	foveaPosition.first.second = (foveaPositionOnGrid.first.second*astSnap.width)/frameRes;
	foveaPosition.second.first = (foveaPositionOnGrid.second.first*astSnap.height)/frameRes;
	foveaPosition.second.second = (foveaPositionOnGrid.second.second*astSnap.height)/frameRes;

	const AsteroidSnapshot& fovealView = astSnap.cachingResampleArea(foveaPosition.first.first, foveaPosition.second.first,
	                                                                 foveaPosition.first.second, foveaPosition.second.second,
	                                                                 foveaRes, foveaRes, baseThreshold);
	for(unsigned i=0; i<foveaRes; i++)
		for(unsigned j=0; j<foveaRes; j++)
			savedPercept.push_back(fovealView.getBinary(i, j));

	for(unsigned k=0; k<numSensors; k++)
		brain->setInput(k, savedPercept[k]);

//	cout << "Wrote sensor readings:"; for(bool p : savedPercept) cout << p; cout << endl;

	foveaPositionTimeSeries.push_back(foveaPosition);
	perceptTimeSeries.push_back(savedPercept);
	controlsTimeSeries.push_back(controls);
	sensorStateDescriptionTimeSeries.push_back(getSensorStateDescription());

	AbstractSensors::update(visualize); // increment the clock
}

void PeripheralAndRelativeSaccadingEyesSensors::reset(int visualize) {
//	cout << "Reset is called on sensors" << endl << endl;
	AbstractSensors::reset(visualize);
	resetFoveaPosition();
	controls.assign(numMotors, false);
	foveaPositionTimeSeries.clear();
	perceptTimeSeries.clear();
	controlsTimeSeries.clear();
	sensorStateDescriptionTimeSeries.clear();
}

void PeripheralAndRelativeSaccadingEyesSensors::doHeavyInit() {

	// Loading asteroid snapshots, determining optimal peripheral FOV threshold for each
	if(snapshotsCache.empty()) {
		if(!readPersistentSnapshotsCache()) {
			readSnapshotsIntoCache();
			writePersistentSnapshotsCache();
		}
	}
	loadSnapshotsFromCache();

	set<string> asteroidNames = datasetParser->getAsteroidsNames();

/*
	cout << "Sensor " << this << ": verifying snapshots read from RAM cache" << endl;
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
		if(!(asteroidSnapshots[make_tuple(an)]==AsteroidSnapshot(snapshotPath, baseThreshold)))
			cout << "Snapshots differ for asteroid " << an << "!" << endl;
	}
*/

	for(const string& an : asteroidNames) {
		if(numThresholdsToTry > 0) {
			uint8_t periFOVThresh = asteroidSnapshots[make_tuple(an)].getBestThreshold(peripheralFOVRes, numThresholdsToTry);
			peripheralFOVThresholds.emplace(make_tuple(an), periFOVThresh);
		}
		else
			peripheralFOVThresholds.emplace(make_tuple(an), baseThreshold);
	}

	// Preparing the state
	resetFoveaPosition();
	controls.assign(numMotors, false);

	// analyzeDataset();
}

void* PeripheralAndRelativeSaccadingEyesSensors::logTimeSeries(const string& label) {
	ofstream ctrlog(string("sensorState_") + label + string(".log"));
	for(const auto& ssd : sensorStateDescriptionTimeSeries)
		ctrlog << ssd << endl;
	ctrlog.close();
	return nullptr;
}

unsigned PeripheralAndRelativeSaccadingEyesSensors::numSaccades() {
	set<Range2d> differentPositions;
	for(const auto& fp : foveaPositionTimeSeries)
		differentPositions.insert(fp);
	return differentPositions.size();
}

unsigned PeripheralAndRelativeSaccadingEyesSensors::numActiveStatesInRecording() {
	unsigned activeStates = 0;
	for(const auto& percept : perceptTimeSeries)
		activeStates += percept.size() - count(percept.begin(), percept.end(), 0);
	return activeStates;
}

double PeripheralAndRelativeSaccadingEyesSensors::sensoryMotorEntropy(int shift) {
	if(perceptTimeSeries.size() != controlsTimeSeries.size()) {
		cerr << "Percept and controls time series are of different lengths, cannot compute shared entropy" << endl;
		exit(EXIT_FAILURE);
	}
	if(perceptTimeSeries.size() <= abs(shift))
		return 0.;

	vector<long unsigned> perceptDigits;
	if(shift>=0) {
		for(auto it=perceptTimeSeries.begin(); it!=perceptTimeSeries.end()-shift; it++)
			perceptDigits.push_back(positionalBinaryToDecimal(*it));
	}
	else {
		for(auto it=perceptTimeSeries.begin()+abs(shift); it!=perceptTimeSeries.end(); it++)
			perceptDigits.push_back(positionalBinaryToDecimal(*it));
	}

	vector<long unsigned> oculoMotorDigits;
	if(shift>=0) {
		for(auto it=controlsTimeSeries.begin()+shift; it!=controlsTimeSeries.end(); it++)
			oculoMotorDigits.push_back(positionalBinaryToDecimal(*it));
	}
	else {
		for(auto it=controlsTimeSeries.begin(); it!=controlsTimeSeries.end()-abs(shift); it++)
			oculoMotorDigits.push_back(positionalBinaryToDecimal(*it));
	}

/*
	cout << "percept digits:";
	for(const auto& pd : perceptDigits)
		cout << " " << pd;
	cout << endl << "motor digits:";
	for(const auto& cd : oculoMotorDigits)
		cout << " " << cd;
	cout << endl;
*/

	const unsigned nts = perceptDigits.size();

	map<pair<long unsigned, long unsigned>, double> joint;
	map<long unsigned, double> ppercept;
	map<long unsigned, double> pmotor;
	for(unsigned i=0; i<nts; i++) {
		incrementMapFieldRobustly(pair<long unsigned, long unsigned>(perceptDigits[i], oculoMotorDigits[i]), joint);
		incrementMapFieldRobustly(perceptDigits[i], ppercept);
		incrementMapFieldRobustly(oculoMotorDigits[i], pmotor);
	}
	long unsigned ps, ms;

/*
	cout << "joint histogram:";
	for(const auto& jp : joint) {
		tie(ps, ms) = jp.first;
		cout << " (" << ps << "," << ms << "): " << jp.second;
	}
	cout << endl;
	cout << "percept histogram:";
	for(const auto& pp : ppercept)
		cout << " " << pp.first << ":" << pp.second;
	cout << endl;
	cout << "motor histogram:";
	for(const auto& mp : pmotor)
		cout << " " << mp.first << ":" << mp.second;
	cout << endl;
*/

	for(auto& jp : joint)
		jp.second /= static_cast<double>(nts);
	for(auto& pp : ppercept)
		pp.second /= static_cast<double>(nts);
	for(auto& mp : pmotor)
		mp.second /= static_cast<double>(nts);

	double sharedEntropy = 0.;
	for(const auto& jp : joint) {
		tie(ps, ms) = jp.first;
//		cout << "j:" << jp.second << " p:" << ppercept.at(ps) << " m:" << pmotor.at(ms) << endl << flush;
		sharedEntropy += jp.second*log2(jp.second/(ppercept.at(ps)*pmotor.at(ms)));
	}

//	cout << endl << flush;

	return sharedEntropy;
}

Range2d PeripheralAndRelativeSaccadingEyesSensors::generateRandomInitialState() {

	const int xshift = Random::getInt(frameRes-foveaRes);
	const int yshift = Random::getInt(frameRes-foveaRes);
	return Range2d(Range1d(xshift, xshift+foveaRes), make_pair(yshift, yshift+foveaRes));
}

/********** Private PeripheralAndRelativeSaccadingEyesSensors definitions **********/

void PeripheralAndRelativeSaccadingEyesSensors::analyzeDataset() {
	cout << "WARNING: PeripheralAndRelativeSaccadingEyesSensors do not implement dataset analysis!" << endl;
	cout << "If you need this functionality, rerun the simulation with AbsoluteFocusingSaccadingEyesSensors" << endl;
}

void PeripheralAndRelativeSaccadingEyesSensors::resetFoveaPosition() {
//	cout << "resetFoveaPosition called, initial range is " << range2dToStr(*initialFoveaPositionPtr) << ", asteroid name is " << *currentAsteroidName << endl;
	foveaPositionOnGrid = *initialFoveaPositionPtr;
	foveaPosition = foveaPositionOnGrid; // this should really be scaled to the original resolution, but since it is not available here and the value isn't used I'm putting in the surrogate
}

string PeripheralAndRelativeSaccadingEyesSensors::getSensorStateDescription() {
	nlohmann::json stateJSON = nlohmann::json::object();
	stateJSON["type"] = "PeripheralAndRelativeSaccadingEyesSensors";
	stateJSON["controls"] = nlohmann::json::array();
	for(const auto& ctrl : controls)
		stateJSON["controls"].push_back(static_cast<unsigned>(ctrl));
	stateJSON["foveaPosition"] = nlohmann::json::array();
	stateJSON["foveaPosition"].push_back({foveaPosition.first.first, foveaPosition.first.second});
	stateJSON["foveaPosition"].push_back({foveaPosition.second.first, foveaPosition.second.second});
	unsigned idx = 0;
	if(usePeripheralFOV) {
		stateJSON["peripheralPercept"] = nlohmann::json::array();
		for(unsigned i=0; i<peripheralFOVRes; i++) {
			stateJSON["peripheralPercept"].push_back(nlohmann::json::array());
			for(unsigned j=0; j<peripheralFOVRes; j++) {
				stateJSON["peripheralPercept"].back().push_back(static_cast<unsigned>(savedPercept[idx]));
				idx++;
			}
		}
	}
	stateJSON["fovealPercept"] = nlohmann::json::array();
	for(unsigned i=0; i<foveaRes; i++) {
		stateJSON["fovealPercept"].push_back(nlohmann::json::array());
		for(unsigned j=0; j<foveaRes; j++) {
			stateJSON["fovealPercept"].back().push_back(static_cast<unsigned>(savedPercept[idx]));
			idx++;
		}
	}
	return stateJSON.dump();
}

bool PeripheralAndRelativeSaccadingEyesSensors::readPersistentSnapshotsCache() {
	fs::path cachePath = datasetParser->getDatasetPath() / fs::path("snapshots.cache.bin");
	if(!fs::exists(cachePath))
		return false;
	snapshotsCache.deserialize(cachePath.string());
	return true;
}

void PeripheralAndRelativeSaccadingEyesSensors::writePersistentSnapshotsCache() {
	fs::path cachePath = datasetParser->getDatasetPath() / fs::path("snapshots.cache.bin");
	snapshotsCache.serialize(cachePath.string());
}

void PeripheralAndRelativeSaccadingEyesSensors::readSnapshotsIntoCache() {
	set<string> asteroidNames = datasetParser->getAsteroidsNames();
	size_t cacheSize = 0;
	vector<AsteroidSnapshot> tmpsnaps;
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
		tmpsnaps.emplace_back(snapshotPath, baseThreshold);
		cacheSize += tmpsnaps.back().getSerializedSizeInBytes();
	}
	snapshotsCache.reserve(cacheSize);
	size_t cachePos = 0;
	for(const auto& snap : tmpsnaps) {
		snap.serializeToRAM(snapshotsCache.elements+cachePos);
		cachePos += snap.getSerializedSizeInBytes();
	}
}

void PeripheralAndRelativeSaccadingEyesSensors::loadSnapshotsFromCache() {
	set<string> asteroidNames = datasetParser->getAsteroidsNames();
	size_t cachePos = 0;
	for(const string& an : asteroidNames) {
		asteroidSnapshots.emplace(pair<AsteroidViewParameters,AsteroidSnapshot>(make_tuple(an),
		                                                                        AsteroidSnapshot(snapshotsCache.elements+cachePos)));
		cachePos += asteroidSnapshots[make_tuple(an)].getSerializedSizeInBytes();
	}
}
