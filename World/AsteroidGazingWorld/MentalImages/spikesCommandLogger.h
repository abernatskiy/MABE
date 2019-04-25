#pragma once

#include <string>
#include <fstream>

typedef std::tuple<unsigned,unsigned,unsigned> CommandType;

class CommandLogger {

public:
	CommandLogger(std::string logName) : logname(logName) {};
	~CommandLogger() { if(logfile.is_open()) logfile.close(); };
	void open() { if(!logfile.is_open()) logfile.open(logname); } // unless this is called the object will ignore subsequent calls to log...() functions
	void logMapping(const std::vector<CommandType>& ins, const std::vector<std::vector<CommandType>>& outs) {
		if(!logfile.is_open()) return;

		logfile << "{ \"inputs\": ";
		logCommands(ins);
		logfile << ", \"outputs\": [";
		for(auto it=outs.cbegin(); it!=outs.cend(); it++) {
			logCommands(*it);
			logfile << ( it!=outs.cend()-1 ? "," : "" );
		}
		logfile << "] }" << std::endl;
	}

	void logMessage(std::string msg) {
		if(!logfile.is_open()) return;
		logfile << msg << std::endl;
	}

private:
	std::string logname;
	std::ofstream logfile;

	void logCommand(const CommandType& cmd) {
		if(!logfile.is_open()) return;

		unsigned f, i, j;
		std::tie(f, i, j) = cmd;
		logfile << '[' << f << ',' << i << ',' << j << ']';
	}

	void logCommands(const std::vector<CommandType>& cmds) {
		if(!logfile.is_open()) return;

		logfile << '[';
		for(auto it=cmds.cbegin(); it!=cmds.cend(); it++) {
			logCommand(*it);
			logfile << ( it!=cmds.cend()-1 ? "," : "" );
		}
		logfile << ']';
	}
};


