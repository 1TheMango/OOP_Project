#include "Player.h"
#include "CombatSystem.h"
#include "Dice.h"

Player::Player(std::string n, int m, int a, int d, int r, int c)
	: Entity(n, m, a, d), posR(r), posC(c) {}

int Player::calculateDamage() {
	D6 d6;
	return d6.roll() + attack;
}

Soldier::Soldier(int r, int c) 
	: Player("Soldier", 100, 15, 10, r, c) { 
	setStats(); 
}

void Soldier::setStats() {
	maxHp = 120; hp = 120; attack = 15; defense = 8; mana = 10;
	specialAbilities.push_back({"Power Strike", 5, 15});
}

void Soldier::attackEnemy(Enemy* enemy, std::ostream& log) { 
	CombatSystem cs(this, enemy, log); 
	cs.attack(); 
}

void Soldier::useAbility(Enemy* enemy, std::ostream& log) { 
	CombatSystem cs(this, enemy, log); 
	cs.ability(); 
}

Archer::Archer(int r, int c) 
	: Player("Archer", 80, 18, 5, r, c) { 
	setStats(); 
}

void Archer::setStats() {
	maxHp = 80; hp = 80; attack = 18; defense = 4; mana = 20;
	specialAbilities.push_back({"Piercing Shot", 3, 12});
}

void Archer::attackEnemy(Enemy* enemy, std::ostream& log) { 
	CombatSystem cs(this, enemy, log); 
	cs.attack(); 
}

void Archer::useAbility(Enemy* enemy, std::ostream& log) { 
	CombatSystem cs(this, enemy, log); 
	cs.ability(); 
}

Mage::Mage(int r, int c) 
	: Player("Mage", 70, 8, 3, r, c) { 
	setStats(); 
}

void Mage::setStats() {
	maxHp = 70; hp = 70; attack = 8; defense = 3; mana = 40;
	specialAbilities.push_back({"Fireball", 10, 10});
}

void Mage::attackEnemy(Enemy* enemy, std::ostream& log) { 
	CombatSystem cs(this, enemy, log); 
	cs.attack(); 
}

void Mage::useAbility(Enemy* enemy, std::ostream& log) { 
	CombatSystem cs(this, enemy, log); 
	cs.ability(); 
}