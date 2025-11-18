#pragma once

#include <string>
#include <vector>
#include <iostream>
#include "Dice.h" // Player/Enemy damage calculation uses Dice

// --- Forward Declarations & Enums -------------------------------------------
// We forward-declare CombatSystem to solve a circular dependency
// Entity.h doesn't need to know *how* CombatSystem works,
// only that a class with this name exists.
class CombatSystem;

// --- Special Attributes (UML) -----------------------------------------------
struct SpecialAttributes {
	std::string name; 		// NAME string
	int atkPowerBonus = 0; // ATKPOWER int
	int minRolls = 0; 		// minrolls int (d20 requirement for ability success)
};

// --- Entity Hierarchy (UML) -------------------------------------------------
class Entity {
public:
	std::string name;
	int hp;
	int maxHp;
	int attack; 	// ATK int
	int defense; // DEF int
	int mana = 0; 	// MANA int (used by Player)
	bool defending = false;

	Entity(std::string n, int m, int a, int d) : name(n), hp(m), maxHp(m), attack(a), defense(d) {}
	virtual ~Entity() {}

	// Core combat methods
	virtual int calculateDamage() = 0; // The base damage dice roll + ATK
	void takeDamage(int dmg); // Definition in .cpp
	void resetDefend() { defending = false; }
};

// --- Enemy Subclasses (UML) -------------------------------------------------
class Enemy : public Entity {
protected:
	D6 d6;
public:
	// POS int is handled by the main loop (enemyRow/Col)
	Enemy(std::string n, int m, int a, int d) : Entity(n, m, a, d) {}
	
	// Enemy default damage calculation (Attack Player)
	int calculateDamage() override; // Definition in .cpp
};

class Monster : public Enemy {
public:
	std::string type; // type string
	Monster(std::string t, int m, int a, int d) : Enemy(t, m, a, d), type(t) {}
};

class Boss : public Enemy {
public:
	int level; // Level int
	Boss(std::string n, int l, int m, int a, int d) : Enemy(n, m, a, d), level(l) {
		name = "Boss " + n;
	}
	
	// Boss specialmove() implementation (stronger attack)
	int calculateDamage() override; // Definition in .cpp
};

// --- Player Hierarchy (UML) -------------------------------------------------
class Player : public Entity {
public:
	int posR, posC; // POS int
	std::vector<SpecialAttributes> specialAbilities;

	Player(std::string n, int m, int a, int d, int r, int c)
		: Entity(n, m, a, d), posR(r), posC(c) {}

	virtual void setStats() = 0; // To set initial class stats
	
	// Player damage calculation (Attack)
	int calculateDamage() override; // Definition in .cpp
	
	// Combat actions will be delegated to CombatSystem
	virtual void attackEnemy(Enemy* enemy, std::ostream& log) = 0;
	virtual void useAbility(Enemy* enemy, std::ostream& log) = 0;
};

class Soldier : public Player {
public:
	Soldier(int r, int c) : Player("Soldier", 100, 15, 10, r, c) { setStats(); }
	void setStats() override;
	void attackEnemy(Enemy* enemy, std::ostream& log) override;
	void useAbility(Enemy* enemy, std::ostream& log) override;
};

class Archer : public Player {
public:
	Archer(int r, int c) : Player("Archer", 80, 18, 5, r, c) { setStats(); }
	void setStats() override;
	void attackEnemy(Enemy* enemy, std::ostream& log) override;
	void useAbility(Enemy* enemy, std::ostream& log) override;
};

class Mage : public Player {
public:
	Mage(int r, int c) : Player("Mage", 70, 8, 3, r, c) { setStats(); }
	void setStats() override;
	void attackEnemy(Enemy* enemy, std::ostream& log) override;
	void useAbility(Enemy* enemy, std::ostream& log) override;
};