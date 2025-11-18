#pragma once

#include <random>
#include <chrono>

// Global RNG setup
// We declare it 'extern' here, meaning "it exists somewhere else"
// It will be *defined* in Dice.cpp
extern std::mt19937 rng;

// --- Dice Hierarchy (UML) ---------------------------------------------------
class Dice {
protected:
	int sides;
	int lastRoll = 0;
public:
	Dice(int s) : sides(s) {}
	virtual ~Dice() {}
	virtual int roll(); // Definition moved to .cpp
	int getLastRoll() const { return lastRoll; }
};

class D6 : public Dice { public: D6() : Dice(6) {} };
class D20 : public Dice { public: D20() : Dice(20) {} };