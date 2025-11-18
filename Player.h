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
	virtual void attackEnemy(Enemy* enemy, std::ostream& log) = 0;
	virtual void useAbility(Enemy* enemy, std::ostream& log) = 0;
};

class Soldier : public Player {
public:
	Soldier(int r, int c);
	void setStats() override;
	void attackEnemy(Enemy* enemy, std::ostream& log) override;
	void useAbility(Enemy* enemy, std::ostream& log) override;
};

class Archer : public Player {
public:
	Archer(int r, int c);
	void setStats() override;
	void attackEnemy(Enemy* enemy, std::ostream& log) override;
	void useAbility(Enemy* enemy, std::ostream& log) override;
};

class Mage : public Player {
public:
	Mage(int r, int c);
	void setStats() override;
	void attackEnemy(Enemy* enemy, std::ostream& log) override;
	void useAbility(Enemy* enemy, std::ostream& log) override;
};

#endif