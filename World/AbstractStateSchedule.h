#pragma once

class AbstractStateSchedule {

public:
	virtual void reset(int visualize) = 0;
	virtual void advance(int visualize) = 0;
	virtual bool stateIsFinal() = 0;
};
