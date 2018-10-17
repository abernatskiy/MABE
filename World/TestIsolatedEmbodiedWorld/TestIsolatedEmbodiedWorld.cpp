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

void TestIsolatedEmbodiedWorld::evaluateOrganism(std::shared_ptr<Organism> currentOrganism, int visualize) {

	float scoreAccum = 0.;
//	cout << "Final scores:";
	for(int v : scores) {
		scoreAccum += v;
//		cout << " " << v;
	}
	scoreAccum /= scores.size();

//	cout << " average: " << scoreAccum << endl;

	currentOrganism->dataMap.append("score", scoreAccum);
}
