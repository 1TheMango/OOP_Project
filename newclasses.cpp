// RogueEmblem.cpp
// Single-file SFML game with dice movement and integrated turn-based + dice battle UI
// Fully refactored to comply with the OOP Project Proposal and UML diagram.
//
// Compile:
// g++ RogueEmblem.cpp -o RogueEmblem -lsfml-graphics -lsfml-window -lsfml-system

#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <optional>
#include <functional>
#include <random>
#include <chrono>
#include <memory> 		// For std::unique_ptr

using namespace std;

// Global RNG setup
static std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());

// --- Dice Hierarchy (UML) ---------------------------------------------------
class Dice {
protected:
	int sides;
	int lastRoll = 0;
public:
	Dice(int s) : sides(s) {}
	virtual ~Dice() {}
	virtual int roll() {
		std::uniform_int_distribution<int> dist(1, sides);
		lastRoll = dist(rng);
		return lastRoll;
	}
	int getLastRoll() const { return lastRoll; }
};

class D6 : public Dice { public: D6() : Dice(6) {} };
class D20 : public Dice { public: D20() : Dice(20) {} };

// --- Forward Declarations & Enums -------------------------------------------
enum class GameState { MainMenu, Exploring, InBattle, GameOver, Victory };
struct SpecialAttributes;
class Entity;
class Player;
class Enemy;
class CombatSystem;
class Tile;

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


// --- Combat System (UML) ----------------------------------------------------
class CombatSystem {
private:
	Player* player;
	Enemy* enemy;
	D20 d20;
	std::ostream& log;

public:
	CombatSystem(Player* p, Enemy* e, std::ostream& l) : player(p), enemy(e), log(l) {}

