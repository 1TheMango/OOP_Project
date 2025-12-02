#ifndef DICE_H
#define DICE_H

#include <random>
#include <chrono>

extern std::mt19937 rng;

class Dice {
protected:
	int sides;
	int lastRoll = 0;
public:
	Dice(int s) : sides(s) {}
	virtual ~Dice() {}
	virtual int roll();
	int getLastRoll() const { return lastRoll; }
};

class D6 : public Dice { 
public: 
	D6() : Dice(6) {} 
};

class D20 : public Dice { 
public: 
	D20() : Dice(20) {} 
};

#endif