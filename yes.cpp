#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>

enum GameState {
    START_SCREEN,
    PLAYING
};

int main() {
    sf::RenderWindow window(sf::VideoMode(800, 600), "Fight Club");
    window.setFramerateLimit(60);
    
    sf::Font font;
    if (!font.loadFromFile("assets/Arial.ttf")) {
        std::cerr << "Error loading font\n";
        return -1;
    }
    
    GameState gameState = START_SCREEN;
    sf::Clock clock;
    float glowTime = 0;
    
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();
        sf::Event event;
        
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
            
            if (event.type == sf::Event::KeyPressed) {
                if (gameState == START_SCREEN && event.key.code == sf::Keyboard::Return) {
                    gameState = PLAYING;
                }
            }
        }
        
        window.clear(sf::Color(20, 20, 40));
        
        if (gameState == START_SCREEN) {
            // Title
            sf::Text title("FIGHT CLUB", font, 80);
            title.setFillColor(sf::Color::Red);
            title.setStyle(sf::Text::Bold);
            title.setPosition(180, 150);
            
            // Subtitle
            sf::Text subtitle("Battle Arena", font, 35);
            subtitle.setFillColor(sf::Color::Yellow);
            subtitle.setPosition(280, 260);
            
            // Controls
            sf::Text controls1("Player 1 Controls:", font, 28);
            controls1.setFillColor(sf::Color::Cyan);
            controls1.setPosition(100, 340);
            
            sf::Text controls2("WASD - Move | SPACE - Attack", font, 22);
            controls2.setFillColor(sf::Color::White);
            controls2.setPosition(100, 375);
            
            sf::Text controls3("Player 2 Controls:", font, 28);
            controls3.setFillColor(sf::Color::Yellow);
            controls3.setPosition(100, 420);
            
            sf::Text controls4("Arrow Keys - Move | ENTER - Attack", font, 22);
            controls4.setFillColor(sf::Color::White);
            controls4.setPosition(100, 455);
            
            // Start prompt with glow animation
            sf::Text start("Press ENTER to Start!", font, 32);
            glowTime += dt * 3;
            int alpha = static_cast<int>(127 + 127 * std::sin(glowTime));
            start.setFillColor(sf::Color(0, 255, 0, alpha));
            start.setPosition(220, 520);
            
            window.draw(title);
            window.draw(subtitle);
            window.draw(controls1);
            window.draw(controls2);
            window.draw(controls3);
            window.draw(controls4);
            window.draw(start);
        }
        else if (gameState == PLAYING) {
            sf::Text playing("Game Starting...", font, 40);
            playing.setFillColor(sf::Color::White);
            playing.setPosition(250, 250);
            window.draw(playing);
        }
        
        window.display();
    }
    
    return 0;
}

// Compile with:
// g++ -o fight_club main.cpp -lsfml-graphics -lsfml-window -lsfml-system