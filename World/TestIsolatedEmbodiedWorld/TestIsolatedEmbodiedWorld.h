#pragma once

#include <iostream>
#include <vector>

#include "../AbstractIsolatedEmbodiedWorld.h"
#include "TestMotors.h"
#include "TestSensors.h"

using namespace std;

class TestIsolatedEmbodiedWorld : public AbstractIsolatedEmbodiedWorld {

	// Figure out the state of the world based on a redundant encoding supplied by sensors
	// If it is low, do not actuate actuators
	// If it is high, actuate as many as you can
	// State of the world changes once

public:
	const unsigned evaluationTime = 10;
	const unsigned silenceTime = 6;

	std::shared_ptr<int> worldState;
	std::shared_ptr<int> motorEffort;
	int curScore;
	std::vector<int> scores;

	TestIsolatedEmbodiedWorld(std::shared_ptr<ParametersTable> PT_);

	int numInputs() override { return 3; };
	int numOutputs() override { return 3; };

	void resetWorld() override { *worldState=0; *motorEffort=0; curScore=0; scores.clear(); };
	bool endEvaluation(unsigned long ts) override { return ts >= evaluationTime; };
	void updateExtraneousWorld(int ts, int visualize) {  *worldState = ts>=silenceTime ? 1 : 0; };
	void updateRunningScores(int ts, int visualize) { curScore += *worldState==0 ? -1*(*motorEffort) : *motorEffort; };
	void recordFinalScores(int tott, int visualize) { scores.push_back(curScore); };

	void evaluateOrganism(std::shared_ptr<Organism> currentOrganism);
};
