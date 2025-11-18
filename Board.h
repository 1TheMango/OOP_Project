#ifndef BOARD_H
#define BOARD_H

#include <SFML/Graphics.hpp>
#include <vector>
#include "Tile.h"

class Board {
private:
	int rows, cols;
	float tileSize;
	std::vector<std::vector<Tile*>> grid;
public:
	Board(int r, int c, float size);
	~Board();
	void setTile(int r, int c, Tile* tile, sf::Texture& tex);
	Tile* getTile(int r, int c);
	void draw(sf::RenderWindow& win);
	void replaceWithEmpty(int r, int c, sf::Texture& texEmpty);
};

#endif