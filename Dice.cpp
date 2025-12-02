#include "include/Dice.h"

std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());

int Dice::roll() {
	std::uniform_int_distribution<int> dist(1, sides);
	lastRoll = dist(rng);
	return lastRoll;
}