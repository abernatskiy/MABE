// This .cpp file is purely to aid build scripts. Might be removed if the experiments show it is not needed

#include "TestImaginationWorld.h"
#include "TestMentalImage.h"
#include "TestSensorsForImaginationWorld.h"

TestImaginationWorld::TestImaginationWorld(std::shared_ptr<ParametersTable> PT_) : AbstractImaginationWorld(PT_) {

	trueState = std::make_shared<int>(testTrueStates[0]);

	sensors = std::make_shared<TestSensorsForImaginationWorld>(trueState, shift);
	mentalImage = std::make_shared<TestMentalImage>(trueState);
	makeMotors();
}
