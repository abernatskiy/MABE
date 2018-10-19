#pragma once

#include "../Brain/AbstractBrain.h"

class AbstractSensors {

protected:
	std::shared_ptr<AbstractBrain> brain;

public:
	void attachToBrain(std::shared_ptr<AbstractBrain> br) { brain=br; };

	virtual void reset() = 0;
	virtual void update(int timeStep, int visualize) = 0;
	virtual int numOutputs() = 0;
	virtual int numInputs() { return 0; }; // reload for active perception
};
