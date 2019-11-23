
struct Trait {
	int value;
	float step;
}

std::vector<Trait> readFile(const char* fileName = "evolution.txt");

void saveFile(std::vector<Trait> traits, const char* fileName = "evolution.txt");

std::vector<Trait> mutatedCopy(std::vector<Trait> traits);