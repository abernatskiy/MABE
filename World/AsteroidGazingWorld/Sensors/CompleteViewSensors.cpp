#include "CompleteViewSensors.h"

#include <iostream>
#include <cstdlib>
#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include "../../../Utilities/nlohmann/json.hpp"
#include "misc.h"

using namespace std;
namespace fs = boost::filesystem;

shared_ptr<ParameterLink<int>> CompleteViewSensors::frameResolutionPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_COMPLETE_VIEW-frameResolution", 16,
                                 "resolution of the view (default: 16)");
shared_ptr<ParameterLink<int>> CompleteViewSensors::numPhasesPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_COMPLETE_VIEW-numPhases", 1,
                                 "number of asteroid rotation phases to use (default: 1)");
shared_ptr<ParameterLink<int>> CompleteViewSensors::binarizationThresholdPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_COMPLETE_VIEW-binarizationThreshold", 128,
                                 "threshold to use for the snapshot binarization (0-255, default: 128)");

SerializeableArray<char> CompleteViewSensors::snapshotsCache;

CompleteViewSensors::CompleteViewSensors(shared_ptr<string> curAstName,
                                         shared_ptr<AsteroidsDatasetParser> dsParser,
                                         shared_ptr<ParametersTable> PT_) :
	currentAsteroidName(curAstName), datasetParser(dsParser),
	storedPerceptIdentifier(""),
	frameRes(frameResolutionPL->get(PT_)),
	numPhases(numPhasesPL->get(PT_)),
	baseThreshold(binarizationThresholdPL->get(PT_)) {

	for(unsigned i=0; i<numPhases; i++)
		phases.push_back(i); // current assumption is that we want all phases and that they are naturals up to numPhases, but that may change

	perceptPtr = new Texture(boost::extents[frameRes][frameRes][numPhases][1]);
}

CompleteViewSensors::~CompleteViewSensors() {

	delete perceptPtr;
}

void CompleteViewSensors::update(int visualize) {

	if(storedPerceptIdentifier!=(*currentAsteroidName)) {
		for(unsigned i=0; i<numPhases; i++) {
			AsteroidSnapshot& astSnap = asteroidSnapshots.at(CompleteAsteroidViewParameters(*currentAsteroidName, phases[i]));
			const AsteroidSnapshot& theView = astSnap.cachingResampleArea(0, 0, astSnap.width, astSnap.height, frameRes, frameRes, baseThreshold);
			for(size_t x=0; x<frameRes; x++)
				for(size_t y=0; y<frameRes; y++)
					(*perceptPtr)[x][y][i][0] = theView.getBinary(x, y);
		}
		storedPerceptIdentifier = *currentAsteroidName;
	}

//	cout << *currentAsteroidName << endl;
//	cout << readableRepr(*perceptPtr);

	AbstractSensors::update(visualize); // increment the clock
}

void CompleteViewSensors::reset(int visualize) {
	AbstractSensors::reset(visualize);
}

void CompleteViewSensors::doHeavyInit() {
	if(snapshotsCache.empty()) {
		if(!readPersistentSnapshotsCache()) {
			readSnapshotsIntoCache();
			writePersistentSnapshotsCache();
		}
	}
	loadSnapshotsFromCache();

/*
	set<string> asteroidNames = datasetParser->getAsteroidsNames();

	cout << "Sensor " << this << ": verifying snapshots read from RAM cache" << endl;
	for(const string& an : asteroidNames) {
		map<string,set<unsigned>> parameterValuesSets = datasetParser->getAllParameterValues(an);
		for(const auto& pvs : parameterValuesSets) {
			if(pvs.first=="phase") {
				for(const auto& ph : phases) {
					if(pvs.second.find(ph)==pvs.second.end()) {
						cerr << "CompleteViewSensors: required phase value " << ph << " is absent for asteroid " << an << endl;
						exit(EXIT_FAILURE);
					}
				}
				continue;
			}
			if(pvs.second.size()!=1) {
				cerr << "CompleteViewSensors: parameter " << pvs.first << " takes more than one value and only phase is allowed to vary in the current version" << endl;
				exit(EXIT_FAILURE);
			}
		}
		unsigned condition = *(parameterValuesSets["condition"].begin());
		unsigned distance = *(parameterValuesSets["distance"].begin());

		//cout << "Asteroid " << an << " phase";
		for(unsigned phase : phases) {
			string snapshotPath = datasetParser->getPicturePath(an, condition, distance, phase);
			if(!(asteroidSnapshots[CompleteAsteroidViewParameters(an, phase)]==AsteroidSnapshot(snapshotPath, baseThreshold))) {
				cout << "Snapshots differ for asteroid " << an << " at phase " << phase << "!" << endl << flush;
				exit(EXIT_FAILURE);
			}
			//cout << " " << phase;
		}
		//cout << " OK" << endl;
	}
*/
}

