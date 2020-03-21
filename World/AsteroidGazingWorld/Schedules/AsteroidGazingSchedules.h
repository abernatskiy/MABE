#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include <iterator>
#include <cstdlib>
#include <random>

#include "../../AbstractStateSchedule.h"
#include "../Utilities/AsteroidsDatasetParser.h"
#include "../Sensors/PeripheralAndRelativeSaccadingEyesSensors.h" // for the Range2D type
#include "../../../Global.h" // for Global::update

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

protected:
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

	virtual const std::string& currentStateDescription() override { return asteroidNames.at(currentAsteroidIndex); };
};

class ExhaustiveAsteroidGazingScheduleWithRelativeSensorInitialStates : public ExhaustiveAsteroidGazingSchedule {

private:
  std::shared_ptr<Range2d> sensorInitialState;
	std::map<std::string,std::vector<Range2d>> relativeSensorsInitialStates;
	unsigned currentInitialConditionIdx;
	std::string currentStateDescriptionStr;

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

	const std::string& currentStateDescription() override {
		currentStateDescriptionStr = asteroidNames.at(currentAsteroidIndex) + "_ifp" + std::to_string(currentInitialConditionIdx);
		return currentStateDescriptionStr;
	};
};

//////////////////////////////////////// Minibatch schedules //////////////////////////////////////////

class MinibatchAsteroidGazingSchedule : public AbstractAsteroidGazingSchedule {

protected:
	unsigned currentAsteroidIndex;
	bool terminalState;
	std::mt19937 rng;
	std::vector<unsigned> minibatchIndices;
	unsigned batchSize;

	int curUpdate;

	void updateMinibatchIndices() {
		minibatchIndices.clear();
		while(minibatchIndices.size()<batchSize) {
			unsigned idxcan = std::uniform_int_distribution<int>(0, numAsteroids-1)(rng);
			if(std::find(minibatchIndices.begin(), minibatchIndices.end(), idxcan) == minibatchIndices.end())
				minibatchIndices.push_back(idxcan);
		}
	};

public:
	MinibatchAsteroidGazingSchedule(std::shared_ptr<std::string> curAsteroidName,
	                                 std::shared_ptr<AsteroidsDatasetParser> dsParser,
	                                 unsigned bSize,
	                                 unsigned seed) :
		AbstractAsteroidGazingSchedule(curAsteroidName, dsParser),
		currentAsteroidIndex(0),
		terminalState(false),
		batchSize(bSize),
		rng(seed) {

		*currentAsteroidName = asteroidNames[currentAsteroidIndex];

		curUpdate = Global::update;
		updateMinibatchIndices();

		if(batchSize > numAsteroids/2) {
			std::cerr << "MinibatchAsteroidGazingSchedule: requested a batch size " << batchSize << " that is more than a half of the number of asteroids " << numAsteroids << std::endl;
			std::cerr << "This schedule is ill-adapted to batches this big, exiting to prevent hanging" << std::endl;
			exit(EXIT_FAILURE);
		}
	};

	virtual void reset(int visualize) override {
		currentAsteroidIndex=0;
		*currentAsteroidName = asteroidNames[currentAsteroidIndex];
		terminalState = false;
		if(Global::update != curUpdate) {
			curUpdate = Global::update;
			updateMinibatchIndices();
		}
	};

	virtual void advance(int visualize) override {
		if(currentAsteroidIndex<(batchSize-1))
			currentAsteroidIndex++;
		else
			terminalState = true;
		*currentAsteroidName = asteroidNames[minibatchIndices[currentAsteroidIndex]];
	};

	bool stateIsFinal() override { return terminalState; };

	virtual const std::string& currentStateDescription() override { return asteroidNames.at(minibatchIndices.at(currentAsteroidIndex)); };
};

class MinibatchAsteroidGazingScheduleWithRelativeSensorInitialStates : public MinibatchAsteroidGazingSchedule {
// Written with copy and paste for the sake of saving time on 2020-03-21
// If it is ever refactored rewrite it with the underlying schedule as a class member and not a parent class

private:
  std::shared_ptr<Range2d> sensorInitialState;
	std::map<std::string,std::vector<Range2d>> relativeSensorsInitialStates;
	unsigned currentInitialConditionIdx;
	std::string currentStateDescriptionStr;

public:
	MinibatchAsteroidGazingScheduleWithRelativeSensorInitialStates(std::shared_ptr<std::string> curAsteroidName,
	                                                               std::shared_ptr<AsteroidsDatasetParser> dsParser,
                                                                 std::shared_ptr<Range2d> senInitialState,
	                                                               std::map<std::string,std::vector<Range2d>> relSensorsInitialStates,
	                                                               unsigned bSize,
	                                                               unsigned seed) :
		MinibatchAsteroidGazingSchedule(curAsteroidName, dsParser, bSize, seed),
    sensorInitialState(senInitialState),
		relativeSensorsInitialStates(relSensorsInitialStates),
		currentInitialConditionIdx(0) {};

	void reset(int visualize) override {
		MinibatchAsteroidGazingSchedule::reset(visualize);
		currentInitialConditionIdx = 0;
    *sensorInitialState = relativeSensorsInitialStates[*currentAsteroidName][currentInitialConditionIdx];
	};

	void advance(int visualize) override {
    currentInitialConditionIdx++;
    if(currentInitialConditionIdx == relativeSensorsInitialStates[*currentAsteroidName].size()) {
      MinibatchAsteroidGazingSchedule::advance(visualize);
      currentInitialConditionIdx = 0;
    }
    *sensorInitialState = relativeSensorsInitialStates[*currentAsteroidName][currentInitialConditionIdx];
  };

	const std::string& currentStateDescription() override {
		currentStateDescriptionStr = asteroidNames.at(minibatchIndices.at(currentAsteroidIndex)) + "_ifp" + std::to_string(currentInitialConditionIdx);
		return currentStateDescriptionStr;
	};
};
