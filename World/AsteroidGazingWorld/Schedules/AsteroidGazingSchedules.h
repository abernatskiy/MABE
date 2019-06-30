#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cstdlib>

#include "../../AbstractStateSchedule.h"
#include "../Utilities/AsteroidsDatasetParser.h"
#include "../Sensors/PeripheralAndRelativeSaccadingEyesSensors.h" // for the Range2D type

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
		AbstractAsteroidGazingSchedule(curAsteroidName, dsParser),
		currentAsteroidIndex(0),
		terminalState(false) {

		*currentAsteroidName = asteroidNames[currentAsteroidIndex];
	};

	virtual void reset(int visualize) override {
		currentAsteroidIndex=0;
		*currentAsteroidName = asteroidNames[currentAsteroidIndex];
		terminalState = false;
	};

	virtual void advance(int visualize) override {
		if(currentAsteroidIndex<(numAsteroids-1))
			currentAsteroidIndex++;
		else
			terminalState = true;
		*currentAsteroidName = asteroidNames[currentAsteroidIndex];
	};

	bool stateIsFinal() override { return terminalState; };

	const std::string& currentStateDescription() override { return asteroidNames.at(currentAsteroidIndex); };
};

class ExhaustiveAsteroidGazingScheduleWithRelativeSensorInitialStates : public ExhaustiveAsteroidGazingSchedule {

private:
  std::shared_ptr<Range2d> sensorInitialState;
	std::map<std::string,std::vector<Range2d>> relativeSensorsInitialStates;
	unsigned currentInitialConditionIdx;

public:
	ExhaustiveAsteroidGazingScheduleWithRelativeSensorInitialStates(std::shared_ptr<std::string> curAsteroidName,
	                                                                std::shared_ptr<AsteroidsDatasetParser> dsParser,
                                                                  std::shared_ptr<Range2d> senInitialState,
	                                                                std::map<std::string,std::vector<Range2d>> relSensorsInitialStates) :
		ExhaustiveAsteroidGazingSchedule(curAsteroidName, dsParser),
    sensorInitialState(senInitialState),
		relativeSensorsInitialStates(relSensorsInitialStates),
		currentInitialConditionIdx(0) {};

	void reset(int visualize) override {
		ExhaustiveAsteroidGazingSchedule::reset(visualize);
		currentInitialConditionIdx = 0;
    *sensorInitialState = relativeSensorsInitialStates[*currentAsteroidName][currentInitialConditionIdx];
	};

	void advance(int visualize) override {
    currentInitialConditionIdx++;
    if(currentInitialConditionIdx == relativeSensorsInitialStates[*currentAsteroidName].size()) {
      ExhaustiveAsteroidGazingSchedule::advance(visualize);
      currentInitialConditionIdx = 0;
    }
    *sensorInitialState = relativeSensorsInitialStates[*currentAsteroidName][currentInitialConditionIdx];
  };
};
