#pragma once

#include "../Brain/AbstractBrain.h"

class AbstractMotors {

protected:
	std::shared_ptr<AbstractBrain> brain;

public:
	void attachToBrain(std::shared_ptr<AbstractBrain> br) { brain=br; };

	// Overloading of all four private methods below is encouraged, but call the prototypes withing the extensions for the void ones
	virtual void reset(int visualize) { clock=0; };
	virtual void update(int visualize) { clock++; };
	virtual int numInputs() = 0;
	virtual int numOutputs() { return 0; }; // reload for proprioception

private:
	unsigned long clock;
};
