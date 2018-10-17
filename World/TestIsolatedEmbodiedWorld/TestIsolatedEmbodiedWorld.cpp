#include "TestIsolatedEmbodiedWorld.h"
#include "TestMotors.h"
#include "TestSensors.h"

TestIsolatedEmbodiedWorld::TestIsolatedEmbodiedWorld(std::shared_ptr<ParametersTable> PT_) : AbstractIsolatedEmbodiedWorld(PT_) {

	worldState = std::make_shared<int>(0);
	motorEffort = std::make_shared<int>(0);

	motors = std::make_shared<TestMotors>(motorEffort);
	sensors = std::make_shared<TestSensors>(worldState);

	curScore = 0;
}

void TestIsolatedEmbodiedWorld::evaluateOrganism(std::shared_ptr<Organism> currentOrganism) {

	float scoreAccum = 0.;
	for(int v : scores)
		scoreAccum += v;
	scoreAccum /= evaluationTime;

	currentOrganism->dataMap.append("score", scoreAccum);
}
