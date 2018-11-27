#include "AbsoluteFocusingSaccadingEyesSensors.h"

#include "../../../Brain/AbstractBrain.h"

#include <cstdint>
#include <cstdlib>

/***** Auxiliary definitions *****/

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

unsigned bitsFor(unsigned val) {
	// ceil(log2(val))
	unsigned upper = 1;
	unsigned bits = 0;
	while( upper < val ) { upper *= 2; bits++; }
  return bits;
}

unsigned bitsRangeToDecimal(std::vector<bool>::iterator begin, std::vector<bool>::iterator end) {
	unsigned val = 0;
	while( begin != end ) {
		val <<= 1;
		val += *begin ? 1 : 0;
		begin++;
	}
	return val;
}

unsigned bitsRangeToZeroBiasedParallelBusValue(std::vector<bool>::iterator begin, std::vector<bool>::iterator end) {
//	std::cout << "Converting the bits";
//	for(auto b=begin; b!=end; b++)
//		std::cout << " " << *b;
//	std::cout << " into an integer.";

	unsigned val = 0;
	while( begin != end ) {
		if( !*begin ) return val;
		begin++; val++;
	}
	return val;
}

std::pair<unsigned,unsigned> triSplitRange(unsigned x0, unsigned x1, unsigned splits, std::vector<bool>::iterator begin, std::vector<bool>::iterator end) {
	// returns a part of the segment [x0, x1) resulting from "splits" trinary splits, with the part choice dictated by the boolean input values at every split
	unsigned choice, range, rangeStart;
	while( splits != 0 ) {
		if(*begin)
			choice = *(begin+1) ? 2 : 0; // if the first bit is 1, the second one is used to choose from the top and the bottom part
		else
			choice = 1; // if the first bit is 0, second one is ignored and middle part is chosen
		rangeStart = x0;
		range = x1-x0;
		x0 = rangeStart + (range*choice) / 3;
		x1 = rangeStart + (range*(choice+1)) / 3;
		splits--;
		begin+=2;
	}
	return std::pair<unsigned,unsigned>(x0, x1);
}

std::pair<unsigned,unsigned> biSplitRange(unsigned x0, unsigned x1, unsigned splits, std::vector<bool>::iterator begin, std::vector<bool>::iterator end) {
  // returns a part of the segment [x0, x1) resulting from "splits" binary splits, with the part choice dictated by the boolean input values at every split
	unsigned choice, range, rangeStart;
	while( splits != 0 ) {
		choice = *begin ? 1 : 0;
		rangeStart = x0;
		range = x1-x0;
		x0 = rangeStart + (range*choice) / 2;
		x1 = rangeStart + (range*(choice+1)) / 2;
		splits--;
		begin++;
	}
	return std::pair<unsigned,unsigned>(x0, x1);
}

/***** AbsoluteFocusingSaccadingEyesSensors class definitions *****/

AbsoluteFocusingSaccadingEyesSensors::AbsoluteFocusingSaccadingEyesSensors(std::shared_ptr<std::string> curAstName,
                                                                           std::shared_ptr<AsteroidsDatasetParser> datasetParser,
                                                                           unsigned fovRes, unsigned mzoom, unsigned splitFac) :
	currentAsteroidName(curAstName), foveaResolution(fovRes), maxZoom(mzoom), splittingFactor(splitFac),
	phaseControls(bitsFor(numPhases)), zoomLevelControls(maxZoom), zoomPositionControls(2*maxZoom*bitsFor(splittingFactor)),
	numSensors(getNumSensoryChannels()), numMotors(getNumControls()) {

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
				asteroidSnapshots[an][condition][distance].emplace(phase, snapshotPath); // TODO: downsample based on allowed zoom levels
			}
		}
  }
	// At this point all snapshots from the dataset is in RAM

	// Defining the temporaries
	constCondition = getACondition(asteroidSnapshots);
	constDistance = getADistance(asteroidSnapshots);

	// Ruling out unsupported values of splittingFactor
	if( splittingFactor!=2 && splittingFactor!=3 ) {
		std::cerr << "Unsupported splitting factor of " << splittingFactor << " has been requested" << std::endl;
		exit(EXIT_FAILURE);
	}
}

unsigned AbsoluteFocusingSaccadingEyesSensors::getNumSensoryChannels() {
	return foveaResolution*foveaResolution; // TODO: implement color depth
}

