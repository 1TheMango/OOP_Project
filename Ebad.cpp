#include <SFML/Graphics.hpp>
#include <iostream>
#include <vector>
#include <string>

// --- Enums ---
enum GameState { MENU, PLAYING, GAME_OVER };
enum ElementType { FIRE, WATER, EARTH, LIGHTNING };

// --- 1. Abstract Base Class: Entity ---
class Entity {
protected:
    sf::RectangleShape shape;
    sf::Vector2f velocity;

public:
    Entity(float x, float y, float w, float h, sf::Color color) {
        shape.setPosition(x, y);
        shape.setSize(sf::Vector2f(w, h));
        shape.setFillColor(color);
    }

    virtual ~Entity() {}

    virtual void update(float dt) = 0;

    virtual void draw(sf::RenderWindow& window) {
        window.draw(shape);
    }

    sf::FloatRect getBounds() { return shape.getGlobalBounds(); }
    sf::Vector2f getPosition() { return shape.getPosition(); }
};

// --- 2. Projectile Class ---
class Projectile : public Entity {
public:
    bool active;
    float speed;

    Projectile(float x, float y, sf::Vector2f direction, sf::Color color) 
        : Entity(x, y, 20.f, 10.f, color), active(true), speed(500.f) {
        velocity = direction;
    }

    void update(float dt) override {
        shape.move(velocity * speed * dt);

        // Deactivate if off screen
        if (shape.getPosition().x < 0 || shape.getPosition().x > 800) {
            active = false;
        }
    }
};

// --- 3. Weapon System ---
class Weapon {
public:
    virtual Projectile* attack(sf::Vector2f origin, sf::Vector2f direction, sf::Color color) = 0;
};

class MagicStaff : public Weapon {
public:
    // Factory method to create projectiles
    Projectile* attack(sf::Vector2f origin, sf::Vector2f direction, sf::Color color) override {
        return new Projectile(origin.x, origin.y, direction, color);
    }
};

// --- 4. Player Class ---
class Player : public Entity {
public:
    std::string name;
    float hp;
    float mp;
    MagicStaff staff; 
    std::vector<ElementType> elementQueue;

    Player(std::string n, float x, float y, sf::Color c) 
        : Entity(x, y, 50.f, 50.f, c), name(n), hp(100), mp(100) {}

    void move(float dx, float dy, float dt) {
        shape.move(dx * 200.f * dt, dy * 200.f * dt);
    }

    // Queue an element (Fire, Water, etc.)
    void queueElement(ElementType type) {
        if (elementQueue.size() < 3) {
            elementQueue.push_back(type);
        }
    }

    Projectile* castSpell(sf::Vector2f direction) {
        // Mana Check
        float cost = 15.0f;
        if (mp >= cost) {
            mp -= cost;
            
            // Determine color based on first element (Visuals only)
            sf::Color spellColor = sf::Color::White;
            if (!elementQueue.empty()) {
                if (elementQueue[0] == FIRE) spellColor = sf::Color::Red;
                else if (elementQueue[0] == WATER) spellColor = sf::Color::Blue;
                else if (elementQueue[0] == EARTH) spellColor = sf::Color::Green;
                else if (elementQueue[0] == LIGHTNING) spellColor = sf::Color::Yellow;
            }
            
            elementQueue.clear(); // Reset queue
            return staff.attack(shape.getPosition(), direction, spellColor);
        }
        return nullptr;
    }

    void update(float dt) override {
        // Mana Regen
        if (mp < 100) mp += 5.f * dt;
    }

    void takeDamage(float amount) {
        hp -= amount;
        if (hp < 0) hp = 0;
    }
};

// --- 5. Managers ---

class ShockManager {
public:
    // The "Twist" - Hardware Feedback
    void sendShock(int intensity) {
        std::cout << ">>> [ARDUINO HARDWARE] ZAP! Sending Shock Signal (Intensity: " << intensity << ") <<<" << std::endl;
    }
};

class CollisionManager {
public:
    bool checkCollision(Projectile* p, Player* target) {
        if (p->active && p->getBounds().intersects(target->getBounds())) {
            p->active = false;
            return true;
        }
        return false;
    }
};

class UIManager {
private:
    sf::Font font;
    sf::Text hudText;
    sf::Text menuText;

public:
    UIManager() {
        // Ensure arial.ttf is in your project directory
        if (!font.loadFromFile("assets/Arial.ttf")) {
            std::cerr << "Error: arial.ttf not found!" << std::endl;
        }

        // HUD Setup
        hudText.setFont(font);
        hudText.setCharacterSize(18);
        hudText.setFillColor(sf::Color::White);
        hudText.setPosition(10, 10);

        // Menu Setup
        menuText.setFont(font);
        menuText.setCharacterSize(40);
        menuText.setStyle(sf::Text::Bold);
        menuText.setPosition(150, 250);
    }

