#ifndef COMBATSYSTEM_H
#define COMBATSYSTEM_H

#include "Player.h"
#include "Enemy.h"
#include "Dice.h"
#include <iostream>

class CombatSystem {
private:
	Player* player;
	Enemy* enemy;
	D20 d20;
	std::ostream& log;

public:
	CombatSystem(Player* p, Enemy* e, std::ostream& l);
	void attack();
	void ability();
	void defend();
	bool run();
	void enemyTurn();
	bool isEnemyDefeated() const;
	bool isPlayerDefeated() const;
};

#endif