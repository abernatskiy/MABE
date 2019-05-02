#pragma once

#include <string>
#include <fstream>
#include <vector>

inline double pointEntropy(double p) {
	if(p == 0. || p == 1.)
		return 0;
	else
		return -p*log2(p)-(1.-p)*log2(1.-p);
}

class LogFile {
private:
	int fileSuffix;
	bool isOpen;
	std::ofstream fs;
	bool fileExists(std::string fn) {
		struct stat buffer;
		return (stat (fn.c_str(), &buffer) == 0); // stackoverflow.com/questions/12774207
	};
	std::string getNextAvailableFile(std::string prefix) {
		std::string candidateFN;
		while(1) {
			candidateFN = prefix + std::to_string(fileSuffix) + ".log";
			if(fileExists(candidateFN)) {
				fileSuffix++;
			} else {
				return candidateFN;
			}
		}
	};

	std::vector<std::vector<unsigned>> nodeStatesTS; // holds time series of the current brain "run"
	std::vector<std::vector<unsigned>> nodeStatesHeap; // holds time series from all brain runs lumped together
	std::vector<std::vector<unsigned>> nodePairsStatesTS;

public:
	LogFile() : fileSuffix(0), isOpen(false) {};
	void open(std::string prefix) {
		if (!isOpen) {
			std::string availableFile = getNextAvailableFile(prefix);
			fs.open(availableFile, std::ios::out); // std::ios::out|std::ios::app for append
			isOpen = true;
		}
	};
	~LogFile() { if (isOpen) fs.close(); };
	void log(std::string s) { if (isOpen) fs << s; };

	void logStateBeforeUpdate(const std::vector<double>& nodes) {
		// First, good old file logging
		fs << "Transition:" << std::setprecision(1);
    for(const auto& s : nodes)
      fs << ' ' << s;
    fs << " ->";

		// Next, remembering the initial state for future processing
		const bool rememberInitialState = true;
		bool nodeStatesHeapIsEmpty = (nodeStatesHeap.size()==0);
		if(nodeStatesTS.size()==0) {
			unsigned counter = 0;
			for(const auto& s : nodes) {
				if(rememberInitialState)
					nodeStatesTS.push_back({s>0 ? 1 : 0});
				else
					nodeStatesTS.push_back({});
				if(nodeStatesHeapIsEmpty)
					if(rememberInitialState)
						nodeStatesHeap.push_back({s>0 ? 1 : 0});
					else
						nodeStatesHeap.push_back({});
				else
					if(rememberInitialState)
						nodeStatesHeap[counter].push_back(s>0 ? 1 : 0);
				counter++;
			}
		}
	};

	void logStateAfterUpdate(const std::vector<double>& nodes) {
		// Complimentary logging after the update
		fs << std::setprecision(1);
		for(const auto& s : nodes)
			fs << ' ' << s;
		fs << std::endl;

		// Remembering the current state
		for(unsigned i=0; i<nodes.size(); i++) {
			nodeStatesTS[i].push_back(nodes[i]>0 ? 1 : 0);
			nodeStatesHeap[i].push_back(nodes[i]>0 ? 1 : 0);
		}
	};

	void logBrainReset() {
		fs << "Brain was reset" << std::endl;
		if( nodeStatesTS.size()>0 && nodeStatesHeap.size()>0 ) {
			logTSEntropy("Current node state", nodeStatesTS);
			fs << "Current node state is averaged over a trajectory " << nodeStatesTS[0].size() << " states long" << std::endl;
			nodeStatesTS.clear();

			logTSEntropy("Cumulative node state", nodeStatesHeap);
			fs << "Cumulatives are over " << nodeStatesHeap[0].size() << " measured states" << std::endl;
		}
		else
			fs << "No states were recorded before the reset, so probabilities/entropies logged" << std::endl;
	};

	void logTSEntropy(std::string label, const std::vector<std::vector<unsigned>>& ts) {
		fs << label << " probabilities of one:";
		fs << std::setprecision(2);
		std::vector<double> probsOfOne;
		for(auto nsts : ts) {
			unsigned sumOfStates = std::accumulate(nsts.begin(), nsts.end(), 0);
			double prob = static_cast<double>(sumOfStates)/static_cast<double>(nsts.size());
			probsOfOne.push_back(prob);
			fs << ' ' << prob;
		}
		fs << std::endl;

		fs << label << " enthropies:";
		fs << std::setprecision(2);
		for(auto p : probsOfOne) {
			fs << ' ' << pointEntropy(p);
		}
		fs << std::endl;
	};
};
