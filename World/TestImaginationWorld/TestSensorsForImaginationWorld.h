#pragma once

#include "../AbstractSensors.h"

class TestSensorsForImaginationWorld : public AbstractSensors {

	// Sensors return a binary representation of the world state plus the shift

private:
	const int digits = 4;
	const int shift;
	std::shared_ptr<int> worldState;

public:
	TestSensorsForImaginationWorld(std::shared_ptr<int> wState, int sh) : worldState(wState), shift(sh) {};
	void reset() override {}; // sensors themselves are stateless
	void update(int timeStep, int visualize) override {
		int outnum = *worldState + shift;
		for(int i=0; i<digits; i++)
			brain->setInput(i, (outnum>>i) % 2);
	};
	int numOutputs() override { return digits; };
};
