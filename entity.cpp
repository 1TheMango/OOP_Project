#include "entity.h"
#include "CombatSystem.h" // Must include for the delegation methods

// --- Entity Methods ---
void Entity::takeDamage(int dmg) {
	if (defending) {
		dmg = dmg / 2;
		std::cout << name << " defended; damage halved to " << dmg << "\n";
	}
	hp -= dmg;
	if (hp < 0) hp = 0;
}

// --- Enemy Methods ---
int Enemy::calculateDamage() {
	// Simple Enemy attack: d6 + ATK
	return d6.roll() + attack;
}

// --- Boss Methods ---
int Boss::calculateDamage() {
	// Boss attacks harder: 2*d6 + ATK
	return d6.roll() + d6.roll() + attack;
}

// --- Player Methods ---
int Player::calculateDamage() {
	D6 d6; // Local dice for calculation
	return d6.roll() + attack;
}

// --- Soldier Methods ---
void Soldier::setStats() {
	maxHp = 120; hp = 120; attack = 15; defense = 8; mana = 10;
	specialAbilities.push_back({"Power Strike", 5, 15}); // ATKBonus 5, requires d20>=15
}
void Soldier::attackEnemy(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.attack(); }
void Soldier::useAbility(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.ability(); }

// --- Archer Methods ---
void Archer::setStats() {
	maxHp = 80; hp = 80; attack = 18; defense = 4; mana = 20;
	specialAbilities.push_back({"Piercing Shot", 3, 12}); // ATKBonus 3, requires d20>=12
}
void Archer::attackEnemy(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.attack(); }
void Archer::useAbility(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.ability(); }

// --- Mage Methods ---
void Mage::setStats() {
	maxHp = 70; hp = 70; attack = 8; defense = 3; mana = 40;
	specialAbilities.push_back({"Fireball", 10, 10}); // ATKBonus 10, requires d20>=10
}
void Mage::attackEnemy(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.attack(); }
void Mage::useAbility(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.ability(); }