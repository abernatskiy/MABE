#pragma once

#include "../AbstractMotors.h"

class TestMotors : public AbstractMotors {

public:
	std::shared_ptr<int> motorEffort;
	std::shared_ptr<AbstractBrain> brain;

	TestMotors(std::shared_ptr<int> mEffort) : AbstractMotors(), motorEffort(mEffort) {};

	void resetMotors() override {}; // no need to change the internal state of the motors - motorEffort belongs to the world
	void attachToBrain(std::shared_ptr<AbstractBrain> br) override { brain=br; };
	void update(int timeStep, int visualize) override {
		*motorEffort = 0;
		for(int i=0; i<brain->nrOutputValues; i++)
			*motorEffort += brain->readOutput(i);
	};
};
