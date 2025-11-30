#ifndef PLAYER_H
#define PLAYER_H

#include "Entity.h"
#include "Enemy.h"
#include "SpecialAttributes.h"
#include <vector>
#include <iostream>

class Player : public Entity {
public:
	int posR, posC;
	std::vector<SpecialAttributes> specialAbilities;

	Player(std::string n, int m, int a, int d, int r, int c);
	virtual void setStats() = 0;
	int calculateDamage() override;
};

class Soldier : public Player {
public:
	Soldier(int r, int c);
	void setStats() override;
};

class Archer : public Player {
public:
	Archer(int r, int c);
	void setStats() override;
};

class Mage : public Player {
public:
	Mage(int r, int c);
	void setStats() override;
};

#endif