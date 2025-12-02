#ifndef UIBUTTON_H
#define UIBUTTON_H

#include <SFML/Graphics.hpp>
#include <functional>

struct Button {
	sf::RectangleShape rect;
	sf::Text label;
	std::function<void()> onClick;
	bool contains(sf::Vector2f p) const { 
		return rect.getGlobalBounds().contains(p); 
	}
};

#endif