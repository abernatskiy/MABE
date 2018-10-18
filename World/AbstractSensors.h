#pragma once

#include "../Brain/AbstractBrain.h"

class AbstractSensors {

public:
	virtual void reset() = 0;
	virtual void attachToBrain(std::shared_ptr<AbstractBrain> br) = 0;
	virtual void update(int timeStep, int visualize) = 0;
	virtual int numOutputs() = 0;
	virtual int numInputs() { return 0; }; // reload for active perception
};
