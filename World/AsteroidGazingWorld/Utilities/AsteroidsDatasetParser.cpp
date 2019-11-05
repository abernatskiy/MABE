#include "AsteroidsDatasetParser.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <regex>
#include <vector>

// Auxiliary function definition, see https://stackoverflow.com/questions/874134

bool hasEnding(std::string const &fullString, std::string const &ending) {

	if (fullString.length() >= ending.length())
		return (0 == fullString.compare(fullString.length()-ending.length(), ending.length(), ending));
	else
		return false;
}

// AsteroidDatasetParser class definitions

AsteroidsDatasetParser::AsteroidsDatasetParser(std::string datasetPath) :

	fsDatasetPath(fs::system_complete(datasetPath)),
	asteroidsNamesCacheFull(false) {
	if( !fs::exists(fsDatasetPath) ) { std::cerr << "Dataset path " << fsDatasetPath.string() << " does not exist" << std::endl; exit(EXIT_FAILURE); }
	if( !fs::is_directory(fsDatasetPath) ) { std::cerr << "Dataset path " << fsDatasetPath.string() << " is not a directory" << std::endl; exit(EXIT_FAILURE); }
}

std::set<std::string> AsteroidsDatasetParser::getAsteroidsNames() {

	if(asteroidsNamesCacheFull)
		return asteroidsNamesCache;

	fs::path namesCachePath = fsDatasetPath / fs::path("names.cache");
	if(fs::is_regular_file(namesCachePath)) {
		std::ifstream namesCacheStream(namesCachePath.string());
		std::string curName;
		while(std::getline(namesCacheStream, curName))
			asteroidsNamesCache.insert(curName);
	}
	else {
		// list all subdirs of the dataset dir
		fs::directory_iterator end_iter;
		for( fs::directory_iterator dir_itr(fsDatasetPath); dir_itr != end_iter; ++dir_itr ) {
			try {
				if( fs::is_directory(dir_itr->status()) )
					asteroidsNamesCache.insert(dir_itr->path().filename().string());
			}
			catch (const std::exception & ex) {
				std::cerr << "Exception " << ex.what() << " occurred while trying to list dataset folder " << fsDatasetPath << std::endl;
				exit(EXIT_FAILURE);
			}
		}

		// and write about what we found out to the cache file
		std::ofstream namesCacheOStream(namesCachePath.string());
		for(std::string s : asteroidsNamesCache)
			namesCacheOStream << s << std::endl;
	}

	asteroidsNamesCacheFull = true;

	return asteroidsNamesCache;
}

std::string AsteroidsDatasetParser::getICQPath(std::string asteroidName) {

	std::string icqpath = ( fsDatasetPath / fs::path(asteroidName) / fs::path("icq.txt") ).string();
	checkIfRegularFile(icqpath);
	return icqpath;
}

std::string AsteroidsDatasetParser::getDescriptionPath(std::string asteroidName) {
	std::string descpath = ( fsDatasetPath / fs::path(asteroidName) / fs::path("logfile.txt") ).string();
	checkIfRegularFile(descpath);
	return descpath;
}

const std::vector<command_type>& AsteroidsDatasetParser::cachingGetDescription(std::string asteroidName) {
	auto itDesc = descriptionCache.find(asteroidName);
	if(itDesc==descriptionCache.end()) {
		std::string descPath = getDescriptionPath(asteroidName);
		std::ifstream commandsFstream(descPath);
		std::string cline;
		std::vector<command_type> desc;
		while( std::getline(commandsFstream, cline) ) {
			command_type com;
			std::istringstream cstream(cline);
			for(auto it=std::istream_iterator<command_field_type>(cstream); it!=std::istream_iterator<command_field_type>(); it++)
				com.push_back(*it);
			desc.push_back(com);
		}
	  commandsFstream.close();
		descriptionCache[asteroidName] = desc;
//		for(const auto& com : desc) {
//			for(const auto& f : com)
//				std::cout << f << ' ';
//			std::cout << std::endl;
//		}
		itDesc = descriptionCache.find(asteroidName); // I am sleepy. Minimizing the number of possible points of failure
	}

	return itDesc->second;
}

