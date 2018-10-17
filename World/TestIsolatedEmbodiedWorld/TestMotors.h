#pragma once

#include "../AbstractMotors.h"

using namespace std;

class TestMotors : public AbstractMotors {

public:
	std::shared_ptr<int> motorEffort;
	std::shared_ptr<AbstractBrain> brain;

	TestMotors(std::shared_ptr<int> mEffort) : AbstractMotors(), motorEffort(mEffort) {};

	void resetMotors() override {}; // no need to change the internal state of the motors - motorEffort belongs to the world
	void attachToBrain(std::shared_ptr<AbstractBrain> br) override { brain=br; };
	void update(int timeStep, int visualize) override {
		*motorEffort = 0;
//		cout << "Updating the motor effort with the follwing brain outputs:";
		for(int i=0; i<brain->nrOutputValues; i++) {
//			cout << " " << brain->readOutput(i);
			*motorEffort += brain->readOutput(i)>0 ? 1 : 0;
		}
//		cout << endl;
	};
};
