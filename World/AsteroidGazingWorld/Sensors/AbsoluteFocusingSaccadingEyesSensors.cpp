#include "AbsoluteFocusingSaccadingEyesSensors.h"

#include "../../../Brain/AbstractBrain.h"

#include <cstdlib>

// Auxiliary definitions

static const std::vector<std::string> allowedParameters = { "condition", "distance", "phase" };

void validateParameterValues(std::map<std::string,std::set<unsigned>> pvals) {
	// Validation is based on conventions valid on Nov 5th 2018
	for( auto pname : allowedParameters ) {
		if( pvals.find(pname) == pvals.end() ) {
			std::cerr << "Required parameter " << pname << " not found in the snapshots file names" << std::endl;
			exit(EXIT_FAILURE);
		}
	}
	if( pvals.size() != allowedParameters.size() ) {
		std::cerr << "Too many parameters in the snapshots file names. Allowed parameter names are ";
		for( auto pname : allowedParameters )
			std::cerr << '"' << pname << '"' << ", ";
		std::cerr << std::endl;
		exit(EXIT_FAILURE);
	}
	for( auto pname : allowedParameters ) {
		if( pvals[pname].size() == 0 ) {
			std::cerr << "Required parameter " << pname << " has no known values. Likely a parsing error has occured. Exiting" << std::endl;
			exit(EXIT_FAILURE);
		}
	}
}

// AbsoluteFocusingSaccadingEyesSensors class definitions

AbsoluteFocusingSaccadingEyesSensors::AbsoluteFocusingSaccadingEyesSensors(std::shared_ptr<std::string> curAstName,
                                                                           std::shared_ptr<AsteroidsDatasetParser> datasetParser,
                                                                           unsigned res) :
	currentAsteroidName(curAstName), resolution(res), numSensors(getNumInputChannels()) {

	// Reading asteroid snapshots into RAM for quick access
	std::set<std::string> asteroidNames = datasetParser->getAsteroidsNames();
	for( auto an : asteroidNames ) {
		// Checking if we have all the params required and no extra stuff
		std::map<std::string,std::set<unsigned>> parameterValSets = datasetParser->getAllParameterValues(an);
		validateParameterValues(parameterValSets);

		// Converting parameter value sets into vectors - important for making a cartesian product of them later
		std::map<std::string,std::vector<unsigned>> parameterVals;
		for( auto pname : allowedParameters ) {
			parameterVals[pname] = std::vector<unsigned>();
			std::copy(parameterValSets[pname].begin(), parameterValSets[pname].end(), std::back_inserter(parameterVals[pname]));
		}

		// Iterating over the cartesian product of all parameter values arrays and reading the corresponding pictures
		// Cartesian product code credit: Matt @ stackoverflow.com/questions/5279051, heavily modified

		asteroidSnapshots[an] = std::map<unsigned,std::map<unsigned,std::map<unsigned,AsteroidSnapshot>>>();

//		std::cout << "Parameter combinations for asteroid " << an << std::endl;
		auto product = []( long long a, std::pair<std::string,std::vector<unsigned>> b ) { return a*(b.second).size(); };
		const long long numParameterCombinations = std::accumulate( parameterVals.begin(), parameterVals.end(), 1LL, product );

		std::vector<unsigned> currentParameterCombination(parameterVals.size());

		for(long long n=0; n<numParameterCombinations; n++) {
			std::lldiv_t q{ n, 0 };
			for(int i=allowedParameters.size()-1; i>=0; i--) {
				q = std::div( q.quot, parameterVals[allowedParameters[i]].size() );
				currentParameterCombination[i] = (parameterVals[allowedParameters[i]])[q.rem];
			}

			// Reading the snapshot corresponding to the current combination of parameters into the library
			const unsigned condition = currentParameterCombination[0];
			const unsigned distance = currentParameterCombination[1];
			const unsigned phase = currentParameterCombination[2];
			if( asteroidSnapshots[an].find(condition) == asteroidSnapshots[an].end() ) asteroidSnapshots[an][condition] = std::map<unsigned,std::map<unsigned,AsteroidSnapshot>>();
			if( asteroidSnapshots[an][condition].find(distance) == asteroidSnapshots[an][condition].end() ) asteroidSnapshots[an][condition][distance] = std::map<unsigned,AsteroidSnapshot>();
			if( asteroidSnapshots[an][condition][distance].find(phase) != asteroidSnapshots[an][condition][distance].end() ) {
				std::cerr << "Asteroid snapshot is read into the storage twice! Exiting" << std::endl;
				exit(EXIT_FAILURE);
			}
			else {
				const std::string snapshotPath = datasetParser->getPicturePath(an, condition, distance, phase);
				asteroidSnapshots[an][condition][distance].emplace(phase, snapshotPath);
			}
		}
  }
	// At this point all snapshots from the dataset is in RAM
}

unsigned AbsoluteFocusingSaccadingEyesSensors::getNumInputChannels() {
	// yes I know this is suboptimal
	unsigned pow = 1;
	for(unsigned i=0; i<resolution; i++)
		pow *= 4;
	return pow;
}

void AbsoluteFocusingSaccadingEyesSensors::update(int visualize) {

	// for now, just write ones to the sensory inputs of the brain
	for(unsigned i=0; i<numSensors; i++)
		brain->setInput(i, 1.);

	AbstractSensors::update(visualize); // increment the clock
}
