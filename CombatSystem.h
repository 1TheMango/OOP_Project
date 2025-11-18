#pragma once

#include "entity.h" // Depends on Entity classes
#include "Dice.h"   // Uses D20 and D6
#include <iostream>

// --- Combat System (UML) ----------------------------------------------------
class CombatSystem {
private:
	Player* player;
	Enemy* enemy;
	D20 d20;
	std::ostream& log;

public:
	CombatSystem(Player* p, Enemy* e, std::ostream& l);

	// Player Options
	void attack();
	void ability();
	void defend();
	bool run();

	// Enemy Turn logic
	void enemyTurn();

	// Win/Loss check functions
	bool isEnemyDefeated() const;
	bool isPlayerDefeated() const;
};