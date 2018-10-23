#pragma once

#include "../AbstractMotors.h"

using namespace std;

class TestMotors : public AbstractMotors {

public:
	std::shared_ptr<int> motorEffort;

	TestMotors(std::shared_ptr<int> mEffort) : AbstractMotors(), motorEffort(mEffort) {};

	void reset(int visualize) override { AbstractMotors::reset(visualize); }; // no need to change the internal state of the motors - motorEffort belongs to the world
	void update(int visualize) override {
		AbstractMotors::update(visualize);
		*motorEffort = 0;
//		cout << "Updating the motor effort with the follwing brain outputs:";
		for(int i=0; i<brain->nrOutputValues; i++) {
//			cout << " " << brain->readOutput(i);
			*motorEffort += brain->readOutput(i)>0 ? 1 : 0;
		}
//		cout << endl;
	};
	int numInputs() { return 3; };
};