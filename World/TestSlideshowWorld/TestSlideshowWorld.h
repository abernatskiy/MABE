#pragma once

#include <vector>

#include "../AbstractSlideshowWorld.h"

// Grab these two from ../TestImaginationWorld/ if you're reworking this example into something practical
#include "../TestImaginationWorld/TestMentalImage.h"
#include "../TestImaginationWorld/TestSensorsForImaginationWorld.h"

#include "TestStateSchedule.h"

class TestSlideshowWorld : public AbstractSlideshowWorld {

	// The task here is similar to TestImaginationWorld: the agent must learn to subtract a constant
	// shift from the sensory data, with sensors providing binary representation of the true state
	// of the world. The difference is that a schedule is used to govern the changes of the world state.

private:
	const int shift = 2;

	std::shared_ptr<int> trueState;

	bool resetAgentBetweenStates() override { return true; };
	int brainUpdatesPerWorldState() override { return 10; };

public:
	TestSlideshowWorld(std::shared_ptr<ParametersTable> PT_) : AbstractSlideshowWorld(PT_) {
		trueState = std::make_shared<int>(0);
		sensors = std::make_shared<TestSensorsForImaginationWorld>(trueState, shift);
		mentalImage = std::make_shared<TestMentalImage>(trueState);
		makeMotors();
		stateSchedule = std::make_shared<TestStateSchedule>(trueState);
	};
};
