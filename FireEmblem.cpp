// FireEmblem.cpp
// Main game loop file.
// All class definitions are now in .h and .cpp files.
//
// Compile (example for g++):
// g++ FireEmblem.cpp Dice.cpp Entity.cpp CombatSystem.cpp Tile.cpp Board.cpp -o FireEmblem -lsfml-graphics -lsfml-window -lsfml-system

#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <memory> 		// For std::unique_ptr

// Include all our new class headers
#include "Dice.h"
#include "entity.h"
#include "CombatSystem.h"
#include "Tile.h"
#include "Board.h"
#include "Player.cpp" // Player subclasses are in Player.cpp
#include "entity.cpp"
#include "CombatSystem.cpp"
#include "Tile.cpp"
#include "Board.cpp"
using namespace std;

// --- UI Manager (Simplified for single-file SFML) ---------------------------
// The Button struct is retained as a UI Helper.
struct Button {
	sf::RectangleShape rect;
	sf::Text label;
	std::function<void()> onClick;
	bool contains(sf::Vector2f p) const { return rect.getGlobalBounds().contains(p); }
};

// --- Main Game Loop ---------------------------------------------------------
int main() {
	// Dice system initialization is handled by global RNG setup in Dice.cpp
	
	const int ROWS = 10, COLS = 10;
	const float TILE_SIZE = 64.f;
	const int WINDOW_W = int(COLS * TILE_SIZE);
	const int WINDOW_H = int(ROWS * TILE_SIZE + 160); // extra space for battle UI

	sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "FireEmblem - OOP Project");
	window.setFramerateLimit(60);

	// Textures - renamed texNormal to texEmpty for UML compliance
	sf::Texture texEmpty, texBlocked, texMonster, texBoss, texExit, texPlayer;
	if (!texEmpty.loadFromFile("assets/normal.png")) 	cerr << "Warn: missing assets/normal.png\n";
	if (!texBlocked.loadFromFile("assets/blocked.png")) cerr << "Warn: missing assets/blocked.png\n";
	if (!texMonster.loadFromFile("assets/monster.png")) cerr << "Warn: missing assets/monster.png\n";
	if (!texBoss.loadFromFile("assets/boss.png")) 		cerr << "Warn: missing assets/boss.png\n";
	if (!texExit.loadFromFile("assets/exit.png")) 		cerr << "Warn: missing assets/exit.png\n";
	if (!texPlayer.loadFromFile("assets/player.png")) 	cerr << "Warn: missing assets/player.png\n";

	// --- ADD: Battle-specific textures ---
	sf::Texture texSoldierBattle, texArcherBattle, texMageBattle;
	sf::Texture texMonsterBattle, texBossBattle;
	if (!texSoldierBattle.loadFromFile("assets/soldier_battle.png")) cerr << "Warn: missing assets/soldier_battle.png\n";
	if (!texArcherBattle.loadFromFile("assets/archer_battle.png")) 	cerr << "Warn: missing assets/archer_battle.png\n";
	if (!texMageBattle.loadFromFile("assets/mage_battle.png")) 		cerr << "Warn: missing assets/mage_battle.png\n";
	if (!texMonsterBattle.loadFromFile("assets/monster_battle.png")) cerr << "Warn: missing assets/monster_battle.png\n";
	if (!texBossBattle.loadFromFile("assets/boss_battle.png")) 		cerr << "Warn: missing assets/boss_battle.png\n";
	

	sf::Font font;
	bool fontOk = true;
	if (!font.loadFromFile("assets/font.ttf")) {
		cerr << "Warn: missing assets/font.ttf (UI labels will be empty). Put a TTF at assets/font.ttf\n";
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
	std::string battleMessage; // For the new battle log
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
	sf::Text titleText("FireEmblem", font, 48); // Renamed title
	titleText.setFillColor(sf::Color::White);
	titleText.setPosition(WINDOW_W/2 - titleText.getLocalBounds().width/2, 100);
	sf::Text subtitleText("Select your class:", font, 24);
	subtitleText.setFillColor(sf::Color::White);
	subtitleText.setPosition(WINDOW_W/2 - subtitleText.getLocalBounds().width/2, 180);

	// --- ADD: Battle Sprites ---
	sf::Sprite playerBattleSprite;
	sf::Sprite enemyBattleSprite;

	vector<Button> menuButtons;
	menuButtons.push_back(makeButton(WINDOW_W/2 - 100, 250, 200, 50, "Soldier", font, [&](){
		player = std::make_unique<Soldier>(playerR, playerC);
		playerBattleSprite.setTexture(texSoldierBattle); // <-- SET TEXTURE
		playerBattleSprite.setPosition(100, 200); // <-- SET POSITION
		state = GameState::Exploring;
	}));
	menuButtons.push_back(makeButton(WINDOW_W/2 - 100, 320, 200, 50, "Archer", font, [&](){
		player = std::make_unique<Archer>(playerR, playerC);
		playerBattleSprite.setTexture(texArcherBattle); // <-- SET TEXTURE
		playerBattleSprite.setPosition(100, 200); // <-- SET POSITION
		state = GameState::Exploring;
	}));
	menuButtons.push_back(makeButton(WINDOW_W/2 - 100, 390, 200, 50, "Mage", font, [&](){
		player = std::make_unique<Mage>(playerR, playerC);
		playerBattleSprite.setTexture(texMageBattle); // <-- SET TEXTURE
		playerBattleSprite.setPosition(100, 200); // <-- SET POSITION
		state = GameState::Exploring;
	}));

	// --- Battle UI Buttons ---
	vector<Button> battleButtons;
	const float btnW = 160, btnH = 40;
	//float baseY = ROWS * TILE_SIZE + 30; // <-- This is the old Y
	float battleBtnY = WINDOW_H - 70.f; // <-- New Y for battle screen

	// Helper to start a battle at tile r,c
	auto startBattle = [&](int r, int c, bool isBoss){
		std::cout << "\n--- Battle start ---\n";
		
		if (isBoss) {
			 currentEnemy = std::make_unique<Boss>("Dungeon Lord", 3, 100, 15, 8);
			 enemyBattleSprite.setTexture(texBossBattle); // <-- SET TEXTURE
		} else {
			// Randomly spawn a Monster or a small Boss for variety
			if (D20().roll() > 15) {
				 currentEnemy = std::make_unique<Boss>("Ogre", 1, 25, 10, 5);
				 enemyBattleSprite.setTexture(texBossBattle); // <-- SET TEXTURE
			} else {
				 currentEnemy = std::make_unique<Monster>("Goblin", 18, 5, 2);
				 enemyBattleSprite.setTexture(texMonsterBattle); // <-- SET TEXTURE
			}
		}
		// enemyBattleSprite.setPosition(WINDOW_W - 350, 150); // <-- OLD BUGGY POSITION
		enemyBattleSprite.setPosition(WINDOW_W - 200 - 100, 150); // <-- NEW POSITION (width 200, margin 100)
		
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
	battleButtons.push_back(makeButton(20, battleBtnY, btnW, btnH, "Attack", font, [&](){ // <-- UPDATE: Y position
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
	battleButtons.push_back(makeButton(210, battleBtnY, btnW, btnH, "Defend", font, [&](){ // <-- UPDATE: Y position
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
	battleButtons.push_back(makeButton(400, battleBtnY, btnW, btnH, "Ability", font, [&](){ // <-- UPDATE: Y position
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
	battleButtons.push_back(makeButton(590, battleBtnY, btnW, btnH, "Run", font, [&](){ // <-- UPDATE: Y position
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
							
							// Check for dynamic_cast to get derived class methods
							if (MonsterTile* mt = dynamic_cast<MonsterTile*>(t)) {
								if (mt->shouldTriggerCombat()) {
									startBattle(nr, nc, false); // Start monster battle
								}
							} else if (BossTile* bt = dynamic_cast<BossTile*>(t)) {
								if (bt->shouldTriggerCombat()) {
									startBattle(nr, nc, true); // Start BOSS battle
								}
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
				string s = "Exploring. Move: WASD. Press SPACE to roll (d6). MovePoints: " + std::to_string(movePoints) + 
						   " | " + player->name + " HP: " + std::to_string(player->hp) + "/" + std::to_string(player->maxHp);
				sf::Text t(s, font, 16); t.setFillColor(sf::Color::White); t.setPosition(10, ROWS*TILE_SIZE + 10);
				window.draw(t);
			}
		}
		else if (state == GameState::InBattle) {
			window.clear(sf::Color(40, 40, 60)); // Dark battle background

			if (fontOk && player && currentEnemy) {
				// Draw Player Box and Name
				window.draw(playerBattleSprite);
				playerBattleName.setString(player->name);
				playerBattleName.setPosition(playerBattleSprite.getPosition().x + 20, playerBattleSprite.getPosition().y - 70);
				window.draw(playerBattleName);

				// Draw Enemy Box and Name
				window.draw(enemyBattleSprite);
				enemyBattleName.setString(currentEnemy->name);
				enemyBattleName.setPosition(enemyBattleSprite.getPosition().x + 20, enemyBattleSprite.getPosition().y - 70);
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