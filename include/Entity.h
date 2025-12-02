#ifndef ENTITY_H
#define ENTITY_H

#include <string>
#include <iostream>

class Entity {
public:
	std::string name;
	int hp;
	int maxHp;
	int attack;
	int defense;
	int mana = 0;
	bool defending = false;

	Entity(std::string n, int m, int a, int d);
	virtual ~Entity() {}

	virtual int calculateDamage() = 0;
	void takeDamage(int dmg);
	void resetDefend() { defending = false; }
};

#endif