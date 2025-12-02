#ifndef ENEMY_H
#define ENEMY_H

#include "Entity.h"
#include "Dice.h"

class Enemy : public Entity {
protected:
	D6 d6;
public:
	Enemy(std::string n, int m, int a, int d);
	int calculateDamage() override;
};

class Monster : public Enemy {
public:
	std::string type;
	Monster(std::string t, int m, int a, int d);
};

class Boss : public Enemy {
public:
	int level;
	Boss(std::string n, int l, int m, int a, int d);
	int calculateDamage() override;
};

#endif