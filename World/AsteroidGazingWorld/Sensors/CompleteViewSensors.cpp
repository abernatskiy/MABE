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

SerializeableArray<char> CompleteViewSensors::snapshotsCache;

CompleteViewSensors::CompleteViewSensors(shared_ptr<string> curAstName,
                                         shared_ptr<AsteroidsDatasetParser> dsParser,
                                         shared_ptr<ParametersTable> PT_) :
	currentAsteroidName(curAstName), datasetParser(dsParser),
	storedPerceptIdentifier(""),
	frameRes(frameResolutionPL->get(PT_)) {

	perceptPtr = new Texture(boost::extents[frameRes][frameRes][1][1]);
}

CompleteViewSensors::~CompleteViewSensors() {

	delete perceptPtr;
}

void CompleteViewSensors::update(int visualize) {

	if(storedPerceptIdentifier!=(*currentAsteroidName)) {
		AsteroidSnapshot& astSnap = asteroidSnapshots.at(make_tuple(*currentAsteroidName));
		for(size_t x=0; x<frameRes; x++)
			for(size_t y=0; y<frameRes; y++)
				(*perceptPtr)[x][y][0][0] = astSnap.getBinary(x, y);
		storedPerceptIdentifier = *currentAsteroidName;
	}

//	cout << *currentAsteroidName << endl;
//	cout << readableTextureRepr(*perceptPtr);

	AbstractSensors::update(visualize); // increment the clock
}

void CompleteViewSensors::reset(int visualize) {

//	cout << "Reset is called on sensors" << endl << endl;
	AbstractSensors::reset(visualize);
}

void CompleteViewSensors::doHeavyInit() {

	// Loading asteroid snapshots, determining optimal peripheral FOV threshold for each
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
			if(pvs.second.size()!=1) {
				cerr << "CompleteViewSensors: Only asteroids with single available snapshot are supported at this moment" << endl;
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
	vector<AsteroidSnapshot> tmpsnaps;
	for(const string& an : asteroidNames) {
		map<string,set<unsigned>> parameterValuesSets = datasetParser->getAllParameterValues(an);
		for(const auto& pvs : parameterValuesSets) {
			if(pvs.second.size()!=1) {
				cerr << "CompleteViewSensors: Only asteroids with single available snapshot are supported at this moment" << endl;
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

void CompleteViewSensors::loadSnapshotsFromCache() {

	set<string> asteroidNames = datasetParser->getAsteroidsNames();
	size_t cachePos = 0;
	for(const string& an : asteroidNames) {
		asteroidSnapshots.emplace(pair<AsteroidViewParameters,AsteroidSnapshot>(make_tuple(an),
		                                                                        AsteroidSnapshot(snapshotsCache.elements+cachePos)));
		cachePos += asteroidSnapshots[make_tuple(an)].getSerializedSizeInBytes();
	}
}
