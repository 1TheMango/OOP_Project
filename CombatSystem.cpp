#include "CombatSystem.h"

CombatSystem::CombatSystem(Player* p, Enemy* e, std::ostream& l) : player(p), enemy(e), log(l) {}

// TurnManager() logic is implicitly handled by the main loop and action calls.
// Player Options: Attack
void CombatSystem::attack() {
	if (!player || !enemy) return;

	int hitRoll = d20.roll();
	log << "[Dice] Player d20 = " << hitRoll << " (" << player->name << " ATK: " << player->attack << ")\n";

	// To Hit Check: d20 + Player ATK vs (10 + Enemy DEF)
	int attackCheck = hitRoll + player->attack;
	int defenseTarget = 10 + enemy->defense;
	
	bool crit = (hitRoll == 20);
	
	if (attackCheck >= defenseTarget || crit) {
		int dmg = player->calculateDamage();
		if (crit) { dmg += D6().roll(); log << "CRITICAL! extra d6\n"; } // D6 is local to function
		
		enemy->takeDamage(dmg);
		log << player->name << " hits " << enemy->name << " for " << dmg << " damage.\n";
	} else {
		log << player->name << "'s attack missed (Target: " << defenseTarget << ").\n";
	}
}

// Player Options: Ability
void CombatSystem::ability() {
	if (!player || !enemy || player->specialAbilities.empty()) return;
	
	const SpecialAttributes& ability = player->specialAbilities[0];
	const int MANA_COST = 5; // Fixed cost for simplicity
	
	if (player->mana < MANA_COST) {
		log << "Not enough Mana (" << player->mana << ")! Cost is " << MANA_COST << ".\n";
		return;
	}
	
	int abilityRoll = d20.roll();
	log << "[Dice] Ability d20 = " << abilityRoll << " (Required: >=" << ability.minRolls << ")\n";
	
	// Check for ability success based on roll
	if (abilityRoll >= ability.minRolls) {
		D6 d6;
		// Damage Calculator(): 2*d6 + ATK + Bonus
		int dmg = d6.roll() + d6.roll() + player->attack + ability.atkPowerBonus;
		player->mana -= MANA_COST;
		
		enemy->takeDamage(dmg);
		log << ability.name << " success! You deal " << dmg << " damage. Mana left: " << player->mana << "\n";
	} else {
		log << ability.name << " failed (roll too low).\n";
	}
}

// Player Options: Defend
void CombatSystem::defend() {
	if (!player) return;
	player->defending = true;
	log << player->name << " braces for the next attack (defend).\n";
}

// Player Options: Run
bool CombatSystem::run() {
	int d20Roll = d20.roll();
	log << "[Dice] Run d20 = " << d20Roll << "\n";
	if (d20Roll >= 12) {
		log << "You fled the battle!\n";
		return true;
	} else {
		log << "Run failed!\n";
		return false;
	}
}

// Enemy Turn logic
void CombatSystem::enemyTurn() {
	if (!enemy || player->hp <= 0) return;
	
	log << "--- [Enemy Turn] " << enemy->name << " attacks ---\n";
	
	int d20Roll = d20.roll();
	log << "[Dice] Enemy d20 = " << d20Roll << " (" << enemy->name << " ATK: " << enemy->attack << ")\n";
	
	// To Hit Check: d20 + Enemy ATK vs (10 + Player DEF)
	int attackCheck = d20Roll + enemy->attack;
	int defenseTarget = 10 + player->defense;
	
	bool crit = (d20Roll == 20);
	
	if (attackCheck >= defenseTarget || crit) {
		int dmg = enemy->calculateDamage();
		if (crit) { dmg += D6().roll(); log << "Enemy CRITICAL!\n"; }
		
		player->takeDamage(dmg);
		log << enemy->name << " deals " << dmg << " damage. Player HP = " << player->hp << "\n";
	} else {
		log << enemy->name << " missed (Target: " << defenseTarget << ").\n";
	}
	
	// Reset defend status
	player->resetDefend();
}

// Win/Loss check functions
bool CombatSystem::isEnemyDefeated() const {
	return enemy && enemy->hp <= 0;
}
bool CombatSystem::isPlayerDefeated() const {
	return player && player->hp <= 0;
}