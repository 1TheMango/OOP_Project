#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <functional>
#include <algorithm> // For min/max
#include <sstream>  

#include "include/Dice.h"
#include "include/Player.h"
#include "include/Enemy.h"
#include "include/CombatSystem.h"
#include "include/Board.h"
#include "include/Tile.h"
#include "include/GameState.h"
#include "include/UIButton.h"

using namespace std;

// --- HELPER FUNCTION: CREATE BUTTON ---
Button createButton(float x, float y, float w, float h, const string& txt, sf::Font& font, bool fontOk, function<void()> cb) {
    Button b;
    b.rect = sf::RectangleShape(sf::Vector2f(w, h));
    b.rect.setPosition(x, y);
    b.rect.setFillColor(sf::Color(70, 70, 70));
    b.rect.setOutlineThickness(1.f);
    b.rect.setOutlineColor(sf::Color::Black);

    if (fontOk) {
        b.label.setFont(font);
        b.label.setString(txt);
        b.label.setCharacterSize(18);
        b.label.setFillColor(sf::Color::White);
        sf::FloatRect lb = b.label.getLocalBounds();
        b.label.setPosition(x + (w - lb.width) / 2 - lb.left, y + (h - lb.height) / 2 - lb.top);
    }
    b.onClick = cb;
    return b;
}

// --- HELPER FUNCTION: LOAD LEVEL ---
void loadLevel(int levelIdx, const vector<vector<string>>& allLevels, Board& board, 
               int rows, int cols, int& pStartR, int& pStartC,
               sf::Texture& tEmpty, sf::Texture& tBlocked, 
               sf::Texture& tMonster, sf::Texture& tBoss, sf::Texture& tExit) 
{
    if(levelIdx >= allLevels.size()) return;
    const vector<string>& layout = allLevels[levelIdx];
    
    for (int r = 0; r < rows; r++){
        for (int c = 0; c < cols; c++){
            char ch = layout[r][c];
            if (ch=='N') board.setTile(r,c,new EmptyTile(), tEmpty); 
            else if (ch=='B') board.setTile(r,c,new BlockedTile(), tBlocked);
            else if (ch=='M') board.setTile(r,c,new MonsterTile(), tMonster);
            else if (ch=='T') board.setTile(r,c,new BossTile(), tBoss);
            else if (ch=='E') board.setTile(r,c,new ExitTile(), tExit);
            else if (ch=='P') { 
                board.setTile(r,c,new EmptyTile(), tEmpty); 
                pStartR = r; pStartC = c; 
            }
            else board.setTile(r,c,new EmptyTile(), tEmpty);
        }
    }
    cout << "Loaded Level " << levelIdx + 1 << endl;
}

// --- HELPER FUNCTION: START BATTLE ---
void startBattle(int r, int c, bool isBoss, int levelIndex, 
                 Player* player, Enemy*& curEnemy, 
                 CombatSystem*& combatSys, 
                 int& enemyR, int& enemyC, GameState& state, string& msg,
                 stringstream& ss)
{
    ss.str(""); 
    ss.clear();
    ss << "--- Battle start ---\n";

    if (curEnemy) { delete curEnemy; curEnemy = nullptr; }
    if (combatSys) { delete combatSys; combatSys = nullptr; }

    if (isBoss) {
        int hpBonus = levelIndex * 3;
        int atkBonus = levelIndex * 2;
        curEnemy = new Boss("Dungeon Lord", 3, 30+ hpBonus,6+ atkBonus , 8);
    } else {
        int hpBonus = levelIndex * 5;
        int atkBonus = levelIndex * 2;
        if (D20().roll() > 15) curEnemy = new Monster("Ogre", 1, 35 + hpBonus, 6 + atkBonus);
        else curEnemy = new Monster("Goblin", 18, 5 + hpBonus, 2 + atkBonus);
    }
    
    combatSys = new CombatSystem(player, curEnemy, ss);
    
    enemyR = r; 
    enemyC = c;
    state = GameState::InBattle;
    
    player->resetDefend();
    
    ss << player->name << " vs " << curEnemy->name;
    msg = ss.str(); 
}

