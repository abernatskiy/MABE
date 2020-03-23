#pragma once

#include "boost/filesystem/operations.hpp"
#include "boost/filesystem/path.hpp"

#include <string>
#include <map>
#include <set>

namespace fs = boost::filesystem;

typedef unsigned command_field_type;
typedef std::vector<command_field_type> command_type;

std::string asteroidsDescriptionToString(const std::vector<command_type>& desc);

class AsteroidsDatasetParser {

public:
	AsteroidsDatasetParser(std::string datasetPath);
	std::set<std::string> getAsteroidsNames();

	std::string getICQPath(std::string asteroidName);
	std::string getDescriptionPath(std::string asteroidName);
	const std::vector<command_type>& cachingGetDescription(std::string asteroidName);
	std::map<std::vector<command_type>,unsigned> getDescriptionStatistics();

	std::set<std::string> getAllPicturePaths(std::string asteroidName);
	std::string getPicturePath(std::string asteroidName, unsigned condition, unsigned distance, unsigned phase);
	std::map<std::string,std::set<unsigned>> getAllParameterValues(std::string asteroidName);

private:
	fs::path fsDatasetPath;
	std::map<std::string,std::vector<command_type>> descriptionCache;
	std::set<std::string> asteroidsNamesCache;
	bool asteroidsNamesCacheFull;

	void checkIfRegularFile(std::string path);
};