std::set<std::string> AsteroidsDatasetParser::getAllPicturePaths(std::string asteroidName) {

	std::set<std::string> pictureName;
	fs::path asteroidDir(asteroidName);
	fs::directory_iterator end_iter;
	for( fs::directory_iterator dir_itr(fsDatasetPath/asteroidDir); dir_itr != end_iter; ++dir_itr ) {
		try {
			std::string filename = dir_itr->path().filename().string();
			if( fs::is_regular_file(dir_itr->status()) && hasEnding(filename, ".png") )
				pictureName.insert(dir_itr->path().string());
    } catch (const std::exception & ex) {
      std::cerr << "Exception " << ex.what() << " occurred while trying to list an asteroid subfolder " << (fsDatasetPath/asteroidDir) << std::endl;
      exit(EXIT_FAILURE);
    }
  }
	return pictureName;
}

std::string AsteroidsDatasetParser::getPicturePath(std::string asteroidName, unsigned condition, unsigned distance, unsigned phase) {

	std::ostringstream buffer;
	buffer << "condition" << condition << "_distance" << distance << "_phase" << std::setfill('0') << std::setw(4) << phase << ".png";
	std::string outpath = ( fsDatasetPath / fs::path(asteroidName) / fs::path(buffer.str()) ).string();
	checkIfRegularFile(outpath);
	return outpath;
}

std::map<std::string,std::set<unsigned>> AsteroidsDatasetParser::getAllParameterValues(std::string asteroidName) {

	// Listing all available pictures
	std::set<std::string> picPaths = getAllPicturePaths(asteroidName);

	// Picking one picture and determining parameter names from its filename
	std::string examplePicPath = *picPaths.begin();
	std::size_t filenameStarts = examplePicPath.find_last_of('/');
	std::string filename = examplePicPath.substr(filenameStarts+1);

	std::size_t current, previous = 0;
	current = filename.find('_');
	std::vector<std::string> paramNames;
	std::regex nameFinder("^([[:alpha:]]+)[0-9]+.*");
	std::smatch m;
	bool stop = false;
	while( !stop ) {
		if( current == std::string::npos ) stop = true;

		std::string paramRecord = filename.substr(previous, current-previous);
		std::regex_match(paramRecord, m, nameFinder);
		paramNames.push_back(m[1]);
		previous = current + 1;
		current = filename.find('_', previous);
	}

	// Scanning all the picture file names for parameter values, storing the results in the output array
	std::map<std::string,std::set<unsigned>> parameterValues;
	std::ostringstream buffer;
	for(std::string pn : paramNames) {
		parameterValues[pn] = std::set<unsigned>();
		buffer << pn << "([0-9]+)_";
	}
	std::regex paramValsExtractor(buffer.str().substr(0, buffer.str().size()-1));
	for(std::string picpath : picPaths) {
		std::size_t filenameBegins = picpath.find_last_of('/');
		std::size_t filenameEnds = picpath.find_last_of('.');
		std::string filename = picpath.substr(filenameBegins+1, filenameEnds-filenameBegins-1);

		std::regex_match(filename, m, paramValsExtractor);
		for(unsigned i=0; i<paramNames.size(); i++)
			parameterValues[paramNames[i]].insert(std::stoul(m[i+1]));
	}

	return parameterValues;

}

void AsteroidsDatasetParser::checkIfRegularFile(std::string path) {

	fs::path boostPath(path);
	if( !fs::is_regular_file(fs::path(path)) ) {
		std::cerr << "Path " << path << " does not lead to a regular file and it should" << std::endl;
		exit(EXIT_FAILURE);
	}
}