// --- HELPER FUNCTION: CHECK BATTLE STATUS ---
// MODIFIED: This function NO LONGER deletes the enemy or changes state immediately.
// It simply detects if the battle ended, updates the log, and sets the 'battleOver' flag.
void checkBattleStatus(Player* player, Enemy* curEnemy, 
                    CombatSystem* combatSys, 
                    bool& bossDefeated, bool isLevelBossBattle,
                    stringstream& ss,
                    bool& battleOver, sf::Clock& battleDelayClock)
{
    if (!curEnemy || !combatSys) return;

    if (combatSys->isEnemyDefeated()) {
        ss << "\n" << curEnemy->name << " defeated! +5 HP.";
        player->hp = min(player->hp + 5, player->maxHp);

        if (isLevelBossBattle) {
            bossDefeated = true;
            ss << "\n>>> BOSS DEFEATED! Exit UNLOCKED! <<<";
            cout << ">>> DUNGEON BOSS DEFEATED! The Exit is now UNLOCKED! <<<\n";
        }
        
        // Trigger the end sequence
        battleOver = true;
        battleDelayClock.restart();

    } else if (combatSys->isPlayerDefeated()) {
        ss << "\n" << player->name << " died.";
        cout << player->name << " died. Game Over.\n";
        
        // Trigger the end sequence
        battleOver = true;
        battleDelayClock.restart();
    }
}

