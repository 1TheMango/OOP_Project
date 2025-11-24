
//g++ main.cpp Dice.cpp Entity.cpp Enemy.cpp Player.cpp CombatSystem.cpp Tile.cpp Board.cpp -o RogueEmblem -lsfml-graphics -lsfml-window -lsfml-system

#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <memory>

#include "Dice.h"
#include "Player.h"
#include "Enemy.h"
#include "CombatSystem.h"
#include "Board.h"
#include "Tile.h"
#include "GameState.h"
#include "UIButton.h"

using namespace std;

int main() {
    const int ROWS = 10, COLS = 10;
    
    // --- RESOLUTION SETTINGS CHANGED HERE ---
    // Tile size 80 * 10 Cols = 800 Width
    const float TILE_SIZE = 80.f; 
    const int WINDOW_W = int(COLS * TILE_SIZE);
    
    // Board is 800px tall. Added 100px for UI = 900 Height
    const int WINDOW_H = int(ROWS * TILE_SIZE + 100); 

    sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "RogueEmblem - OOP Project");
    window.setFramerateLimit(60);

    // Load textures
    sf::Texture texEmpty, texBlocked, texMonster, texBoss, texExit, texPlayer;
    // Note: Ensure these images exist in your assets folder
    if (!texEmpty.loadFromFile("assets/normal.png"))    cerr << "Warn: missing assets/normal.png\n";
    if (!texBlocked.loadFromFile("assets/blocked.png")) cerr << "Warn: missing assets/blocked.png\n";
    if (!texMonster.loadFromFile("assets/monster.png")) cerr << "Warn: missing assets/monster.png\n";
    if (!texBoss.loadFromFile("assets/boss.png"))       cerr << "Warn: missing assets/boss.png\n";
    if (!texExit.loadFromFile("assets/exit.png"))       cerr << "Warn: missing assets/exit.png\n";
    if (!texPlayer.loadFromFile("assets/player2.jpg"))   cerr << "Warn: missing assets/player.png\n";
	sf::Texture texBattleBg, texPortraitPlayer, texPortraitEnemy;
    if (!texBattleBg.loadFromFile("assets/battle_bg.jpg"))          cerr << "Warn: missing assets/battle_bg.png\n";
    if (!texPortraitPlayer.loadFromFile("assets/portrait_player.jpg")) cerr << "Warn: missing assets/portrait_player.png\n";
    if (!texPortraitEnemy.loadFromFile("assets/portrait_enemy.png"))   cerr << "Warn: missing assets/portrait_enemy.png\n";

    sf::Font font;
    bool fontOk = true;
    if (!font.loadFromFile("assets/Arial.ttf")) {
        cerr << "Warn: missing assets/Arial.ttf\n";
        fontOk = false;
    }

    // Create board and load level
    Board board(ROWS, COLS, TILE_SIZE);
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
        {'E','N','N','N','B','N','N','N','N','T'}
    };

    int playerR = 0, playerC = 0;
    for (int r=0; r<ROWS; r++){
        for (int c=0; c<COLS; c++){
            char ch = LEVEL[r][c];
            if (ch=='N') board.setTile(r,c,new EmptyTile(), texEmpty); 
            else if (ch=='B') board.setTile(r,c,new BlockedTile(), texBlocked);
            else if (ch=='M') board.setTile(r,c,new MonsterTile(), texMonster);
            else if (ch=='T') board.setTile(r,c,new BossTile(), texBoss);
            else if (ch=='E') board.setTile(r,c,new ExitTile(), texExit);
            else if (ch=='P') { board.setTile(r,c,new EmptyTile(), texEmpty); playerR=r; playerC=c; }
        }
    }

    unique_ptr<Player> player = nullptr;
    
    sf::Sprite playerSprite; 
    playerSprite.setTexture(texPlayer); 
    // --- UPDATED SCALE ---
    // Scaled up to 1.25x so it fits nicely in the 80px tiles (assuming source is 64px)
    playerSprite.setScale(1.25f, 1.25f);

    int movePoints = 0;

    GameState state = GameState::MainMenu;
    string battleMessage;
    unique_ptr<Enemy> currentEnemy = nullptr; 
    unique_ptr<CombatSystem> combatSystem = nullptr;
    int enemyRow = -1, enemyCol = -1;

    // Button factory lambda
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

    // Main menu UI
    sf::Text titleText("RogueEmblem", font, 48);
    titleText.setFillColor(sf::Color::White);
    titleText.setPosition(WINDOW_W/2 - titleText.getLocalBounds().width/2, 100);
    sf::Text subtitleText("Select your class:", font, 24);
    subtitleText.setFillColor(sf::Color::White);
    subtitleText.setPosition(WINDOW_W/2 - subtitleText.getLocalBounds().width/2, 180);

    vector<Button> menuButtons;
    menuButtons.push_back(makeButton(WINDOW_W/2 - 100, 250, 200, 50, "Soldier", font, [&](){
        player = make_unique<Soldier>(playerR, playerC);
        state = GameState::Exploring;
    }));
    menuButtons.push_back(makeButton(WINDOW_W/2 - 100, 320, 200, 50, "Archer", font, [&](){
        player = make_unique<Archer>(playerR, playerC);
        state = GameState::Exploring;
    }));
    menuButtons.push_back(makeButton(WINDOW_W/2 - 100, 390, 200, 50, "Mage", font, [&](){
        player = make_unique<Mage>(playerR, playerC);
        state = GameState::Exploring;
    }));

    vector<Button> battleButtons;
    const float btnW = 160, btnH = 40;
    // Adjusted Y position to stay near bottom of the 900px window
    float battleBtnY = WINDOW_H - 70.f;

    auto startBattle = [&](int r, int c, bool isBoss){
        cout << "\n--- Battle start ---\n";
        
        if (isBoss) {
             currentEnemy = make_unique<Boss>("Dungeon Lord", 3, 100, 15, 8);
        } else {
            if (D20().roll() > 15) {
                 currentEnemy = make_unique<Boss>("Ogre", 1, 25, 10, 5);
            } else {
                 currentEnemy = make_unique<Monster>("Goblin", 18, 5, 2);
            }
        }
        
        combatSystem = make_unique<CombatSystem>(player.get(), currentEnemy.get(), cout);
        enemyRow = r; enemyCol = c;
        state = GameState::InBattle;
        player->resetDefend();
        battleMessage = "Battle started! " + player->name + " vs " + currentEnemy->name;
    };
    
    auto checkBattleEnd = [&](){
        if (!currentEnemy || !combatSystem) return;

        if (combatSystem->isEnemyDefeated()) {
            cout << currentEnemy->name << " defeated! Player gains 5 HP.\n";
            player->hp = min(player->hp + 5, player->maxHp);
            
            board.replaceWithEmpty(enemyRow, enemyCol, texEmpty);

            currentEnemy = nullptr;
            combatSystem = nullptr;
            state = GameState::Exploring;
        } else if (combatSystem->isPlayerDefeated()) {
            cout << player->name << " died. Game Over.\n";
            state = GameState::GameOver;
            currentEnemy = nullptr;
            combatSystem = nullptr;
        }
    };

    // Battle buttons
    battleButtons.push_back(makeButton(20, battleBtnY, btnW, btnH, "Attack", font, [&](){
        if (state != GameState::InBattle || !combatSystem) return;
        battleMessage = player->name + " attacks!\n";
        combatSystem->attack();
        checkBattleEnd(); 
        if (state == GameState::InBattle) { 
            battleMessage += "Enemy attacks!\n";
            combatSystem->enemyTurn();
            checkBattleEnd();
            if(state == GameState::InBattle) battleMessage += "Your turn.";
            else if (state == GameState::GameOver) battleMessage += "You were defeated!";
        } else if (state == GameState::Exploring) {
            battleMessage += "You won the battle!";
        }
    }));

    battleButtons.push_back(makeButton(210, battleBtnY, btnW, btnH, "Defend", font, [&](){
        if (state != GameState::InBattle || !combatSystem) return;
        battleMessage = player->name + " defends!\n";
        combatSystem->defend();
        battleMessage += "Enemy attacks!\n";
        combatSystem->enemyTurn();
        checkBattleEnd();
        if(state == GameState::InBattle) battleMessage += "Your turn.";
        else if (state == GameState::GameOver) battleMessage += "You were defeated!";
    }));

    battleButtons.push_back(makeButton(400, battleBtnY, btnW, btnH, "Ability", font, [&](){
        if (state != GameState::InBattle || !combatSystem) return;
        battleMessage = player->name + " uses ability!\n";
        combatSystem->ability();
        checkBattleEnd();
        if (state == GameState::InBattle) { 
            battleMessage += "Enemy attacks!\n";
            combatSystem->enemyTurn();
            checkBattleEnd();
            if(state == GameState::InBattle) battleMessage += "Your turn.";
            else if (state == GameState::GameOver) battleMessage += "You were defeated!";
        } else if (state == GameState::Exploring) {
            battleMessage += "You won the battle!";
        }
    }));

    battleButtons.push_back(makeButton(590, battleBtnY, btnW, btnH, "Run", font, [&](){
        if (state != GameState::InBattle || !combatSystem) return;
        if (combatSystem->run()) {
            Tile* t = board.getTile(player->posR, player->posC);
            if (MonsterTile* mt = dynamic_cast<MonsterTile*>(t)) { mt->resetCombatTrigger(); }
            if (BossTile* bt = dynamic_cast<BossTile*>(t)) { bt->resetCombatTrigger(); }

            currentEnemy = nullptr;
            combatSystem = nullptr;
            state = GameState::Exploring;
            battleMessage = "You fled!";
        } else {
            battleMessage = "Run failed!\n";
            battleMessage += "Enemy attacks!\n";
            combatSystem->enemyTurn();
            checkBattleEnd();
            if(state == GameState::InBattle) battleMessage += "Your turn.";
            else if (state == GameState::GameOver) battleMessage += "You were defeated!";
        }
    }));

    // Battle screen UI elements
    sf::RectangleShape playerBox(sf::Vector2f(250, 300));
    playerBox.setTexture(&texPortraitPlayer);              
    playerBox.setPosition(100, 200);
    playerBox.setPosition(100, 200);
	sf::RectangleShape battleBgRect(sf::Vector2f(WINDOW_W, WINDOW_H));
    battleBgRect.setTexture(&texBattleBg);

    sf::RectangleShape enemyBox(sf::Vector2f(250, 300));
    enemyBox.setTexture(&texPortraitEnemy);     
    enemyBox.setPosition(WINDOW_W - 350, 150);
    // Adjusted position relative to new width
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

    // Main loop
    while (window.isOpen()) {
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();

            if (state == GameState::MainMenu) {
                if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mp(ev.mouseButton.x, ev.mouseButton.y);
                    for (auto &b : menuButtons) {
                        if (b.contains(mp)) {
                            b.onClick();
                            if (player) {
                                playerSprite.setPosition(player->posC * TILE_SIZE, player->posR * TILE_SIZE);
                            }
                            break;
                        }
                    }
                }
            }
            else if (state == GameState::Exploring) {
                if (!player) {
                    state = GameState::MainMenu;
                    continue;
                }

                if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Space) {
                    D6 moveDice;
                    movePoints = moveDice.roll();
                    cout << "[Movement] Rolled d6 = " << movePoints << " move points\n";
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
                        
                        if (!t) { cout << "Cannot move out of bounds" << endl; }
                        else if (t->isBlocked()) { cout << "Blocked tile" << endl; }
                        else {
                            player->posR = nr; player->posC = nc;
                            playerSprite.setPosition(player->posC * TILE_SIZE, player->posR * TILE_SIZE);
                            movePoints--;
                            
                            t->onEnter(player.get());
                            
                            if (t->isMonster() && dynamic_cast<MonsterTile*>(t)->shouldTriggerCombat()) {
                                startBattle(nr, nc, false);
                            } else if (t->isBoss() && dynamic_cast<BossTile*>(t)->shouldTriggerCombat()) {
                                startBattle(nr, nc, true);
                            } else if (t->isExit()) {
                                cout << "You reached the exit - Victory!\n";
                                state = GameState::Victory;
                            }
                        }
                    }
                }
            }
            else if (state == GameState::InBattle) {
                if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mp(ev.mouseButton.x, ev.mouseButton.y);
                    for (auto &b : battleButtons) {
                        if (b.contains(mp)) {
                            b.onClick();
                            break;
                        }
                    }
                }
            }
        }

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

            if (fontOk && player) {
                string s = "Exploring. Move: WASD. Press SPACE to roll (d6). MovePoints: " + to_string(movePoints) + 
                           " | " + player->name + " HP: " + to_string(player->hp) + "/" + to_string(player->maxHp);
                sf::Text t(s, font, 16); 
                t.setFillColor(sf::Color::White); 
                // Adjusted text position for 900 height
                t.setPosition(10, ROWS*TILE_SIZE + 10);
                window.draw(t);
            }
        }
        else if (state == GameState::InBattle) {
            window.draw(battleBgRect);

            if (fontOk && player && currentEnemy) {
                window.draw(playerBox);
                playerBattleName.setString(player->name);
                playerBattleName.setPosition(playerBox.getPosition().x + 20, playerBox.getPosition().y - 70);
                window.draw(playerBattleName);

                window.draw(enemyBox);
                enemyBattleName.setString(currentEnemy->name);
                enemyBattleName.setPosition(enemyBox.getPosition().x + 20, enemyBox.getPosition().y - 70);
                window.draw(enemyBattleName);

                float playerHpPercent = static_cast<float>(player->hp) / player->maxHp;
                if (playerHpPercent < 0) playerHpPercent = 0;
                playerHpBarBack.setPosition(playerBattleName.getPosition().x, playerBattleName.getPosition().y + 40);
                playerHpBarFront.setPosition(playerBattleName.getPosition().x, playerBattleName.getPosition().y + 40);
                playerHpBarFront.setSize(sf::Vector2f(BAR_WIDTH * playerHpPercent, BAR_HEIGHT));
                window.draw(playerHpBarBack);
                window.draw(playerHpBarFront);

                float enemyHpPercent = static_cast<float>(currentEnemy->hp) / currentEnemy->maxHp;
                if (enemyHpPercent < 0) enemyHpPercent = 0;
                enemyHpBarBack.setPosition(enemyBattleName.getPosition().x, enemyBattleName.getPosition().y + 40);
                enemyHpBarFront.setPosition(enemyBattleName.getPosition().x, enemyBattleName.getPosition().y + 40);
                enemyHpBarFront.setSize(sf::Vector2f(BAR_WIDTH * enemyHpPercent, BAR_HEIGHT));
                window.draw(enemyHpBarBack);
                window.draw(enemyHpBarFront);

                battleLogText.setString(battleMessage);
                window.draw(battleLogText);
            }

            for (auto &b : battleButtons) {
                window.draw(b.rect);
                if (fontOk) window.draw(b.label);
            }
        }

        if (state == GameState::GameOver) {
            sf::RectangleShape overlay(sf::Vector2f(WINDOW_W, WINDOW_H));
            overlay.setFillColor(sf::Color(0,0,0,180));
            window.draw(overlay);
            if (fontOk) {
                sf::Text go("GAME OVER (YOU DIED)", font, 48); 
                go.setFillColor(sf::Color::Red);
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
                sf::Text txt("VICTORY", font, 48); 
                txt.setFillColor(sf::Color::Black);
                sf::FloatRect txtRect = txt.getLocalBounds();
                txt.setPosition(WINDOW_W/2 - txtRect.width/2 - txtRect.left, WINDOW_H/2 - txtRect.height/2 - txtRect.top);
                window.draw(txt);
            }
        }
        window.display();
    }
    return 0;
}