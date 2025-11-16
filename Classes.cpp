// Classes.cpp
// Single-file SFML game with dice movement and integrated turn-based + dice battle UI
// Compile:
// g++ Classes.cpp -o Classes -lsfml-graphics -lsfml-window -lsfml-system

#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <optional>
#include <functional>  // REQUIRED for std::function
#include <random>      // REQUIRED for std::mt19937
#include <chrono>      // REQUIRED for chrono::high_resolution_clock

using namespace std;

// ----------------------------- Utilities (dice) -----------------------------
static std::mt19937 rng((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count());
int roll_d(int sides) { std::uniform_int_distribution<int> dist(1, sides); return dist(rng); }

// ----------------------------- Tile classes -----------------------------
class Tile {
protected:
    sf::Sprite sprite;
public:
    virtual ~Tile() {}
    virtual bool isBlocked() const { return false; }
    virtual bool isMonster() const { return false; }
    virtual bool isExit() const { return false; }
    virtual void onEnter() {}
    sf::Sprite& getSprite() { return sprite; }
};

class NormalTile : public Tile {};
class BlockedTile : public Tile { public: bool isBlocked() const override { return true; } };
class MonsterTile : public Tile { public: bool isMonster() const override { return true; } void onEnter() override { cout << "[Event] Monster encountered (board)\n"; } };
class ExitTile : public Tile { public: bool isExit() const override { return true; } };

// ----------------------------- Board -----------------------------
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
    void replaceWithNormal(int r,int c, sf::Texture& texNormal) {
        delete grid[r][c];
        grid[r][c] = new NormalTile();
        grid[r][c]->getSprite().setTexture(texNormal);
        grid[r][c]->getSprite().setPosition(c * tileSize, r * tileSize);
    }
};

// ----------------------------- Entities -----------------------------------
struct Actor {
    string name;
    int hp;
    int maxHp;
    int attack;
    int defense;
    bool defending = false;
};

enum class GameState { Exploring, InBattle, GameOver, Victory };

// ----------------------------- UI Helpers ---------------------------------
struct Button {
    sf::RectangleShape rect;
    sf::Text label;
    std::function<void()> onClick;
    bool contains(sf::Vector2f p) const { return rect.getGlobalBounds().contains(p); }
};

