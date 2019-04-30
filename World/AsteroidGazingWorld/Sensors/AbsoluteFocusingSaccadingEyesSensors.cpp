#include "AbsoluteFocusingSaccadingEyesSensors.h"

#include "../../../Brain/AbstractBrain.h"
#include "../Utilities/shades.h"

#include <fstream>
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

std::shared_ptr<ParameterLink<int>> AbsoluteFocusingSaccadingEyesSensors::foveaResolutionPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_ABSOLUTE_SACCADING_EYE-foveaResolution", 3,
                                 "number of rows and columns in the sensors fovea (resulting number of sensory inputs is r^2)");
std::shared_ptr<ParameterLink<int>> AbsoluteFocusingSaccadingEyesSensors::splittingFactorPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_ABSOLUTE_SACCADING_EYE-splittingFactor", 3,
                                 "the factor z that determines how zoom works, in particular the snapshot is divided into z^2 sub-areas at each zoom level; acceptable values are 2 and 3");
std::shared_ptr<ParameterLink<int>> AbsoluteFocusingSaccadingEyesSensors::maxZoomPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_ABSOLUTE_SACCADING_EYE-maxZoom", 3,
                                 "the number of allowed zoom levels");
std::shared_ptr<ParameterLink<int>> AbsoluteFocusingSaccadingEyesSensors::activeThresholdingDepthPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_ABSOLUTE_SACCADING_EYE-activeThresholdingDepth", -1,
                                 "number of bisections of the 0..255 interval that the sensors can make to get thresholds (e.g. 0..127 yields a threshold of 63), negatives meaning fixed threshold of 160");
std::shared_ptr<ParameterLink<bool>> AbsoluteFocusingSaccadingEyesSensors::lockAtMaxZoomPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_ABSOLUTE_SACCADING_EYE-lockAtMaxZoom", false,
                                 "should sensors be locked at max zoom level? If true, zooming out is impossible/disabled");
std::shared_ptr<ParameterLink<bool>> AbsoluteFocusingSaccadingEyesSensors::startZoomedInPL =
  Parameters::register_parameter("WORLD_ASTEROID_GAZING_ABSOLUTE_SACCADING_EYE-startZoomedIn", false,
                                 "should the default state of the nodes (0000...) correspond to max zoom level instead of the min level (no zoom)?");

AbsoluteFocusingSaccadingEyesSensors::AbsoluteFocusingSaccadingEyesSensors(std::shared_ptr<std::string> curAstName,
                                                                           std::shared_ptr<AsteroidsDatasetParser> dsParser,
                                                                           std::shared_ptr<ParametersTable> PT_) :
	currentAsteroidName(curAstName), datasetParser(dsParser),
	foveaResolution(foveaResolutionPL->get(PT_)),
	maxZoom(maxZoomPL->get(PT_)),
	splittingFactor(splittingFactorPL->get(PT_)),
	useConstantThreshold(activeThresholdingDepthPL->get(PT_)<0),
	activeThresholdingDepth(activeThresholdingDepthPL->get(PT_)<0 ? 0 : static_cast<unsigned>(activeThresholdingDepthPL->get(PT_))),
	lockAtMaxZoom(lockAtMaxZoomPL->get(PT_)),
	startZoomedIn(startZoomedInPL->get(PT_)),
	phaseControls(bitsFor(numPhases)),
	zoomLevelControls(lockAtMaxZoom ? 0 : maxZoom),
	zoomPositionControls(2*maxZoom*bitsFor(splittingFactor)),
	numSensors(getNumSensoryChannels()),
	numMotors(getNumControls()) {

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
				asteroidSnapshots[an][condition][distance].emplace(std::piecewise_construct,
				                                                   std::forward_as_tuple(phase),
				                                                   std::forward_as_tuple(snapshotPath, constantBinarizationThreshold)); // TODO: downsample based on allowed zoom levels for memory efficiency
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

	// Analyzing the dataset for learnability
	analyzeDataset();
}

std::uint8_t AbsoluteFocusingSaccadingEyesSensors::getThreshold(std::vector<bool>::iterator begin, std::vector<bool>::iterator end) {

	// I could use bitshifts for that, but chose not to - A.B.

	if(useConstantThreshold)
		return constantBinarizationThreshold;

	std::uint8_t min = 0;
	std::uint8_t max = 255;
	std::uint8_t mid;
	for(auto it=begin; it!=end; it+=2) {
		mid = min+(max-min)/2;
		if(!(*it))
			return mid;
		if(*(it+1))
			min = mid;
		else
			max = mid+1;
	}
	return mid;
}

unsigned AbsoluteFocusingSaccadingEyesSensors::getNumSensoryChannels() {
	return foveaResolution*foveaResolution; // TODO: implement color depth
}

