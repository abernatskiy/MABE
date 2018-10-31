#include "AsteroidGazingSchedules.h"

void AbstractAsteroidGazingSchedule::readAsteroidsFromDir(std::string datasetPath) {
	// Listing a dir in c++14 turned out to be not straightforward, so leaving a placeholder here for now
	for(int i=0; i<10; i++)
		asteroidNames.push_back("asteroid" + std::to_string(i));
}

AbstractAsteroidGazingSchedule::AbstractAsteroidGazingSchedule(std::shared_ptr<std::string> curAsteroidName, std::string astDatasetPath) :
	currentAsteroidName(curAsteroidName), asteroidsDatasetPath(astDatasetPath) {

	// listing the dataset dir and obtaining the list of asteroid names
	readAsteroidsFromDir(asteroidsDatasetPath);
	numAsteroids = asteroidNames.size();
	// initialize the name of asteroid currently observed in daughter classes
}
