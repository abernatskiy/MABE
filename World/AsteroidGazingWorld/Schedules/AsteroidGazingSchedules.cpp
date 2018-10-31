#include "AsteroidGazingSchedules.h"

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"
#include <iostream>
#include <algorithm>
#include <cstdlib>

AbstractAsteroidGazingSchedule::AbstractAsteroidGazingSchedule(std::shared_ptr<std::string> curAsteroidName, std::string astDatasetPath) :
	currentAsteroidName(curAsteroidName), asteroidsDatasetPath(astDatasetPath) {

	readAsteroids();
	// listing the dataset dir and obtaining the list of asteroid names
	numAsteroids = asteroidNames.size();
	// initialize the name of asteroid currently observed in daughter classes
}

void AbstractAsteroidGazingSchedule::readAsteroids() {
	// Every subfolder in the dataset folder is assumed to have a name of an asteroid
	namespace fs = boost::filesystem;
	fs::path fsDatasetPath = fs::system_complete(asteroidsDatasetPath);
	if( !fs::exists(fsDatasetPath) || !fs::is_directory(fsDatasetPath) ) {
		std::cerr << "Dataset path " << fsDatasetPath << " does not exist or is not a directory" << std::endl;
		exit(EXIT_FAILURE);
	}

	fs::directory_iterator end_iter;
	for( fs::directory_iterator dir_itr(fsDatasetPath); dir_itr != end_iter; ++dir_itr ) {
		try {
			if( fs::is_directory(dir_itr->status()) )
				asteroidNames.push_back(dir_itr->path().filename().string());
		} catch (const std::exception & ex) {
			std::cerr << "Exception " << ex.what() << " occurred while trying to list a dataset folder" << std::endl;
			exit(EXIT_FAILURE);
		}
	}

	std::sort(asteroidNames.begin(), asteroidNames.end());
}