unsigned AbsoluteFocusingSaccadingEyesSensors::getNumControls() {
	return conditionControls +
	       distanceControls +
	       phaseControls +
	       zoomLevelControls +
	       zoomPositionControls +
	       2*activeThresholdingDepth;
}

void AbsoluteFocusingSaccadingEyesSensors::update(int visualize) {
	// Converting the controls into a vector of bools
	std::vector<bool> controls(numMotors);
	for(unsigned i=0; i<numMotors; i++)
		controls[i] = brain->readOutput(i) > 0.5;

//	if(visualize) {
//		std::cout << "Control inputs of AbsoluteFocusingSaccadingEyes:";
//		for(int ci : controls)
//			std::cout << ' ' << ci;
//		std::cout << std::endl << std::flush;
//	}

	// Parsing it and getting the sensor values in meantime:
	//  1. Determining which snapshot we want to see
	std::vector<bool>::iterator controlsIter = controls.begin();
	const unsigned condition = constCondition;
	const unsigned distance = constDistance;
	const unsigned phase = bitsRangeToDecimal(controlsIter, controlsIter+=phaseControls);
	//     ... we now know it is asteroidSnapshots[*currentAsteroidName][condition][distance][phase]
	//     Let's get some useful params of that snapshot

//	if(visualize)
//		std::cout << "Snapshot summary: asteroid name " << *currentAsteroidName << ", condition " << condition << ", distance " << distance << ", phase " << phase << std::endl;

	AsteroidSnapshot& astSnap = asteroidSnapshots.at(*currentAsteroidName).at(condition).at(distance).at(phase);

//	if(visualize) {
//		std::cout << "Perceiving the following asteroid:" << std::endl;
//		astSnap.print();
//	}

	// 2. Determining which part of the snapshot we want to see
	const unsigned zoomLevel = lockAtMaxZoom ?
	                                           maxZoom :
	                                           (startZoomedIn ?
                                                              maxZoom-bitsRangeToZeroBiasedParallelBusValue(controlsIter, controlsIter+zoomLevelControls) :
                                                              bitsRangeToZeroBiasedParallelBusValue(controlsIter, controlsIter+zoomLevelControls));
	controlsIter += zoomLevelControls;
	unsigned x0, x1, y0, y1;

//	if(visualize) {
//		std::cout << "Using controls";
//		for(auto it=controlsIter; it!=controlsIter+zoomPositionControls/2; it++)
//			std::cout << ' ' << *it;
//		std::cout << " to split the range 0 to " << astSnap.width << " with a splitting factor of " << splittingFactor << " to a zoom level of " << zoomLevel << std::endl;
//	}

	auto foveaPosControlsStartIt = controlsIter;

	std::tie(x0, x1) = splittingFactor==3 ? triSplitRange( 0, astSnap.width, zoomLevel, controlsIter, controlsIter+(zoomPositionControls/2) ) :
	                                         biSplitRange( 0, astSnap.width, zoomLevel, controlsIter, controlsIter+(zoomPositionControls/2) );
	controlsIter += (zoomPositionControls/2);

//	if(visualize) {
//		std::cout << "Resulting range: " << x0 << ' ' << x1 << std::endl;
//		std::cout << "Using controls";
//		for(auto it=controlsIter; it!=controlsIter+zoomPositionControls/2; it++)
//			std::cout << ' ' << *it;
//		std::cout << " to split the range 0 to " << astSnap.height << " with a splitting factor of " << splittingFactor << " to a zoom level of " << zoomLevel << std::endl;
//	}

	std::tie(y0, y1) = splittingFactor==3 ? triSplitRange( 0, astSnap.height, zoomLevel, controlsIter, controlsIter+(zoomPositionControls/2) ) :
	                                         biSplitRange( 0, astSnap.height, zoomLevel, controlsIter, controlsIter+(zoomPositionControls/2) );
	controlsIter += (zoomPositionControls/2);

	auto foveaPosControlsEndIt = controlsIter;

	std::uint8_t threshold = getThreshold(controlsIter, controlsIter+2*activeThresholdingDepth);

//	if(visualize) {
//		std::cout << "Resulting range: " << y0 << ' ' << y1 << std::endl;
//	}

	// 3. Getting that part and feeding it to the brain line by line
	const AsteroidSnapshot& view = astSnap.cachingResampleArea(x0, y0, x1, y1, foveaResolution, foveaResolution, threshold);

//	if(visualize) {
//		std::cout << "Resulting view in full resolution:" << std::endl;
//		view.print(foveaResolution, true);
//	}

	unsigned pixNum = 0;
	savedPercept.clear();
//	if(visualize)
//		std::cout << "Thresholding at " << binarizationThreshold << " and printing the resulting bitmap" << std::endl;
	for(unsigned i=0; i<foveaResolution; i++) {
		for(unsigned j=0; j<foveaResolution; j++) {
			brain->setInput(pixNum++, view.getBinary(i,j));
			savedPercept.push_back(view.getBinary(i,j));
		}
	}
//	if(visualize)
//		view.printBinary();


//	if(visualize)
	{
		std::vector<unsigned> apr;
		apr.push_back(condition);
		apr.push_back(distance);
		apr.push_back(phase);
		apr.push_back(zoomLevel);
		for(auto it=foveaPosControlsStartIt; it!=foveaPosControlsEndIt; it++)
			apr.push_back(static_cast<unsigned>(*it));
		apr.push_back(static_cast<unsigned>(threshold));
		perceptionControlsTimeSeries.push_back(apr);
	}

	AbstractSensors::update(visualize); // increment the clock
}