/********** Private CompleteViewSensors definitions **********/

bool CompleteViewSensors::readPersistentSnapshotsCache() {

	fs::path cachePath = datasetParser->getDatasetPath() / fs::path("snapshots.cache.bin");
	if(!fs::exists(cachePath))
		return false;
	snapshotsCache.deserialize(cachePath.string());
	return true;
}

void CompleteViewSensors::writePersistentSnapshotsCache() {

	fs::path cachePath = datasetParser->getDatasetPath() / fs::path("snapshots.cache.bin");
	snapshotsCache.serialize(cachePath.string());
}

void CompleteViewSensors::readSnapshotsIntoCache() {

	set<string> asteroidNames = datasetParser->getAsteroidsNames();
	size_t cacheSize = 0;
	vector<vector<AsteroidSnapshot>> tmpsnaps;
	for(const string& an : asteroidNames) {
		map<string,set<unsigned>> parameterValuesSets = datasetParser->getAllParameterValues(an);
		for(const auto& pvs : parameterValuesSets) {
//			cout << pvs.first << ":"; for(const auto& pval : pvs.second) cout << " " << pval; cout << endl;
			if(pvs.first=="phase") {
				for(const auto& ph : phases) {
					if(pvs.second.find(ph)==pvs.second.end()) {
						cerr << "CompleteViewSensors: required phase value " << ph << " is absent for asteroid " << an << endl;
						exit(EXIT_FAILURE);
					}
				}
				continue;
			}
			if(pvs.second.size()!=1) {
				cerr << "CompleteViewSensors: parameter " << pvs.first << " takes more than one value and only phase is allowed to vary in the current version" << endl;
				exit(EXIT_FAILURE);
			}
		}
		unsigned condition = *(parameterValuesSets["condition"].begin());
		unsigned distance = *(parameterValuesSets["distance"].begin());

		tmpsnaps.push_back({});
		for(unsigned phase : phases) {
			string snapshotPath = datasetParser->getPicturePath(an, condition, distance, phase);
			tmpsnaps.back().emplace_back(snapshotPath, baseThreshold);
			cacheSize += tmpsnaps.back().back().getSerializedSizeInBytes();
		}
	}
	snapshotsCache.reserve(cacheSize);
	size_t cachePos = 0;
	for(const auto& astSnaps : tmpsnaps) {
		for(const auto& snap : astSnaps) {
			snap.serializeToRAM(snapshotsCache.elements+cachePos);
			cachePos += snap.getSerializedSizeInBytes();
		}
	}
}

void CompleteViewSensors::loadSnapshotsFromCache() {

	set<string> asteroidNames = datasetParser->getAsteroidsNames();
	size_t cachePos = 0;
	for(const string& an : asteroidNames) {
		for(unsigned phase : phases) {
			CompleteAsteroidViewParameters curParams(an, phase);
			asteroidSnapshots.emplace(pair<CompleteAsteroidViewParameters,AsteroidSnapshot>(curParams,
			                                                                                AsteroidSnapshot(snapshotsCache.elements+cachePos)));
			cachePos += asteroidSnapshots[curParams].getSerializedSizeInBytes();
		}
	}
}
