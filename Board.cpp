#include "Board.h"

Board::Board(int r, int c, float size) : rows(r), cols(c), tileSize(size) {
	grid.resize(rows, std::vector<Tile*>(cols, nullptr));
}

Board::~Board() {
	for (int r=0; r<rows; r++) {
		for (int c=0; c<cols; c++) {
			delete grid[r][c];
		}
	}
}

void Board::setTile(int r, int c, Tile* tile, sf::Texture& tex) {
	if (!tile) return;
	tile->getSprite().setTexture(tex);
	tile->getSprite().setPosition(c * tileSize, r * tileSize);
	grid[r][c] = tile;
}

Tile* Board::getTile(int r,int c) {
	if (r<0 || c<0 || r>=rows || c>=cols) return nullptr;
	return grid[r][c];
}

void Board::draw(sf::RenderWindow& win) {
	for (int r=0; r<rows; r++) {
		for (int c=0; c<cols; c++) {
			if (grid[r][c]) {
				win.draw(grid[r][c]->getSprite());
			}
		}
	}
}

void Board::replaceWithEmpty(int r,int c, sf::Texture& texEmpty) {
	delete grid[r][c];
	grid[r][c] = new EmptyTile(); // Use EmptyTile
	grid[r][c]->getSprite().setTexture(texEmpty);
	grid[r][c]->getSprite().setPosition(c * tileSize, r * tileSize);
}