void AbsoluteFocusingSaccadingEyesSensors::reset(int visualize) {
	AbstractSensors::reset(visualize);
	perceptionControlsTimeSeries.clear();
}

void AbsoluteFocusingSaccadingEyesSensors::analyzeDataset() {

	using namespace std;

	// Preparing a log file
	string logpath = Global::outputPrefixPL->get() + "datasetAnalysis.log";
	ofstream logfile(logpath);
	if(!logfile.is_open()) {
		cerr << "Could not open the dataset analysis log file " << logpath << endl;
		exit(EXIT_FAILURE);
	}

	// Determining and logging the dimensions of the perception system
	unsigned maxRes = foveaResolution;
	for(unsigned i=0; i<maxZoom; i++)
		maxRes *= splittingFactor;
	logfile << "Maximum visual system resolution is " << maxRes << "x" << maxRes << ":" << endl
	        << " - fovea resolution is " << foveaResolution << "x" << foveaResolution << endl
	        << " - at zoom the field is split " << splittingFactor << "-way up to " << maxZoom << " times" << endl;

	logfile << endl;

	// Checking if we're dealing with a dataset that is too complex to analyze at the moment
	set<tuple<unsigned,unsigned,unsigned>> astViews;
	for(const auto& astRec : asteroidSnapshots) {
		set<tuple<unsigned,unsigned,unsigned>> views;
		for(const auto& condRec : astRec.second)
			for(const auto& distRec : condRec.second)
				for(const auto& phaseRec : distRec.second)
					views.insert(make_tuple(condRec.first, distRec.first, phaseRec.first));
		if(views.size() != 1) {
			logfile << "Number of views available for asteroid " << astRec.first << " is not one (" << views.size()
			        << "). Analysis of such datasets is not implemented yet, exiting the analyzer." << endl;
			logfile.close();
			return;
		}
		astViews.insert(*views.begin());
	}
	if(astViews.size() != 1) {
		logfile << "Different views are available for different asteroids (a total of " << astViews.size()
		        << " views). Analysis of such datasets is not implemented yet, exiting the analyzer." << endl;
		logfile.close();
		return;
	}
	const unsigned theCondition = get<0>(*astViews.begin());
	const unsigned theDistance = get<1>(*astViews.begin());
	const unsigned thePhase = get<2>(*astViews.begin());
	logfile << "Dataset supported by the analyzer: all asteroids are available at condition " << theCondition << ", distance " << theDistance << ", phase " << thePhase << endl;

	logfile << endl;

	// Obtaining the list of distinct percepts, annotated with asteroid names
	vector<AsteroidSnapshot> perceptShots;
	vector<vector<string>> perceptAsteroidNames;
	for(const auto& astRec : asteroidSnapshots) {
		const auto& currentShot = astRec.second.at(theCondition).at(theDistance).at(thePhase);
		const auto& currentPerceptShot = currentShot.resampleArea(0, 0, currentShot.width, currentShot.height, maxRes, maxRes, currentShot.binarizationThreshold);

		bool perceptFound = false;
		for(unsigned i=0; i<perceptShots.size(); i++) {
			if(perceptShots[i].binaryIsTheSame(currentPerceptShot)) {
				perceptAsteroidNames[i].push_back(astRec.first);
				perceptFound = true;
				break;
			}
		}

		if(!perceptFound) {
			perceptShots.push_back(currentPerceptShot);
			perceptAsteroidNames.push_back({astRec.first});
		}
	}

	// Logging the distribution of percepts
	unsigned maxAsteroidsPerPercept = max_element(perceptAsteroidNames.begin(), perceptAsteroidNames.end(),
	                                              [](vector<string> a, vector<string> b) {return a.size()<b.size();} )->size();
	vector<unsigned> perceptsHistogram(maxAsteroidsPerPercept+1, 0);
	for(unsigned i=0; i<perceptShots.size(); i++)
		perceptsHistogram[perceptAsteroidNames[i].size()] += 1;
	logfile << "Distribution of percept frequencies:" << endl;
	for(unsigned i=1; i<maxAsteroidsPerPercept+1; i++)
		logfile << perceptsHistogram[i] << " percepts encountered " << i << " times" << endl;

	logfile << endl;

	// Counting common sculpting commands for each percept
	vector<set<command_type>> commandIntersections(perceptShots.size()); // command_type is defined at AsteroidsDatasetParser.h
	unsigned numCommands;
	for(unsigned p=0; p<perceptShots.size(); p++) {
		{
			ifstream initialCommandsStream(datasetParser->getDescriptionPath(perceptAsteroidNames[p][0]));
			string cline;
			while(getline(initialCommandsStream, cline)) {
				stringstream cstream(cline);
				command_type com;
				for(auto it=istream_iterator<command_field_type>(cstream); it!=istream_iterator<command_field_type>(); it++)
					com.push_back(*it);
				commandIntersections[p].insert(com);
			}
			initialCommandsStream.close();

			if(p==0)
				numCommands = commandIntersections[p].size();
			else if(commandIntersections[p].size()!=numCommands) {
				logfile << "Variation in number of commands detected. Such datasets are not yet sopported. Exiting analyzer" << endl;
				logfile.close();
				return;
			}
		}

		for(unsigned j=1; j<perceptAsteroidNames[p].size(); j++) {
			set<command_type> curCommands;

			ifstream currentCommandsStream(datasetParser->getDescriptionPath(perceptAsteroidNames[p][j]));
			string cline;
			while(getline(currentCommandsStream, cline)) {
				stringstream cstream(cline);
				command_type com;
				for(auto it=istream_iterator<command_field_type>(cstream); it!=istream_iterator<command_field_type>(); it++)
					com.push_back(*it);
				curCommands.insert(com);
			}
			currentCommandsStream.close();

			if(curCommands.size()!=numCommands) {
				logfile << "Variation in number of commands detected. Such datasets are not yet sopported. Exiting analyzer" << endl;
				logfile.close();
				return;
			}

			vector<set<command_type>::iterator> toRemove;
			for(auto it=commandIntersections[p].begin(); it!=commandIntersections[p].end(); it++)
				if(curCommands.find(*it)==curCommands.end())
					toRemove.push_back(it);
			for(const auto& rit : toRemove)
				commandIntersections[p].erase(rit);
		}
	}

	// Computing and logging the maximum number of commands that can be recovered with the available percepts
	unsigned numRecoverableCommands = 0;
	for(unsigned i=0; i<perceptShots.size(); i++)
		numRecoverableCommands += numCommands + commandIntersections[i].size()*(perceptAsteroidNames[i].size()-1);
	logfile << "Total number of recoverable commands is " << numRecoverableCommands
	        << " out of " << numCommands*asteroidSnapshots.size()
	        << " (" << numCommands*perceptShots.size() << " from unique percepts plus "
	        << numRecoverableCommands-numCommands*perceptShots.size() << " by giving the same answer on same percept)" << endl;
	logfile << endl;

	// Logging the list of different percepts
	logfile << "List of distinct percepts:" << endl << endl;
	for(unsigned i=0; i<perceptShots.size(); i++) {
		logfile << "Percept " << i << ", corresponding to asteroids";
		for(const auto& astName : perceptAsteroidNames[i])
			logfile << " " << astName;
		logfile << " (which have " << commandIntersections[i].size() << " common intersecting commands:";
		for(const auto& comCom : commandIntersections[i]) {
			for(const auto& comField : comCom)
				logfile << " " << comField;
			logfile << ",";
		}
		logfile << ")" << endl;
		logfile << perceptShots[i].getPrintedBinary();
	}

	// Closing the log file
	logfile.close();
}

void* AbsoluteFocusingSaccadingEyesSensors::logTimeSeries(const std::string& label) {

	std::ofstream ctrlog(std::string("sensorControls_") + label + std::string(".log"));
	for(const auto& apr : perceptionControlsTimeSeries) {
		for(auto it=apr.begin(); it!=apr.end(); it++)
			ctrlog << *it << ( it==apr.end()-1 ? "" : " " );
		ctrlog << std::endl;
	}
	ctrlog.close();

	return nullptr;
}

unsigned AbsoluteFocusingSaccadingEyesSensors::numSaccades() {

	std::set<std::vector<unsigned>> saccades;
	for(const auto& sc : perceptionControlsTimeSeries)
		saccades.insert(sc);
	return saccades.size();
}
