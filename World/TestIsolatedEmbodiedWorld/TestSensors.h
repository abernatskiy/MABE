#pragma once

#include "../AbstractSensors.h"

class TestSensors : public AbstractSensors {

public:
	std::shared_ptr<int> worldState;

	TestSensors(std::shared_ptr<int> wState) : AbstractSensors(), worldState(wState) {};

	void reset() override {};
	void update(int timeStep, int visualize) override {
		if(*worldState==0) {
			brain->setInput(0, 1);
			brain->setInput(1, 1);
			brain->setInput(2, 0);
		}
		else {
			brain->setInput(0, 0);
			brain->setInput(1, 1);
			brain->setInput(2, 1);
		}
	};
	int numOutputs() { return 3; };
};
