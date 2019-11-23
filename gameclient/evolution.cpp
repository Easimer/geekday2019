#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <utility>
#include <random>
#include "evolution.h"

using namespace std;

std::vector<Trait> readFile(const char* fileName) {
	ifstream infile;
	infile.open(fileName);

	std::string line;
	std::vector<Trait> traits;
	while (std::getline(infile, line))
	{
		std::istringstream iss(line);
		int value;
        float stepsize;
		if (!(iss >> value >> stepsize)) { break; }
		traits.push_back({ value, stepsize });
	}

	infile.close();

    return traits;
}

void saveFile(const std::vector<Trait>& traits, const char* fileName) {
	ofstream outfile;
	outfile.open(fileName);
	for (auto& trait : traits)
	{
		outfile << trait.value << " " << trait.step << std::endl;
	}
	outfile.close();
}

std::vector<Trait> mutatedCopy(const std::vector<Trait>& traits) {
	std::random_device rd;
	std::mt19937 e2(rd());
	std::uniform_real_distribution<> dist(0.5, 2);
	std::uniform_real_distribution<> dist2(0, 1);

	std::vector<Trait> newTraits(traits.size());
	for (auto& trait : traits)
	{
		float stepsize = trait.step * dist(e2);
		int value = trait.value;
		if (dist2(e2) > 0.5) value += stepsize;
		else value -= stepsize;
		newTraits.push_back({ value, stepsize });
	}

	return newTraits;
}