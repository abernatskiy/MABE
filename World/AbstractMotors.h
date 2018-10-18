#pragma once

#include "../Brain/AbstractBrain.h"

class AbstractMotors {

public:
	virtual void reset() = 0;
	virtual void attachToBrain(std::shared_ptr<AbstractBrain> br) = 0;
	virtual void update(int timeStep, int visualize) = 0;
	virtual int numInputs() = 0;
	virtual int numOutputs() { return 0; }; // reload for proprioception
};