	// TurnManager() logic is implicitly handled by the main loop and action calls.
	// Player Options: Attack
	void attack() {
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
	void ability() {
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
	void defend() {
		if (!player) return;
		player->defending = true;
		log << player->name << " braces for the next attack (defend).\n";
	}

	// Player Options: Run
	bool run() {
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
	void enemyTurn() {
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
	bool isEnemyDefeated() const {
		return enemy && enemy->hp <= 0;
	}
	bool isPlayerDefeated() const {
		return player && player->hp <= 0;
	}
};

// Implement Player class delegation (as CombatSystem must be fully defined)
void Soldier::attackEnemy(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.attack(); }
void Soldier::useAbility(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.ability(); }
void Archer::attackEnemy(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.attack(); }
void Archer::useAbility(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.ability(); }
void Mage::attackEnemy(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.attack(); }
void Mage::useAbility(Enemy* enemy, std::ostream& log) { CombatSystem cs(this, enemy, log); cs.ability(); }


// --- Tile Hierarchy (UML) ---------------------------------------------------
class Tile {
protected:
	sf::Sprite sprite;
public:
	virtual ~Tile() {}
	// Position/isVisited is handled by Board/Game logic for simplicity
	
	virtual bool isBlocked() const { return false; }
	virtual bool isMonster() const { return false; }
	virtual bool isBoss() const { return false; } // Added for BossTile
	virtual bool isExit() const { return false; }
	
	// onEnter(Player) as per UML
	virtual void onEnter(Player* p) { (void)p; }
	
	sf::Sprite& getSprite() { return sprite; }
};

class EmptyTile : public Tile { 
public: 
	void onEnter(Player* p) override { 
		// std::cout << "[Event] Empty tile entered.\n";
	} 
}; // Represents 'N' in the level layout

class BlockedTile : public Tile { 
public: 
	bool isBlocked() const override { return true; } 
	// denyMovement() logic is handled by the main game loop
};

class MonsterTile : public Tile { 
private:
	bool combatTriggered = false; // To prevent infinite battle loop on 'Run'
public: 
	bool isMonster() const override { return true; } 
	// triggerCombat() logic is handled in the main game loop
	void onEnter(Player* p) override { 
		if (!combatTriggered) {
			 std::cout << "[Event] Monster encountered (board)\n"; 
			 combatTriggered = true;
		}
	} 
	bool shouldTriggerCombat() const { return combatTriggered; }
	void resetCombatTrigger() { combatTriggered = false; }
};

// New BossTile class per UML
class BossTile : public Tile {
private:
	bool combatTriggered = false;
public: 
	bool isBoss() const override { return true; }
	void onEnter(Player* p) override {
		if (!combatTriggered) {
			std::cout << "[Event] BOSS encountered (board)\n";
			combatTriggered = true;
		}
	}
	bool shouldTriggerCombat() const { return combatTriggered; }
	void resetCombatTrigger() { combatTriggered = false; }
};

class ExitTile : public Tile { 
public: 
	bool isExit() const override { return true; } 
	// endLevel() logic is handled in the main game loop
	void onEnter(Player* p) override { 
		std::cout << "[Event] Exit reached\n"; 
	} 
};

// --- Board Class ------------------------------------------------------------
// (Modified from original to use proper Tile* types)
class Board {
private:
	int rows, cols;
	float tileSize;
	vector<vector<Tile*>> grid;
public:
	Board(int r, int c, float size) : rows(r), cols(c), tileSize(size) {
		grid.resize(rows, vector<Tile*>(cols, nullptr));
	}
	~Board() {
		for (int r=0;r<rows;r++) for (int c=0;c<cols;c++) delete grid[r][c];
	}
	void setTile(int r, int c, Tile* tile, sf::Texture& tex) {
		if (!tile) return;
		tile->getSprite().setTexture(tex);
		tile->getSprite().setPosition(c * tileSize, r * tileSize);
		grid[r][c] = tile;
	}
	Tile* getTile(int r,int c) {
		if (r<0||c<0||r>=rows||c>=cols) return nullptr;
		return grid[r][c];
	}
	void draw(sf::RenderWindow& win) {
		for (int r=0;r<rows;r++) for (int c=0;c<cols;c++) if (grid[r][c]) win.draw(grid[r][c]->getSprite());
	}
	// Replaces MonsterTile with EmptyTile upon defeat
	void replaceWithEmpty(int r,int c, sf::Texture& texEmpty) {
		delete grid[r][c];
		grid[r][c] = new EmptyTile();
		grid[r][c]->getSprite().setTexture(texEmpty);
		grid[r][c]->getSprite().setPosition(c * tileSize, r * tileSize);
		// Reset the player's position sprite to ensure it draws over the empty tile
	}
};

// --- UI Manager (Simplified for single-file SFML) ---------------------------
// In the UML, UIManager handles screen transitions. In this refactor, 
// the rendering and button setup in 'main' acts as the UIManager methods.
// The Button struct is retained as a UI Helper.
struct Button {
	sf::RectangleShape rect;
	sf::Text label;
	std::function<void()> onClick;
	bool contains(sf::Vector2f p) const { return rect.getGlobalBounds().contains(p); }
};

// --- Main Game Loop ---------------------------------------------------------
int main() {
	// Dice system initialization is handled by global RNG setup
	
	const int ROWS = 10, COLS = 10;
	const float TILE_SIZE = 64.f;
	const int WINDOW_W = int(COLS * TILE_SIZE);
	const int WINDOW_H = int(ROWS * TILE_SIZE + 160); // extra space for battle UI

	sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "RogueEmblem - OOP Project");
	window.setFramerateLimit(60);

	// Textures - renamed texNormal to texEmpty for UML compliance
	sf::Texture texEmpty, texBlocked, texMonster, texBoss, texExit, texPlayer;
	if (!texEmpty.loadFromFile("assets/normal.png")) 	cerr << "Warn: missing assets/normal.png\n";
	if (!texBlocked.loadFromFile("assets/blocked.png")) cerr << "Warn: missing assets/blocked.png\n";
	if (!texMonster.loadFromFile("assets/monster.png")) cerr << "Warn: missing assets/monster.png\n";
	if (!texBoss.loadFromFile("assets/boss.png")) 		cerr << "Warn: missing assets/boss.png\n";
	if (!texExit.loadFromFile("assets/exit.png")) 		cerr << "Warn: missing assets/exit.png\n";
	if (!texPlayer.loadFromFile("assets/player.png")) 	cerr << "Warn: missing assets/player.png\n";

	sf::Font font;
	bool fontOk = true;
	if (!font.loadFromFile("assets/Arial.ttf")) {
		cerr << "Warn: missing assets/Arial.ttf (UI labels will be empty). Put a TTF at assets/Arial.ttf\n";
		fontOk = false;
	}

	// Create board and load static level
	Board board(ROWS, COLS, TILE_SIZE);
	// 'T' is now BossTile
	char LEVEL[10][10] = {
		{'P','N','N','N','B','N','N','N','N','N'},
		{'N','B','B','N','B','N','M','N','B','N'},
		{'N','N','N','N','N','N','N','N','N','N'},
		{'N','B','N','B','N','B','N','B','N','N'},
		{'N','N','N','N','N','N','N','B','N','N'},
		{'N','B','N','N','B','N','N','N','N','N'},
		{'N','N','N','B','N','B','N','N','B','N'},
		{'N','N','N','N','N','N','N','N','N','N'},
		{'N','B','N','N','N','N','B','N','M','N'},
		{'E','N','N','N','B','N','N','N','N','T'} // 'E' moved, 'T' (Boss) added
	};

	int playerR = 0, playerC = 0;
	for (int r=0;r<ROWS;r++){
		for (int c=0;c<COLS;c++){
			char ch = LEVEL[r][c];
			// Using EmptyTile to replace NormalTile for UML compliance
			if (ch=='N') board.setTile(r,c,new EmptyTile(), texEmpty); 
			else if (ch=='B') board.setTile(r,c,new BlockedTile(), texBlocked);
			else if (ch=='M') board.setTile(r,c,new MonsterTile(), texMonster);
			else if (ch=='T') board.setTile(r,c,new BossTile(), texBoss); // Added BossTile
			else if (ch=='E') board.setTile(r,c,new ExitTile(), texExit);
			// 'P' is starting pos, replaced with EmptyTile
			else if (ch=='P') { board.setTile(r,c,new EmptyTile(), texEmpty); playerR=r; playerC=c; }
		}
	}

	// Player is now polymorphic, initialized at Main Menu
	std::unique_ptr<Player> player = nullptr;
	
	// Player sprite
	sf::Sprite playerSprite; playerSprite.setTexture(texPlayer); playerSprite.setScale(0.9f,0.9f);
	// Position will be set after class selection

	int movePoints = 0;

	// Battle state containers
	GameState state = GameState::MainMenu; // Start at Main Menu
	std::string battleMessage; // <-- ADD: For the new battle log
	// Enemy is now a unique_ptr to handle polymorphism (Monster or Boss)
	std::unique_ptr<Enemy> currentEnemy = nullptr; 
	std::unique_ptr<CombatSystem> combatSystem = nullptr;
	int enemyRow = -1, enemyCol = -1;

	// Battle UI elements
	sf::RectangleShape panel(sf::Vector2f(WINDOW_W - 20, 140));
	panel.setPosition(10, ROWS * TILE_SIZE + 10);
	panel.setFillColor(sf::Color(30,30,30,220));
	panel.setOutlineColor(sf::Color::Black);
	panel.setOutlineThickness(2.f);

	// Helper function for creating buttons
	auto makeButton = [&](float x, float y, float w, float h, const string& txt, sf::Font& f, function<void()> cb) -> Button {
		Button b;
		b.rect = sf::RectangleShape(sf::Vector2f(w,h));
		b.rect.setPosition(x,y);
		b.rect.setFillColor(sf::Color(70,70,70));
		b.rect.setOutlineThickness(1.f);
		b.rect.setOutlineColor(sf::Color::Black);
		if (fontOk) {
			b.label.setFont(f);
			b.label.setString(txt);
			b.label.setCharacterSize(18);
			b.label.setFillColor(sf::Color::White);
			sf::FloatRect lb = b.label.getLocalBounds();
			b.label.setPosition(x + (w - lb.width)/2 - lb.left, y + (h - lb.height)/2 - lb.top);
		}
		b.onClick = cb;
		return b;
	};

	// --- Main Menu UI (UIManager) ---
	sf::Text titleText("RogueEmblem", font, 48);
	titleText.setFillColor(sf::Color::White);
	titleText.setPosition(WINDOW_W/2 - titleText.getLocalBounds().width/2, 100);
	sf::Text subtitleText("Select your class:", font, 24);
	subtitleText.setFillColor(sf::Color::White);
	subtitleText.setPosition(WINDOW_W/2 - subtitleText.getLocalBounds().width/2, 180);

	vector<Button> menuButtons;
	menuButtons.push_back(makeButton(WINDOW_W/2 - 100, 250, 200, 50, "Soldier", font, [&](){
		player = std::make_unique<Soldier>(playerR, playerC);
		state = GameState::Exploring;
	}));
	menuButtons.push_back(makeButton(WINDOW_W/2 - 100, 320, 200, 50, "Archer", font, [&](){
		player = std::make_unique<Archer>(playerR, playerC);
		state = GameState::Exploring;
	}));
	menuButtons.push_back(makeButton(WINDOW_W/2 - 100, 390, 200, 50, "Mage", font, [&](){
		player = std::make_unique<Mage>(playerR, playerC);
		state = GameState::Exploring;
	}));

	// --- Battle UI Buttons ---
	vector<Button> battleButtons;
	const float btnW = 160, btnH = 40;
	float baseY = ROWS * TILE_SIZE + 30; // <-- This is the old Y
	float battleBtnY = WINDOW_H - 70.f; // <-- ADD: New Y for battle screen

	// Helper to start a battle at tile r,c
	auto startBattle = [&](int r, int c, bool isBoss){
		std::cout << "\n--- Battle start ---\n";
		
		if (isBoss) {
			 currentEnemy = std::make_unique<Boss>("Dungeon Lord", 3, 100, 15, 8);
		} else {
			// Randomly spawn a Monster or a small Boss for variety
			if (D20().roll() > 15) {
				 currentEnemy = std::make_unique<Boss>("Ogre", 1, 25, 10, 5);
			} else {
				 currentEnemy = std::make_unique<Monster>("Goblin", 18, 5, 2);
			}
		}
		
		combatSystem = std::make_unique<CombatSystem>(player.get(), currentEnemy.get(), std::cout);
		enemyRow = r; enemyCol = c;
		state = GameState::InBattle;
		player->resetDefend();
		battleMessage = "Battle started! " + player->name + " vs " + currentEnemy->name; // <-- ADD
	};
	
	// Function to check and resolve turn-end conditions
	auto checkBattleEnd = [&](){
		if (!currentEnemy || !combatSystem) return;

		if (combatSystem->isEnemyDefeated()) {
			std::cout << currentEnemy->name << " defeated! Player gains 5 HP.\n";
			player->hp = std::min(player->hp + 5, player->maxHp); // Small reward
			
			// Remove monster from board -> replace with EmptyTile
			board.replaceWithEmpty(enemyRow, enemyCol, texEmpty);

			currentEnemy = nullptr;
			combatSystem = nullptr;
			state = GameState::Exploring;
		} else if (combatSystem->isPlayerDefeated()) {
			std::cout << player->name << " died. Game Over.\n";
			state = GameState::GameOver;
			currentEnemy = nullptr;
			combatSystem = nullptr;
		}
	};

	// --- Setup Battle Buttons ---
	// Attack Button
	battleButtons.push_back(makeButton(20, battleBtnY, btnW, btnH, "Attack", font, [&](){ // <-- UPDATE: baseY to battleBtnY
		if (state != GameState::InBattle || !combatSystem) return;
		battleMessage = player->name + " attacks!\n"; // <-- ADD
		combatSystem->attack();
		checkBattleEnd(); 
		if (state == GameState::InBattle) { 
			battleMessage += "Enemy attacks!\n"; // <-- ADD
			combatSystem->enemyTurn();
			checkBattleEnd();
			if(state == GameState::InBattle) battleMessage += "Your turn."; // <-- ADD
			else if (state == GameState::GameOver) battleMessage += "You were defeated!"; // <-- ADD
		} else if (state == GameState::Exploring) {
			battleMessage += "You won the battle!"; // <-- ADD
		}
	}));

	// Defend Button
	battleButtons.push_back(makeButton(210, battleBtnY, btnW, btnH, "Defend", font, [&](){ // <-- UPDATE: baseY to battleBtnY
		if (state != GameState::InBattle || !combatSystem) return;
		battleMessage = player->name + " defends!\n"; // <-- ADD
		combatSystem->defend();
		battleMessage += "Enemy attacks!\n"; // <-- ADD
		combatSystem->enemyTurn();
		checkBattleEnd();
		if(state == GameState::InBattle) battleMessage += "Your turn."; // <-- ADD
		else if (state == GameState::GameOver) battleMessage += "You were defeated!"; // <-- ADD
	}));

	// Ability Button
	battleButtons.push_back(makeButton(400, battleBtnY, btnW, btnH, "Ability", font, [&](){ // <-- UPDATE: baseY to battleBtnY
		if (state != GameState::InBattle || !combatSystem) return;
		battleMessage = player->name + " uses ability!\n"; // <-- ADD
		combatSystem->ability();
		checkBattleEnd();
		if (state == GameState::InBattle) { 
			battleMessage += "Enemy attacks!\n"; // <-- ADD
			combatSystem->enemyTurn();
			checkBattleEnd();
			if(state == GameState::InBattle) battleMessage += "Your turn."; // <-- ADD
			else if (state == GameState::GameOver) battleMessage += "You were defeated!"; // <-- ADD
		} else if (state == GameState::Exploring) {
			battleMessage += "You won the battle!"; // <-- ADD
		}
	}));

	// Run Button
	battleButtons.push_back(makeButton(590, battleBtnY, btnW, btnH, "Run", font, [&](){ // <-- UPDATE: baseY to battleBtnY
		if (state != GameState::InBattle || !combatSystem) return;
		if (combatSystem->run()) {
			// Success: state set to Exploring, pointers cleared
			Tile* t = board.getTile(player->posR, player->posC);
			if (MonsterTile* mt = dynamic_cast<MonsterTile*>(t)) { mt->resetCombatTrigger(); }
			if (BossTile* bt = dynamic_cast<BossTile*>(t)) { bt->resetCombatTrigger(); }

			currentEnemy = nullptr;
			combatSystem = nullptr;
			state = GameState::Exploring;
			battleMessage = "You fled!"; // <-- ADD
		} else {
			battleMessage = "Run failed!\n"; // <-- ADD
			battleMessage += "Enemy attacks!\n"; // <-- ADD
			combatSystem->enemyTurn();
			checkBattleEnd();
			if(state == GameState::InBattle) battleMessage += "Your turn."; // <-- ADD
			else if (state == GameState::GameOver) battleMessage += "You were defeated!"; // <-- ADD
		}
	}));

	// --- Battle Screen UI Elements ---
	sf::RectangleShape playerBox(sf::Vector2f(250, 300));
	playerBox.setFillColor(sf::Color(80, 90, 180, 200));
	playerBox.setPosition(100, 200);

	sf::RectangleShape enemyBox(sf::Vector2f(250, 300));
	enemyBox.setFillColor(sf::Color(180, 70, 70, 200));
	enemyBox.setPosition(WINDOW_W - 350, 150);

	sf::Text playerBattleName, enemyBattleName, battleLogText;
	if(fontOk) {
		playerBattleName.setFont(font);
		playerBattleName.setCharacterSize(24);
		playerBattleName.setFillColor(sf::Color::White);

		enemyBattleName.setFont(font);
		enemyBattleName.setCharacterSize(24);
		enemyBattleName.setFillColor(sf::Color::White);

		battleLogText.setFont(font);
		battleLogText.setCharacterSize(20);
		battleLogText.setFillColor(sf::Color::White);
		battleLogText.setPosition(50, WINDOW_H - 150);
	}

	const float BAR_WIDTH = 200;
	const float BAR_HEIGHT = 25;
	sf::RectangleShape playerHpBarBack(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT));
	playerHpBarBack.setFillColor(sf::Color(50, 50, 50));
	sf::RectangleShape playerHpBarFront(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT));
	playerHpBarFront.setFillColor(sf::Color::Green);
	
	sf::RectangleShape enemyHpBarBack(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT));
	enemyHpBarBack.setFillColor(sf::Color(50, 50, 50));
	sf::RectangleShape enemyHpBarFront(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT));
	enemyHpBarFront.setFillColor(sf::Color::Red);


	// --- Game Loop Start ---
	while (window.isOpen()) {
		sf::Event ev;
		while (window.pollEvent(ev)) {
			if (ev.type == sf::Event::Closed) window.close();

			// --- Event Handling by State ---
			if (state == GameState::MainMenu) {
				if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
					sf::Vector2f mp(ev.mouseButton.x, ev.mouseButton.y);
					for (auto &b : menuButtons) {
						if (b.contains(mp)) {
							b.onClick();
							// Once player is set, update sprite position
							if (player) {
								playerSprite.setPosition(player->posC * TILE_SIZE, player->posR * TILE_SIZE);
							}
							break;
						}
					}
				}
			}
			else if (state == GameState::Exploring) {
				// Must check if player exists (it should, but good practice)
				if (!player) {
					state = GameState::MainMenu; // Failsafe
					continue;
				}

				// Movement Dice (D6)
				if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Space) {
					D6 moveDice;
					movePoints = moveDice.roll();
					std::cout << "[Movement] Rolled d6 = " << movePoints << " move points\n";
				}
				
				// Movement keys (consume movePoints)
				if (ev.type == sf::Event::KeyPressed && movePoints > 0) {
					int dr=0, dc=0;
					if (ev.key.code == sf::Keyboard::W) dr = -1;
					if (ev.key.code == sf::Keyboard::S) dr = +1;
					if (ev.key.code == sf::Keyboard::A) dc = -1;
					if (ev.key.code == sf::Keyboard::D) dc = +1;
					
					if (dr!=0 || dc!=0) {
						int nr = player->posR + dr;
						int nc = player->posC + dc;
						
						Tile* t = board.getTile(nr,nc);
						
						if (!t) { std::cout << "Cannot move out of bounds"<<endl; }
						else if (t->isBlocked()) { std::cout << "Blocked tile"<<endl; }
						else {
							// Valid move: Update position and decrement points
							player->posR = nr; player->posC = nc;
							playerSprite.setPosition(player->posC * TILE_SIZE, player->posR * TILE_SIZE);
							movePoints--;
							
							// Check tile events (MonsterTile, BossTile, ExitTile)
							t->onEnter(player.get()); // Call UML's onEnter(Player*)
							
							if (t->isMonster() && dynamic_cast<MonsterTile*>(t)->shouldTriggerCombat()) {
								startBattle(nr, nc, false); // Start monster battle
							} else if (t->isBoss() && dynamic_cast<BossTile*>(t)->shouldTriggerCombat()) {
								startBattle(nr, nc, true); // Start BOSS battle
							} else if (t->isExit()) {
								std::cout << "You reached the exit - Victory!\n";
								state = GameState::Victory;
							}
						}
					}
				}
			}
			else if (state == GameState::InBattle) {
				// Handle button presses using the mouse
				if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
					sf::Vector2f mp(ev.mouseButton.x, ev.mouseButton.y);
					for (auto &b : battleButtons) {
						if (b.contains(mp)) {
							b.onClick();
							break; // Only handle one click
						}
					}
				}
			}
		} // end events

		// --- Render by State ---
		window.clear(sf::Color(25,25,25));

		if (state == GameState::MainMenu) {
			window.draw(titleText);
			window.draw(subtitleText);
			for(auto &b : menuButtons) {
				window.draw(b.rect);
				if(fontOk) window.draw(b.label);
			}
		}
		else if (state == GameState::Exploring) {
			board.draw(window);
			window.draw(playerSprite);

			// Exploring HUD
			if (fontOk && player) {
				string s = "Exploring. Move: WASD. Press SPACE to roll (d6). MovePoints: " + to_string(movePoints) + 
						   " | " + player->name + " HP: " + to_string(player->hp) + "/" + to_string(player->maxHp);
				sf::Text t(s, font, 16); t.setFillColor(sf::Color::White); t.setPosition(10, ROWS*TILE_SIZE + 10);
				window.draw(t);
			}
		}
		else if (state == GameState::InBattle) {
			window.clear(sf::Color(40, 40, 60)); // Dark battle background

			if (fontOk && player && currentEnemy) {
				// Draw Player Box and Name
				window.draw(playerBox);
				playerBattleName.setString(player->name);
				playerBattleName.setPosition(playerBox.getPosition().x + 20, playerBox.getPosition().y - 70);
				window.draw(playerBattleName);

				// Draw Enemy Box and Name
				window.draw(enemyBox);
				enemyBattleName.setString(currentEnemy->name);
				enemyBattleName.setPosition(enemyBox.getPosition().x + 20, enemyBox.getPosition().y - 70);
				window.draw(enemyBattleName);

				// Draw Player HP Bar
				float playerHpPercent = static_cast<float>(player->hp) / player->maxHp;
				if (playerHpPercent < 0) playerHpPercent = 0;
				playerHpBarBack.setPosition(playerBattleName.getPosition().x, playerBattleName.getPosition().y + 40);
				playerHpBarFront.setPosition(playerBattleName.getPosition().x, playerBattleName.getPosition().y + 40);
				playerHpBarFront.setSize(sf::Vector2f(BAR_WIDTH * playerHpPercent, BAR_HEIGHT));
				window.draw(playerHpBarBack);
				window.draw(playerHpBarFront);

				// Draw Enemy HP Bar
				float enemyHpPercent = static_cast<float>(currentEnemy->hp) / currentEnemy->maxHp;
				if (enemyHpPercent < 0) enemyHpPercent = 0;
				enemyHpBarBack.setPosition(enemyBattleName.getPosition().x, enemyBattleName.getPosition().y + 40);
				enemyHpBarFront.setPosition(enemyBattleName.getPosition().x, enemyBattleName.getPosition().y + 40);
				enemyHpBarFront.setSize(sf::Vector2f(BAR_WIDTH * enemyHpPercent, BAR_HEIGHT));
				window.draw(enemyHpBarBack);
				window.draw(enemyHpBarFront);

				// Draw Battle Log
				battleLogText.setString(battleMessage);
				window.draw(battleLogText);
			}

			// draw battle buttons
			for (auto &b : battleButtons) {
				window.draw(b.rect);
				if (fontOk) window.draw(b.label);
			}
		}

		// GameOver / Victory screens overlay (UIManager EndScreen logic)
		if (state == GameState::GameOver) {
			sf::RectangleShape overlay(sf::Vector2f(WINDOW_W, WINDOW_H));
			overlay.setFillColor(sf::Color(0,0,0,180));
			window.draw(overlay);
			if (fontOk) {
				sf::Text go("GAME OVER (YOU DIED)", font, 48); go.setFillColor(sf::Color::Red);
				sf::FloatRect goRect = go.getLocalBounds();
				go.setPosition(WINDOW_W/2 - goRect.width/2 - goRect.left, WINDOW_H/2 - goRect.height/2 - goRect.top);
				window.draw(go);
			}
		}
		if (state == GameState::Victory) {
			sf::RectangleShape overlay(sf::Vector2f(WINDOW_W, WINDOW_H));
			overlay.setFillColor(sf::Color(0,255,0,200));
			window.draw(overlay);
			if (fontOk) {
				sf::Text txt("VICTORY", font, 48); txt.setFillColor(sf::Color::Black);
				sf::FloatRect txtRect = txt.getLocalBounds();
				txt.setPosition(WINDOW_W/2 - txtRect.width/2 - txtRect.left, WINDOW_H/2 - txtRect.height/2 - txtRect.top);
				window.draw(txt);
			}
		}
		window.display();
	}
	return 0;
}