unsigned AbsoluteFocusingSaccadingEyesSensors::getNumControls() {
	return conditionControls +
	       distanceControls +
	       phaseControls +
	       zoomLevelControls +
	       zoomPositionControls;
}

void AbsoluteFocusingSaccadingEyesSensors::update(int visualize) {
//	visualize = 1; // enable for debug

	// Converting the controls into a vector of bools
	std::vector<bool> controls(numMotors);
	for(unsigned i=0; i<numMotors; i++)
		controls[i] = brain->readOutput(i) > 0.5;

	if(visualize) {
		std::cout << "Control inputs of AbsoluteFocusingSaccadingEyes:";
		for(int ci : controls)
			std::cout << ' ' << ci;
		std::cout << std::endl << std::flush;
	}

	// Parsing it and getting the sensor values in meantime:
	//  1. Determining which snapshot we want to see
	std::vector<bool>::iterator controlsIter = controls.begin();
	const unsigned condition = constCondition;
	const unsigned distance = constDistance;
	const unsigned phase = bitsRangeToDecimal(controlsIter, controlsIter+=phaseControls);
	//     ... we now know it is asteroidSnapshots[*currentAsteroidName][condition][distance][phase]
	//     Let's get some useful params of that snapshot

	if(visualize) {
		std::cout << "Snapshot summary: asteroid name " << *currentAsteroidName << ", condition " << condition << ", distance " << distance << ", phase " << phase << std::endl;
	}

	AsteroidSnapshot& astSnap = asteroidSnapshots.at(*currentAsteroidName).at(condition).at(distance).at(phase);

	if(visualize) {
		std::cout << "Perceiving the following asteroid:" << std::endl;
		astSnap.print();
	}

	// 2. Determining which part of the snapshot we want to see
	const unsigned zoomLevel = bitsRangeToZeroBiasedParallelBusValue(controlsIter, controlsIter+zoomLevelControls);
	controlsIter += zoomLevelControls;
	unsigned x0, x1, y0, y1;

	if(visualize) {
		std::cout << "Using controls";
		for(auto it=controlsIter; it!=controlsIter+zoomPositionControls/2; it++)
			std::cout << ' ' << *it;
		std::cout << " to split the range 0 to " << astSnap.width << " with a splitting factor of " << splittingFactor << " to a zoom level of " << zoomLevel << std::endl;
	}

	std::tie(x0, x1) = splittingFactor==3 ? triSplitRange( 0, astSnap.width, zoomLevel, controlsIter, controlsIter+(zoomPositionControls/2) ) :
	                                         biSplitRange( 0, astSnap.width, zoomLevel, controlsIter, controlsIter+(zoomPositionControls/2) );
	controlsIter += (zoomPositionControls/2);

	if(visualize) {
		std::cout << "Resulting range: " << x0 << ' ' << x1 << std::endl;
		std::cout << "Using controls";
		for(auto it=controlsIter; it!=controlsIter+zoomPositionControls/2; it++)
			std::cout << ' ' << *it;
		std::cout << " to split the range 0 to " << astSnap.height << " with a splitting factor of " << splittingFactor << " to a zoom level of " << zoomLevel << std::endl;
	}

	std::tie(y0, y1) = splittingFactor==3 ? triSplitRange( 0, astSnap.height, zoomLevel, controlsIter, controlsIter+(zoomPositionControls/2) ) :
	                                         biSplitRange( 0, astSnap.height, zoomLevel, controlsIter, controlsIter+(zoomPositionControls/2) );
	controlsIter += (zoomPositionControls/2);

	if(visualize) {
		std::cout << "Resulting range: " << y0 << ' ' << y1 << std::endl;
	}

	// 3. Getting that part and feeding it to the brain line by line
	AsteroidSnapshot view = astSnap.resampleArea(x0, y0, x1, y1, foveaResolution, foveaResolution);

	if(visualize) {
		std::cout << "Resulting view in full resolution:" << std::endl;
		view.print(foveaResolution);
	}

	unsigned pixNum = 0;
	const unsigned contrastLvl = 10; // 127 is the middle of the dynamic range
	for(unsigned i=0; i<foveaResolution; i++)
		for(unsigned j=0; j<foveaResolution; j++) {
			// std::cout << (int) (view.get(i,j)>contrastLvl) << ' ';
			brain->setInput(pixNum++, view.get(i,j)>contrastLvl);
		}
	// std::cout << std::endl;

	AbstractSensors::update(visualize); // increment the clock
}
