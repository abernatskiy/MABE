#pragma once

#include "../Brain/AbstractBrain.h"

class AbstractSensors {

public:
	virtual void resetSensors() = 0;
	virtual void attachToBrain(std::shared_ptr<AbstractBrain> br) = 0;
	virtual void update(int timeStep, int visualize) = 0;
};
