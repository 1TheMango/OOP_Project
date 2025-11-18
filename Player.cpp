#include "Tile.cpp"
#include "Dice.cpp"
#include "CombatSystem.cpp"
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
	void takeDamage(int dmg) {
		if (defending) {
			dmg = dmg / 2;
			std::cout << name << " defended; damage halved to " << dmg << "\n";
		}
		hp -= dmg;
		if (hp < 0) hp = 0;
	}
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
	int calculateDamage() override {
		// Simple Enemy attack: d6 + ATK
		return d6.roll() + attack;
	}
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
	int calculateDamage() override {
		// Boss attacks harder: 2*d6 + ATK
		return d6.roll() + d6.roll() + attack;
	}
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
	int calculateDamage() override {
		D6 d6;
		return d6.roll() + attack;
	}
	
	// Combat actions will be delegated to CombatSystem, but keeping
	// these virtual functions satisfies the UML's intent for subclass attacks/abilities.
	virtual void attackEnemy(Enemy* enemy, std::ostream& log) = 0;
	virtual void useAbility(Enemy* enemy, std::ostream& log) = 0;
};

class Soldier : public Player {
public:
	Soldier(int r, int c) : Player("Soldier", 100, 15, 10, r, c) { setStats(); }

	void setStats() override {
		maxHp = 120; hp = 120; attack = 15; defense = 8; mana = 10;
		// The original logic used a basic attack and ability. Let's map it:
		specialAbilities.push_back({"Power Strike", 5, 15}); // ATKBonus 5, requires d20>=15
	}

	// These methods delegate to the CombatSystem for core logic
	void attackEnemy(Enemy* enemy, std::ostream& log) override;
	void useAbility(Enemy* enemy, std::ostream& log) override;
};

class Archer : public Player {
public:
	Archer(int r, int c) : Player("Archer", 80, 18, 5, r, c) { setStats(); }

	void setStats() override {
		maxHp = 80; hp = 80; attack = 18; defense = 4; mana = 20;
		specialAbilities.push_back({"Piercing Shot", 3, 12}); // ATKBonus 3, requires d20>=12
	}

	void attackEnemy(Enemy* enemy, std::ostream& log) override;
	void useAbility(Enemy* enemy, std::ostream& log) override;
};

class Mage : public Player {
public:
	Mage(int r, int c) : Player("Mage", 70, 8, 3, r, c) { setStats(); }

	void setStats() override {
		maxHp = 70; hp = 70; attack = 8; defense = 3; mana = 40;
		specialAbilities.push_back({"Fireball", 10, 10}); // ATKBonus 10, requires d20>=10
	}

	void attackEnemy(Enemy* enemy, std::ostream& log) override;
	void useAbility(Enemy* enemy, std::ostream& log) override;
};

void Soldier::attackEnemy(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.attack(); }
void Soldier::useAbility(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.ability(); }
void Archer::attackEnemy(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.attack(); }
void Archer::useAbility(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.ability(); }
void Mage::attackEnemy(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.attack(); }
void Mage::useAbility(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.ability(); }