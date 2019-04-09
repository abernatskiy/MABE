#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cstdlib>

#include "../../AbstractStateSchedule.h"
#include "../Utilities/AsteroidsDatasetParser.h"

class AbstractAsteroidGazingSchedule : public AbstractStateSchedule {

protected:
	std::shared_ptr<std::string> currentAsteroidName;

	std::string asteroidsDatasetPath;
	std::vector<std::string> asteroidNames;
	unsigned numAsteroids;

public:
	AbstractAsteroidGazingSchedule(std::shared_ptr<std::string> curAsteroidName, std::shared_ptr<AsteroidsDatasetParser> dsParser) :
		currentAsteroidName(curAsteroidName) {
		std::set<std::string> asteroidNamesSet = dsParser->getAsteroidsNames();
		std::copy(asteroidNamesSet.begin(), asteroidNamesSet.end(), std::back_inserter(asteroidNames));
		std::sort(asteroidNames.begin(), asteroidNames.end());
		numAsteroids = asteroidNames.size();
	};
};

class ExhaustiveAsteroidGazingSchedule : public AbstractAsteroidGazingSchedule {

private:
	unsigned currentAsteroidIndex;
	bool terminalState;

public:
	ExhaustiveAsteroidGazingSchedule(std::shared_ptr<std::string> curAsteroidName, std::shared_ptr<AsteroidsDatasetParser> dsParser) :
		AbstractAsteroidGazingSchedule(curAsteroidName, dsParser), terminalState(false) { reset(0); };

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

	const std::string& currentStateDescription() override { return asteroidNames.at(currentAsteroidIndex); };
};