int main() {
    const int ROWS = 10, COLS = 10;
    const float TILE_SIZE = 80.f; 
    const int WINDOW_W = int(COLS * TILE_SIZE);
    const int WINDOW_H = int(ROWS * TILE_SIZE + 100); 

    sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "RogueEmblem - OOP Project");
    window.setFramerateLimit(120);

    // --- ASSET LOADING ---
    sf::Texture texEmpty, texBlocked, texMonster, texBoss, texExit, texPlayer;
    if (!texEmpty.loadFromFile("assets/normal.png"))    cerr << "Warn: missing assets/normal.png\n";
    if (!texBlocked.loadFromFile("assets/blocked.png")) cerr << "Warn: missing assets/blocked.png\n";
    if (!texMonster.loadFromFile("assets/monster.png")) cerr << "Warn: missing assets/monster.png\n";
    if (!texBoss.loadFromFile("assets/Boss.jpg"))       cerr << "Warn: missing assets/boss.png\n";
    if (!texExit.loadFromFile("assets/exit.png"))       cerr << "Warn: missing assets/exit.png\n";
    if (!texPlayer.loadFromFile("assets/player2.jpg"))  cerr << "Warn: missing assets/player.png\n";
    sf::Texture texSoldier, texArcher, texMage;
    if (!texSoldier.loadFromFile("assets/soldier.jpg")) cerr << "Warn: missing assets/soldier.jpg\n";
    if (!texArcher.loadFromFile("assets/Archer.png"))   cerr << "Warn: missing assets/archer.jpg\n";
    if (!texMage.loadFromFile("assets/Mage.jpeg"))       cerr << "Warn: missing assets/mage.jpg\n";
    sf::Texture texMenuBg;
    if (!texMenuBg.loadFromFile("assets/menu_bg.jpg")) cerr << "Warn: missing assets/menu_bg.jpg\n";
    sf::Texture texBattleBg, texPortraitPlayer, texPortraitEnemy;
    if (!texBattleBg.loadFromFile("assets/battle_bg.jpg"))           cerr << "Warn: missing assets/battle_bg.png\n";
    if (!texPortraitPlayer.loadFromFile("assets/portrait_player.jpg")) cerr << "Warn: missing assets/portrait_player.png\n";
    if (!texPortraitEnemy.loadFromFile("assets/portrait_enemy.png"))   cerr << "Warn: missing assets/portrait_enemy.png\n";
    
    sf::Font font;
    bool fontOk = true;
    if (!font.loadFromFile("assets/Arial.ttf")) {
        cerr << "Warn: missing assets/Arial.ttf\n";
        fontOk = false;
    }

    // --- LEVEL DATA ---
    vector<vector<string>> allLevels = {
        { "PNNNBNNNNN", "NBBNBNMNBN", "NNNNBBNNNN", "NBNBNBNBNN", "NNNNNNNBNN", "NBNNBNNNNN", "NNBBNBNNBN", "BNNNNNNNNN", "BBNNNNBNMB", "ENNNBNNNNT" },
        { "PBBBNNNNNN", "NBBBNBBNBN", "NNNNNBBMMM", "BBBBBBBBNB", "NNNNNNNNNB", "NBBNBBBBBB", "NBBMMNNNNN", "NNNBBBBBNB", "BNNNNNNNNB", "BBBBBBBNET" },
        { "PNNMNNNMNN", "BBNBNBNBBN", "NNNBNBNBNN", "MBBBBBBBBM", "NNNNNNNNNN", "TBBBNBNBBT", "NNNBNBNBNN", "BBBNNNNNBB", "NNMNNNNMNN", "NNNNETNNNN" }
    };

    int currentLevelIndex = 0;
    int playerStartR = 0, playerStartC = 0;
    Board board(ROWS, COLS, TILE_SIZE);
    sf::Sprite menuBgSprite;
    menuBgSprite.setTexture(texMenuBg);
    float bgScaleX = (float)WINDOW_W / texMenuBg.getSize().x;
    float bgScaleY = (float)WINDOW_H / texMenuBg.getSize().y;
    menuBgSprite.setScale(bgScaleX, bgScaleY);

    loadLevel(currentLevelIndex, allLevels, board, ROWS, COLS, playerStartR, playerStartC, 
              texEmpty, texBlocked, texMonster, texBoss, texExit);

    Player* player = nullptr;
    
    sf::Sprite playerSprite; 
    playerSprite.setTexture(texPlayer); 
    playerSprite.setScale(1.25f, 1.25f);
    int movePoints = 0;
    GameState state = GameState::MainMenu;
    
    // --- BATTLE VARIABLES ---
    string battleMessage;
    std::stringstream battleLogStream; 
    sf::Clock battleDelayClock;
    
    // Flags for flow control
    bool enemyTurnPending = false; // Waiting for enemy to attack?
    bool battleOver = false;       // Has the fight ended (waiting for victory screen delay)?
    
    bool levelBossDefeated = false;     
    bool isFightingLevelBoss = false;   

    Enemy* currentEnemy = nullptr; 
    CombatSystem* combatSystem = nullptr;
    
    int enemyRow = -1, enemyCol = -1;

    // --- UI SETUP ---
    sf::Text titleText("RogueEmblem", font, 48);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition(WINDOW_W/2 - titleText.getLocalBounds().width/2, 100);
    sf::Text subtitleText("Select your class:", font, 24);
    subtitleText.setFillColor(sf::Color::White);
    subtitleText.setPosition(WINDOW_W/2 - subtitleText.getLocalBounds().width/2, 180);

    // --- MAIN MENU BUTTONS ---
    vector<Button> menuButtons;
    sf::RectangleShape playerBox(sf::Vector2f(250, 300));
    playerBox.setTexture(&texPortraitPlayer);               
    playerBox.setPosition(100,350);
    
    menuButtons.push_back(createButton(WINDOW_W/2 - 100, 250, 200, 50, "Soldier", font, fontOk, [&](){
        if(player) delete player; 
        player = new Soldier(playerStartR, playerStartC);
        state = GameState::Exploring;
        playerSprite.setPosition(player->posC * TILE_SIZE, player->posR * TILE_SIZE);
    }));
    
    menuButtons.push_back(createButton(WINDOW_W/2 - 100, 320, 200, 50, "Archer", font, fontOk, [&](){
        if(player) delete player;
        player = new Archer(playerStartR, playerStartC);
        state = GameState::Exploring;
        playerBox.setTexture(&texArcher);
        playerSprite.setPosition(player->posC * TILE_SIZE, player->posR * TILE_SIZE);
    }));
    
    menuButtons.push_back(createButton(WINDOW_W/2 - 100, 390, 200, 50, "Mage", font, fontOk, [&](){
        if(player) delete player;
        player = new Mage(playerStartR, playerStartC);
        state = GameState::Exploring;
        playerBox.setTexture(&texMage); 
        playerSprite.setPosition(player->posC * TILE_SIZE, player->posR * TILE_SIZE);
    }));

    // --- BATTLE BUTTONS ---
    vector<Button> battleButtons;
    const float btnW = 160, btnH = 40;
    float battleBtnY = WINDOW_H - 70.f;

    // BUTTON: ATTACK
    battleButtons.push_back(createButton(20, battleBtnY, btnW, btnH, "Attack", font, fontOk, [&](){
        if (state != GameState::InBattle || !combatSystem) return;
        if (enemyTurnPending || battleOver) return; // Wait for animations/end
        
        battleLogStream.str(""); 
        battleLogStream.clear();

        // 1. Player Attack
        combatSystem->attack();
        
        // 2. Check Result
        checkBattleStatus(player, currentEnemy, combatSystem, levelBossDefeated, isFightingLevelBoss, battleLogStream, battleOver, battleDelayClock);
        
        // 3. If battle continues, queue Enemy Turn
        if (!battleOver) {
             enemyTurnPending = true;
             battleDelayClock.restart();
             battleLogStream << "\n[Enemy is preparing...]";
        }

        battleMessage = battleLogStream.str();
    }));

    // BUTTON: DEFEND
    battleButtons.push_back(createButton(210, battleBtnY, btnW, btnH, "Defend", font, fontOk, [&](){
        if (state != GameState::InBattle || !combatSystem) return;
        if (enemyTurnPending || battleOver) return;

        battleLogStream.str(""); 
        battleLogStream.clear();

        combatSystem->defend(); 
        
        // Always triggers enemy turn
        enemyTurnPending = true;
        battleDelayClock.restart();
        battleLogStream << "\n[Enemy is preparing...]";
        
        battleMessage = battleLogStream.str();
    }));

    // BUTTON: ABILITY
    battleButtons.push_back(createButton(400, battleBtnY, btnW, btnH, "Ability", font, fontOk, [&](){
        if (state != GameState::InBattle || !combatSystem) return;
        if (enemyTurnPending || battleOver) return;
        
        battleLogStream.str(""); 
        battleLogStream.clear();

        combatSystem->ability(); 
        
        checkBattleStatus(player, currentEnemy, combatSystem, levelBossDefeated, isFightingLevelBoss, battleLogStream, battleOver, battleDelayClock);
        
        if (!battleOver) {
             enemyTurnPending = true;
             battleDelayClock.restart();
             battleLogStream << "\n[Enemy is preparing...]";
        }
        
        battleMessage = battleLogStream.str();
    }));

    // BUTTON: RUN
    battleButtons.push_back(createButton(590, battleBtnY, btnW, btnH, "Run", font, fontOk, [&](){
        if (state != GameState::InBattle || !combatSystem) return;
        if (enemyTurnPending || battleOver) return;
        
        battleLogStream.str(""); 
        battleLogStream.clear();

        if (combatSystem->run()) {
            Tile* t = board.getTile(player->posR, player->posC);
            if (MonsterTile* mt = dynamic_cast<MonsterTile*>(t)) { mt->resetCombatTrigger(); }
            if (BossTile* bt = dynamic_cast<BossTile*>(t)) { bt->resetCombatTrigger(); }
            
            delete currentEnemy; currentEnemy = nullptr;
            delete combatSystem; combatSystem = nullptr;
            
            state = GameState::Exploring;
            battleMessage = "You fled!";
        } else {
            // Run failed, enemy turn delayed
            enemyTurnPending = true;
            battleDelayClock.restart();
            battleLogStream << "\n[Enemy is preparing...]";
            battleMessage = battleLogStream.str();
        }
    }));

    // --- BATTLE UI BARS ---
    sf::RectangleShape battleBgRect(sf::Vector2f(WINDOW_W, WINDOW_H));
    battleBgRect.setTexture(&texBattleBg);

    sf::RectangleShape enemyBox(sf::Vector2f(250, 300));
    enemyBox.setTexture(&texPortraitEnemy);     
    enemyBox.setPosition(WINDOW_W - 350, 350);

    sf::Text playerBattleName, enemyBattleName, battleLogText;
    if(fontOk) {
        playerBattleName.setFont(font); playerBattleName.setCharacterSize(24); playerBattleName.setFillColor(sf::Color::White);
        enemyBattleName.setFont(font); enemyBattleName.setCharacterSize(24); enemyBattleName.setFillColor(sf::Color::White);
        
        battleLogText.setFont(font); 
        battleLogText.setCharacterSize(20); 
        battleLogText.setFillColor(sf::Color::White);
        battleLogText.setPosition(50, WINDOW_H - 250);
    }
    
    const float BAR_WIDTH = 200, BAR_HEIGHT = 25;
    sf::RectangleShape playerHpBarBack(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT)); playerHpBarBack.setFillColor(sf::Color(50, 50, 50));
    sf::RectangleShape playerHpBarFront(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT)); playerHpBarFront.setFillColor(sf::Color::Green);
    sf::RectangleShape enemyHpBarBack(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT)); enemyHpBarBack.setFillColor(sf::Color(50, 50, 50));
    sf::RectangleShape enemyHpBarFront(sf::Vector2f(BAR_WIDTH, BAR_HEIGHT)); enemyHpBarFront.setFillColor(sf::Color::Red);

    // --- GAME LOOP ---
    while (window.isOpen()) {
        
        // --- DELAYED EVENTS HANDLING ---
        
        // 1. Handle Enemy Turn Delay
        if (state == GameState::InBattle && enemyTurnPending && !battleOver) {
            if (battleDelayClock.getElapsedTime().asSeconds() > 1.5f) {
                battleLogStream << "\n"; // Spacer
                combatSystem->enemyTurn();
                
                checkBattleStatus(player, currentEnemy, combatSystem, levelBossDefeated, isFightingLevelBoss, battleLogStream, battleOver, battleDelayClock);
                
                battleMessage = battleLogStream.str();
                enemyTurnPending = false;
            }
        }
        

        if (state == GameState::InBattle && battleOver) {
            if (battleDelayClock.getElapsedTime().asSeconds() > 2.0f) {
 
                
                if (combatSystem->isPlayerDefeated()) {
                    state = GameState::GameOver;
                } else {

                    board.replaceWithEmpty(enemyRow, enemyCol, texEmpty);
                    state = GameState::Exploring;
                }

                delete currentEnemy; currentEnemy = nullptr; 
                delete combatSystem; combatSystem = nullptr;

                battleOver = false;
                enemyTurnPending = false;
            }
        }
        
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();

            if (state == GameState::MainMenu) {
                if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mp(ev.mouseButton.x, ev.mouseButton.y);
                    for (auto &b : menuButtons) {
                        if (b.contains(mp)) { b.onClick(); break; }
                    }
                }
            }
            else if (state == GameState::Exploring) {
                if (!player) { state = GameState::MainMenu; continue; }

                if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Space) {
                    D6 moveDice;
                    if(movePoints>0){
                        cout<<"you have movepoints"<<endl;
                    }else{
                    movePoints = moveDice.roll();
                    cout << "[Movement] Rolled d6 = " << movePoints << " move points\n";
                    }
                }
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
                        
                        if (!t) cout << "Cannot move out of bounds" << endl;
                        else if (t->isBlocked()) cout << "Blocked tile" << endl;
                        else {
                            player->posR = nr; player->posC = nc;
                            playerSprite.setPosition(player->posC * TILE_SIZE, player->posR * TILE_SIZE);
                            movePoints--;
                            t->onEnter(player); 
                            
                            // Check Combat Triggers
                            bool trigMonster = t->isMonster() && dynamic_cast<MonsterTile*>(t)->shouldTriggerCombat();
                            bool trigBoss = t->isBoss() && dynamic_cast<BossTile*>(t)->shouldTriggerCombat();
                            
                            if (trigMonster) {
                                isFightingLevelBoss = false; 
                                startBattle(nr, nc, false, currentLevelIndex, player, currentEnemy, combatSystem, enemyRow, enemyCol, state, battleMessage, battleLogStream);
                                enemyTurnPending = false; 
                                battleOver = false;
                            }
                            else if (trigBoss) {
                                isFightingLevelBoss = true; 
                                startBattle(nr, nc, true, currentLevelIndex, player, currentEnemy, combatSystem, enemyRow, enemyCol, state, battleMessage, battleLogStream);
                                enemyTurnPending = false;
                                battleOver = false;
                            }
                            else if (t->isExit()) {
                                if (!levelBossDefeated) {
                                    cout << "[LOCKED] The exit is locked! You must defeat the Boss ('T') first.\n";
                                }
                                else if (currentLevelIndex < allLevels.size() - 1) {
                                    cout << "Level " << currentLevelIndex + 1 << " Cleared! Proceeding...\n";
                                    currentLevelIndex++;
                                    levelBossDefeated = false; 

                                    loadLevel(currentLevelIndex, allLevels, board, ROWS, COLS, playerStartR, playerStartC, 
                                              texEmpty, texBlocked, texMonster, texBoss, texExit);
                                    player->posR = playerStartR; player->posC = playerStartC;
                                    playerSprite.setPosition(player->posC * TILE_SIZE, player->posR * TILE_SIZE);
                                    movePoints = 0; 
                                } else {
                                    cout << "Victory!\n";
                                    state = GameState::Victory;
                                }
                            }
                        }
                    }
                }
            }
            else if (state == GameState::InBattle) {
                if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
                    // Prevent interaction if busy
                    if (enemyTurnPending || battleOver) continue;

                    sf::Vector2f mp(ev.mouseButton.x, ev.mouseButton.y);
                    for (auto &b : battleButtons) {
                        if (b.contains(mp)) { b.onClick(); break; }
                    }
                }
            }
        }

        window.clear(sf::Color(25,25,25));

        if (state == GameState::MainMenu) {
            window.draw(menuBgSprite);
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
            if (fontOk && player) {
                string s = "Lvl " + to_string(currentLevelIndex+1) + " | Move: WASD | SPACE(roll): " + to_string(movePoints) +  " | " + player->name + " HP: " + to_string(player->hp) + "/" + to_string(player->maxHp);
                if (!levelBossDefeated) s += " | Exit: LOCKED";
                else s += " | Exit: OPEN";

                sf::Text t(s, font, 16); t.setFillColor(sf::Color::White); t.setPosition(10, ROWS*TILE_SIZE + 10);
                window.draw(t);
            }
        }
        else if (state == GameState::InBattle) {
            window.draw(battleBgRect);
            // Draw UI even if battleOver is true (so we can see the result)
            if (fontOk && player && currentEnemy) {
                window.draw(playerBox);
                playerBattleName.setString(player->name);
                playerBattleName.setPosition(playerBox.getPosition().x + 20, playerBox.getPosition().y - 70);
                window.draw(playerBattleName);

                window.draw(enemyBox);
                enemyBattleName.setString(currentEnemy->name);
                enemyBattleName.setPosition(enemyBox.getPosition().x + 20, enemyBox.getPosition().y - 70);
                window.draw(enemyBattleName);

                float playerHpPercent = max(0.f, static_cast<float>(player->hp) / player->maxHp);
                playerHpBarBack.setPosition(playerBattleName.getPosition().x, playerBattleName.getPosition().y + 40);
                playerHpBarFront.setPosition(playerBattleName.getPosition().x, playerBattleName.getPosition().y + 40);
                playerHpBarFront.setSize(sf::Vector2f(BAR_WIDTH * playerHpPercent, BAR_HEIGHT));
                window.draw(playerHpBarBack); window.draw(playerHpBarFront);

                float enemyHpPercent = max(0.f, static_cast<float>(currentEnemy->hp) / currentEnemy->maxHp);
                enemyHpBarBack.setPosition(enemyBattleName.getPosition().x, enemyBattleName.getPosition().y + 40);
                enemyHpBarFront.setPosition(enemyBattleName.getPosition().x, enemyBattleName.getPosition().y + 40);
                enemyHpBarFront.setSize(sf::Vector2f(BAR_WIDTH * enemyHpPercent, BAR_HEIGHT));
                window.draw(enemyHpBarBack); window.draw(enemyHpBarFront);
                
                battleLogText.setString(battleMessage);
                window.draw(battleLogText);
            }
            
            for (auto &b : battleButtons) {
                // Dim buttons if waiting
                if(enemyTurnPending || battleOver) b.rect.setFillColor(sf::Color(40,40,40)); 
                else b.rect.setFillColor(sf::Color(70,70,70)); 
                
                window.draw(b.rect);
                if (fontOk) window.draw(b.label);
            }
        }

        if (state == GameState::GameOver) {
            sf::RectangleShape overlay(sf::Vector2f(WINDOW_W, WINDOW_H));
            overlay.setFillColor(sf::Color(0,0,0,180));
            window.draw(overlay);
            if (fontOk) {
                sf::Text go("GAME OVER", font, 48); go.setFillColor(sf::Color::Red);
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
                sf::Text txt("ALL LEVELS CLEARED!\n      VICTORY", font, 48); txt.setFillColor(sf::Color::Black);
                sf::FloatRect txtRect = txt.getLocalBounds();
                txt.setPosition(WINDOW_W/2 - txtRect.width/2 - txtRect.left, WINDOW_H/2 - txtRect.height/2 - txtRect.top);
                window.draw(txt);
            }
        }
        window.display();
    }

    if (player) delete player;
    if (currentEnemy) delete currentEnemy;
    if (combatSystem) delete combatSystem;
    
    return 0;
}