    void drawHUD(sf::RenderWindow& window, Player* p1, Player* p2) {
        std::string info = p1->name + " [HP: " + std::to_string((int)p1->hp) + " MP: " + std::to_string((int)p1->mp) + "]" +
                           "     VS     " +
                           p2->name + " [HP: " + std::to_string((int)p2->hp) + " MP: " + std::to_string((int)p2->mp) + "]";
        hudText.setString(info);
        window.draw(hudText);
    }

    void drawMenu(sf::RenderWindow& window) {
        menuText.setString("FIGHT CLUB\n\nPress ENTER to Start");
        menuText.setFillColor(sf::Color::Cyan);
        window.draw(menuText);
    }

    void drawGameOver(sf::RenderWindow& window, std::string winner) {
        menuText.setString("GAME OVER\n\n" + winner + " WINS!\nPress ESC to Exit");
        menuText.setFillColor(sf::Color::Red);
        window.draw(menuText);
    }
};

// --- 6. Main Game Engine ---
class Game {
private:
    sf::RenderWindow window;
    GameState state;
    
    Player* player1;
    Player* player2;
    
    std::vector<Projectile*> projectiles;
    
    UIManager ui;
    CollisionManager collisionMgr;
    ShockManager shockHardware;

public:
    Game() : window(sf::VideoMode(800, 600), "Fight Club Project") {
        window.setFramerateLimit(60);
        state = MENU;

        // Initialize Players
        player1 = new Player("Player 1", 50, 300, sf::Color::Blue);
        player2 = new Player("Player 2", 700, 300, sf::Color::Red);
    }

    ~Game() {
        delete player1;
        delete player2;
        for (auto p : projectiles) delete p;
    }

    void run() {
        sf::Clock clock;
        while (window.isOpen()) {
            float dt = clock.restart().asSeconds();
            processEvents();
            update(dt);
            render();
        }
    }

private:
    void processEvents() {
        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) window.close();

            // Menu Controls
            if (state == MENU) {
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Enter) {
                    state = PLAYING; // Switch to Game Screen
                }
            }
            // Game Controls
            else if (state == PLAYING) {
                // Player 1 Casting (Space)
                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::Num1) player1->queueElement(FIRE);
                    if (event.key.code == sf::Keyboard::Space) {
                        Projectile* p = player1->castSpell(sf::Vector2f(1.f, 0.f)); // Shoot Right
                        if (p) projectiles.push_back(p);
                    }
                    // Player 2 Casting (Enter)
                    if (event.key.code == sf::Keyboard::Enter) {
                        Projectile* p = player2->castSpell(sf::Vector2f(-1.f, 0.f)); // Shoot Left
                        if (p) projectiles.push_back(p);
                    }
                }
            }
            // Game Over Controls
            else if (state == GAME_OVER) {
                if (event.type == sf::Event::KeyPressed && event.key.code == sf::Keyboard::Escape) {
                    window.close();
                }
            }
        }
    }

    void update(float dt) {
        if (state == PLAYING) {
            // Player Movement
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) player1->move(0, -1, dt);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) player1->move(0, 1, dt);
            
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) player2->move(0, -1, dt);
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) player2->move(0, 1, dt);

            player1->update(dt);
            player2->update(dt);

            // Projectile Logic
            for (auto p : projectiles) {
                if (p->active) p->update(dt);
            }

            // Collision Detection
            for (auto p : projectiles) {
                if (collisionMgr.checkCollision(p, player2)) {
                    player2->takeDamage(10);
                    shockHardware.sendShock(10); // Trigger Hardware
                } 
                else if (collisionMgr.checkCollision(p, player1)) {
                    player1->takeDamage(10);
                    shockHardware.sendShock(10); // Trigger Hardware
                }
            }

            // Victory Condition
            if (player1->hp <= 0 || player2->hp <= 0) {
                state = GAME_OVER;
            }
        }
    }

    void render() {
        window.clear(sf::Color::Black);

        if (state == MENU) {
            ui.drawMenu(window);
        }
        else if (state == PLAYING) {
            player1->draw(window);
            player2->draw(window);
            
            for (auto p : projectiles) {
                if (p->active) p->draw(window);
            }
            
            ui.drawHUD(window, player1, player2);
        }
        else if (state == GAME_OVER) {
            if (player1->hp > 0) ui.drawGameOver(window, "PLAYER 1");
            else ui.drawGameOver(window, "PLAYER 2");
        }

        window.display();
    }
};

int main() {
    Game game;
    game.run();
    return 0;
}