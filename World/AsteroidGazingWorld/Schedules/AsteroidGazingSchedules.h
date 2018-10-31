#pragma once

#include <memory>
#include <vector>

#include "../../AbstractStateSchedule.h"

class AbstractAsteroidGazingSchedule : public AbstractStateSchedule {

protected:
	std::shared_ptr<std::string> currentAsteroidName;

	std::string asteroidsDatasetPath;
	std::vector<std::string> asteroidNames;
	unsigned numAsteroids;

private:
	void readAsteroidsFromDir(std::string datasetPath);

public:
	AbstractAsteroidGazingSchedule(std::shared_ptr<std::string> curAsteroidName, std::string astDatasetPath);
};

class ExhaustiveAsteroidGazingSchedule : public AbstractAsteroidGazingSchedule {

private:
	unsigned currentAsteroidIndex;
	bool terminalState;

public:
	ExhaustiveAsteroidGazingSchedule(std::shared_ptr<std::string> curAsteroidName, std::string astDatasetPath) :
		AbstractAsteroidGazingSchedule(curAsteroidName, astDatasetPath), terminalState(false) { reset(0); };

	void reset(int visualize) override {
		currentAsteroidIndex=0;
		*currentAsteroidName = asteroidNames[currentAsteroidIndex];
		terminalState = false;
	};

	void advance(int visualize) override {
		if(currentAsteroidIndex<(numAsteroids-1))
			currentAsteroidIndex++;
		else
				terminalState = true;
		*currentAsteroidName = asteroidNames[currentAsteroidIndex];
	};

	bool stateIsFinal() override { return terminalState; };
};
