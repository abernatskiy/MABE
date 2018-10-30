#pragma once

#include "../AbstractStateSchedule.h"

class AbstractAsteroidGazingSchedule : public AbstractStateSchedule {

protected:
	std::shared_ptr<std::string> currentAsteroidName;
	std::shared_ptr<std::string> previousAsteroidName;

	std::string asteroidsDatasetPath;
	std::vector<std::string> asteroidNames;
	unsigned numAsteroids;
	unsigned currentAsteroidIndex;

private:
	virtual void resetAsteroidIndex() = 0;
	virtual void advanceAsteroidIndex() = 0;
	virtual bool asteroidsExhausted() = 0;

	void readAsteroidsFromDir(std::string datasetPath) {
		// Listing a dir in c++14 turned out to be not straightforward, so leaving a placeholder here for now
		for(int i=0; i<10; i++)
			asteroidNames.push_back("asteroid" + std::to_string(i));
	};

public:
	AbstractAsteroidGazingSchedule(std::shared_ptr<std::string> curAsteroidName, std::string astDatasetPath) :
		currentAsteroidName(curAsteroidName), asteroidsDatasetPath(astDatasetPath) {
		// listing the dataset dir and obtaining the list of asteroid names
		readAsteroidsFromDir(asteroidsDatasetPath);
		numAsteroids = asteroidNames.size();

		// initializing the name of asteroid currently observed to be the first one in the sequence
		reset(0);
	};

	void reset(int visualize) override {
		resetAsteroidIndex();
		*currentAsteroidName = asteroidNames[currentAsteroidIndex];
	};

	void advance(int visualize) override {
		advanceAsteroidIndex();
		*currentAsteroidName = asteroidNames[currentAsteroidIndex];
	};

	bool stateIsFinal() override {
		return asteroidsExhausted();
	};
};

class ExhaustiveAsteroidGazingSchedule : public AbstractAsteroidGazingSchedule {

private:
	bool terminalState;
	void resetAsteroidIndex() override { currentAsteroidIndex=0; };
	void advanceAsteroidIndex() override {
		if(currentAsteroidIndex<numAsteroids) {
			currentAsteroidIndex++;
			if(currentAsteroidIndex>=numAsteroids)
				terminalState = true;
		}
	};
	bool asteroidsExhausted() override {
		return terminalState;
	};

public:
	ExhaustiveAsteroidGazingSchedule(std::shared_ptr<std::string> curAsteroidName, std::string astDatasetPath) :
		AbstractAsteroidGazingSchedule(curAsteroidName, astDatasetPath), terminalState(false) {};
};
