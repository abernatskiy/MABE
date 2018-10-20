#pragma once

#include <vector>

#include "../AbstractImaginationWorld.h"
#include "TestMentalImage.h"
#include "TestSensorsForImaginationWorld.h"

class TestImaginationWorld : public AbstractImaginationWorld {

	// The task here is to learn to subtract a constant shift from the sensory data:
	// Sensors provide the binary representation of the true state of the world

private:
	const unsigned long evaluationTimePerTest = 10;
	const int shift = 2;
	const std::vector<int> testTrueStates = {5, 7, 3, 8, 2}; // perfect performance is 0.936857 due to transients
//	const std::vector<int> testTrueStates = {0, 4, 1, 6, 9};
	std::shared_ptr<int> trueState;

	unsigned curTest(unsigned long timeStep) { return static_cast<unsigned>(timeStep/evaluationTimePerTest); };

	void updateExtraneousWorld(unsigned long timeStep, int visualize) override { *trueState = testTrueStates[curTest(timeStep)]; };
	bool endEvaluation(unsigned long ts) override { return ts >= evaluationTimePerTest*testTrueStates.size(); };

public:
	TestImaginationWorld(std::shared_ptr<ParametersTable> PT_);
};