int main() {
    srand((unsigned)time(NULL));
    const int ROWS = 10, COLS = 10;
    const float TILE_SIZE = 64.f;
    const int WINDOW_W = int(COLS * TILE_SIZE);
    const int WINDOW_H = int(ROWS * TILE_SIZE + 160); // extra space for battle UI

    sf::RenderWindow window(sf::VideoMode(WINDOW_W, WINDOW_H), "Tile RPG - Battle UI + Dice");
    window.setFramerateLimit(60);

    sf::Texture texNormal, texBlocked, texMonster, texExit, texPlayer;
    if (!texNormal.loadFromFile("assets/normal.png"))  cerr << "Warn: missing assets/normal.png\n";
    if (!texBlocked.loadFromFile("assets/blocked.png")) cerr << "Warn: missing assets/blocked.png\n";
    if (!texMonster.loadFromFile("assets/monster.png")) cerr << "Warn: missing assets/monster.png\n";
    if (!texExit.loadFromFile("assets/exit.png")) cerr << "Warn: missing assets/exit.png\n";
    if (!texPlayer.loadFromFile("assets/player.png")) cerr << "Warn: missing assets/player.png\n";

    sf::Font font;
    bool fontOk = true;
    if (!font.loadFromFile("assets/font.ttf")) {
        cerr << "Warn: missing assets/font.ttf (UI labels will be empty). Put a TTF at assets/font.ttf\n";
        fontOk = false;
    }

    // Create board and load static level
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
        {'N','N','N','N','B','N','N','N','N','E'}
    };

    int playerR = 0, playerC = 0;
    for (int r=0;r<ROWS;r++){
        for (int c=0;c<COLS;c++){
            char ch = LEVEL[r][c];
            if (ch=='N') board.setTile(r,c,new NormalTile(), texNormal);
            else if (ch=='B') board.setTile(r,c,new BlockedTile(), texBlocked);
            else if (ch=='M') board.setTile(r,c,new MonsterTile(), texMonster);
            else if (ch=='E') board.setTile(r,c,new ExitTile(), texExit);
            else if (ch=='P') { board.setTile(r,c,new NormalTile(), texNormal); playerR=r; playerC=c; }
        }
    }

    // Player actor & sprite
    Actor playerActor{ "Hero", 40, 40, 6, 3, false };
    sf::Sprite playerSprite; playerSprite.setTexture(texPlayer); playerSprite.setScale(0.9f,0.9f);
    playerSprite.setPosition(playerC * TILE_SIZE, playerR * TILE_SIZE);

    int playerRow = playerR, playerCol = playerC;
    int movePoints = 0;

    // Battle state
    GameState state = GameState::Exploring;
    optional<Actor> enemy;   // current enemy
    int enemyRow = -1, enemyCol = -1;

    // Battle UI elements
    sf::RectangleShape panel(sf::Vector2f(WINDOW_W - 20, 140));
    panel.setPosition(10, ROWS * TILE_SIZE + 10);
    panel.setFillColor(sf::Color(30,30,30,220));
    panel.setOutlineColor(sf::Color::Black);
    panel.setOutlineThickness(2.f);

    // Buttons: Attack, Defend, Ability, Run
    vector<Button> buttons;
    auto makeButton = [&](float x, float y, float w, float h, const string& txt, function<void()> cb){
        Button b;
        b.rect = sf::RectangleShape(sf::Vector2f(w,h));
        b.rect.setPosition(x,y);
        b.rect.setFillColor(sf::Color(70,70,70));
        b.rect.setOutlineThickness(1.f);
        b.rect.setOutlineColor(sf::Color::Black);
        if (fontOk) {
            b.label.setFont(font);
            b.label.setString(txt);
            b.label.setCharacterSize(18);
            b.label.setFillColor(sf::Color::White);
            sf::FloatRect lb = b.label.getLocalBounds();
            b.label.setPosition(x + (w - lb.width)/2 - lb.left, y + (h - lb.height)/2 - lb.top);
        }
        b.onClick = cb;
        buttons.push_back(b);
    };

    const float btnW = 160, btnH = 40;
    float baseY = ROWS * TILE_SIZE + 30;
    makeButton(20, baseY, btnW, btnH, "Attack", [&](){
        if (state != GameState::InBattle) return;
        // Player attack action: d20 to hit, d6 damage
        int d20 = roll_d(20);
        cout << "[Dice] Player d20 = " << d20 << "\n";
        int hitTarget = 10 + (enemy->defense); // target threshold
        bool crit = (d20 == 20);
        if (d20 + playerActor.attack >= hitTarget || crit) {
            int dmg = roll_d(6) + playerActor.attack;
            if (crit) { dmg += roll_d(6); cout << "CRITICAL! extra d6\n"; }
            cout << "You hit for " << dmg << " damage.\n";
            enemy->hp -= dmg;
        } else {
            cout << "Your attack missed.\n";
        }
    });

    makeButton(210, baseY, btnW, btnH, "Defend", [&](){
        if (state != GameState::InBattle) return;
        playerActor.defending = true;
        cout << "You brace for the next attack (defend).\n";
    });

    makeButton(400, baseY, btnW, btnH, "Ability", [&](){
        if (state != GameState::InBattle) return;
        // Ability: d20 requirement >=15, deals big damage (2*d6 + attack)
        int d20 = roll_d(20); cout << "[Dice] Ability d20 = " << d20 << "\n";
        if (d20 >= 15) {
            int dmg = roll_d(6) + roll_d(6) + playerActor.attack;
            cout << "Ability success! You deal " << dmg << " damage.\n";
            enemy->hp -= dmg;
        } else {
            cout << "Ability failed (roll too low).\n";
        }
    });

    makeButton(590, baseY, btnW, btnH, "Run", [&](){
        if (state != GameState::InBattle) return;
        // Try to run: succeed if d20 >= 12
        int d20 = roll_d(20); cout << "[Dice] Run d20 = " << d20 << "\n";
        if (d20 >= 12) {
            cout << "You fled the battle!\n";
            // return to exploring; enemy remains on tile (no removal)
            state = GameState::Exploring;
            enemy.reset();
        } else {
            cout << "Run failed!\n";
            // proceed to enemy turn
        }
    });

    // HUD text (if font loaded)
    sf::Text hudText;
    if (fontOk) { hudText.setFont(font); hudText.setCharacterSize(16); hudText.setFillColor(sf::Color::White); }

    // Simple turn controller: 0 = player's action waiting; 1 = enemy turn pending
    bool waitingForPlayerAction = false;
    bool enemyTurnPending = false;

    // Helper to start a battle at tile r,c
    auto startBattle = [&](int r, int c){
        cout << "--- Battle start ---"<<endl;
        enemy = Actor{ "Goblin", 18, 18, 5, 2, false };
        enemyRow = r; enemyCol = c;
        state = GameState::InBattle;
        playerActor.defending = false;
        waitingForPlayerAction = true;
        enemyTurnPending = false;
    };

    auto doEnemyTurn = [&](){
        if (!enemy) return;
        cout << "[Enemy Turn] " << enemy->name << " attacks\n";
        int d20 = roll_d(20);
        cout << "[Dice] Enemy d20 = " << d20 << "\n";
        bool crit = (d20 == 20);
        if (d20 + enemy->attack >= 10 + playerActor.defense || crit) {
            int dmg = roll_d(6) + enemy->attack;
            if (crit) { dmg += roll_d(6); cout << "Enemy CRITICAL!\n"; }
            if (playerActor.defending) {
                dmg = dmg/2;
                cout << "You defended; damage halved to " << dmg << "\n";
            }
            playerActor.hp -= dmg;
            cout << "Enemy deals " << dmg << " damage. Player HP = " << playerActor.hp << "\n";
        } else {
            cout << "Enemy missed.\n";
        }
        playerActor.defending = false;
    };

    while (window.isOpen()) {
        sf::Event ev;
        while (window.pollEvent(ev)) {
            if (ev.type == sf::Event::Closed) window.close();

            // Input handling differs by state
            if (state == GameState::Exploring) {
                if (ev.type == sf::Event::KeyPressed && ev.key.code == sf::Keyboard::Space) {
                    // roll movement dice (d6)
                    movePoints = roll_d(6);
                    cout << "[Movement] Rolled d6 = " << movePoints << " move points\n";
                }
                // Movement keys (consume movePoints)
                if (ev.type == sf::Event::KeyPressed && movePoints > 0) {
                    int dr=0, dc=0;
                    if (ev.key.code == sf::Keyboard::W) dr = -1;
                    if (ev.key.code == sf::Keyboard::S) dr = +1;
                    if (ev.key.code == sf::Keyboard::A) dc = -1;
                    if (ev.key.code == sf::Keyboard::D) dc = +1;
                    if (dr!=0 || dc!=0) {
                        int nr = playerRow + dr;
                        int nc = playerCol + dc;
                        Tile* t = board.getTile(nr,nc);
                        if (!t) { cout << "Cannot move out of bounds"<<endl; }
                        else if (t->isBlocked()) { cout << "Blocked tile"<<endl; }
                        else {
                            playerRow = nr; playerCol = nc;
                            playerSprite.setPosition(playerCol * TILE_SIZE, playerRow * TILE_SIZE);
                            movePoints--;
                            // check tile events
                            if (t->isMonster()) {
                                // start battle
                                startBattle(nr, nc);
                            } else if (t->isExit()) {
                                cout << "You reached the exit - Victory!\n";
                                state = GameState::Victory;
                            }
                        }
                    }
                }
            }
            else if (state == GameState::InBattle) {
                // mouse click for button presses
                if (ev.type == sf::Event::MouseButtonPressed && ev.mouseButton.button == sf::Mouse::Left) {
                    sf::Vector2f mp(ev.mouseButton.x, ev.mouseButton.y);
                    for (auto &b : buttons) {
                        if (b.contains(mp)) {
                            // call the button callback
                            b.onClick();
                            // after player action, if still in battle and enemy alive, schedule enemy turn
                            if (state == GameState::InBattle && enemy) {
                                // check enemy death
                                if (enemy->hp <= 0) {
                                    cout << "Enemy defeated!\n";
                                    // remove monster from board -> replace with normal tile
                                    board.replaceWithNormal(enemyRow, enemyCol, texNormal);
                                    enemy.reset();
                                    state = GameState::Exploring;
                                } else {
                                    // enemy gets to act immediately after player's action
                                    doEnemyTurn();
                                    if (playerActor.hp <= 0) {
                                        cout << "You died. Game Over.\n";
                                        state = GameState::GameOver;
                                    }
                                }
                            }
                        }
                    }
                }
            } // end InBattle branch
            else if (state == GameState::GameOver) {
                // nothing - could restart
            }
            else if (state == GameState::Victory) {
                // nothing
            }
        } // end events

        // In-battle auto-checks (e.g., if battle started and player/enemy hp <= 0)
        if (state == GameState::InBattle && enemy) {
            if (enemy->hp <= 0) {
                cout << "Enemy died from earlier action.\n";
                board.replaceWithNormal(enemyRow, enemyCol, texNormal);
                enemy.reset();
                state = GameState::Exploring;
            }
            if (playerActor.hp <= 0) {
                cout << "Player died.\n";
                state = GameState::GameOver;
            }
        }

        // Render
        window.clear(sf::Color(25,25,25));
        board.draw(window);

        // draw player
        window.draw(playerSprite);

        // draw battle UI only if in battle
        if (state == GameState::InBattle) {
            window.draw(panel);

            // Draw HUD texts
            string ph = "Player: " + to_string(playerActor.hp) + " / " + to_string(playerActor.maxHp);
            string eh = "Enemy: " + to_string(enemy->hp) + " / " + to_string(enemy->maxHp);
            if (fontOk) {
                sf::Text t1(ph, font, 18); t1.setFillColor(sf::Color::White); t1.setPosition(20, ROWS*TILE_SIZE + 12);
                sf::Text t2(eh, font, 18); t2.setFillColor(sf::Color::White); t2.setPosition(20, ROWS*TILE_SIZE + 36);
                window.draw(t1); window.draw(t2);
            } else {
                // fallback console HUD
                // (console messages already printed)
            }

            // draw buttons
            for (auto &b : buttons) {
                window.draw(b.rect);
                if (fontOk) window.draw(b.label);
            }
        } else {
            // exploring HUD
            if (fontOk) {
                string s = "Exploring. Move: WASD. Press SPACE to roll movement dice (d6). MovePoints: " + to_string(movePoints);
                sf::Text t(s, font, 16); t.setFillColor(sf::Color::White); t.setPosition(10, ROWS*TILE_SIZE + 10);
                window.draw(t);
            }
        }

        // GameOver / Victory screens overlay
        if (state == GameState::GameOver) {
            sf::RectangleShape overlay(sf::Vector2f(WINDOW_W, WINDOW_H));
            overlay.setFillColor(sf::Color(0,0,0,180));
            window.draw(overlay);
            if (fontOk) {
                sf::Text go("GAME OVER", font, 48); go.setFillColor(sf::Color::Red); go.setPosition(WINDOW_W/2 - 120, WINDOW_H/2 - 40);
                window.draw(go);
            }
        }
        if (state == GameState::Victory) {
            sf::RectangleShape overlay(sf::Vector2f(WINDOW_W, WINDOW_H));
            overlay.setFillColor(sf::Color(255,255,255,200));
            window.draw(overlay);
            if (fontOk) {
                sf::Text txt("YOU WIN", font, 48); txt.setFillColor(sf::Color::Green); txt.setPosition(WINDOW_W/2 - 100, WINDOW_H/2 - 40);
                window.draw(txt);
            }
        }
        window.display();
    }

    return 0;
}