#pragma once

#include <string>

class AbstractStateSchedule {

public:
	virtual void reset(int visualize) = 0;
	virtual void advance(int visualize) = 0;
	virtual bool stateIsFinal() = 0;
	virtual const std::string& currentStateDescription() = 0;
};
