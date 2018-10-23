#pragma once

#include "../AbstractStateSchedule.h"

class TestStateSchedule : public AbstractStateSchedule {

private:
	const std::vector<int> testWorldStates = {5, 7, 3, 8, 2};
	std::shared_ptr<int> worldState;
	unsigned curState;
	void setWorldState(int stateIdx) { *worldState = stateIdx<testWorldStates.size() ? testWorldStates[stateIdx] : testWorldStates[testWorldStates.size()-1]; };

public:
	TestStateSchedule(std::shared_ptr<int> wState) : worldState(wState) {};
	void reset(int visualize) override { curState=0; setWorldState(curState); };
	void advance(int visualize) override { curState++; setWorldState(curState); };
	bool stateIsFinal() override { return curState>=testWorldStates.size(); };
};
