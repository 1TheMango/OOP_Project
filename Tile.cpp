#include "Tile.h"
#include <iostream>

// --- Tile Methods ---
// Base class onEnter does nothing
void Tile::onEnter(Player* p) { 
	(void)p; // Suppress "unused parameter" warning
}

void EmptyTile::onEnter(Player* p) {
	// std::cout << "[Event] Empty tile entered.\n";
}

void MonsterTile::onEnter(Player* p) {
	if (!combatTriggered) {
		std::cout << "[Event] Monster encountered (board)\n"; 
		combatTriggered = true;
	}
}

void BossTile::onEnter(Player* p) {
	if (!combatTriggered) {
		std::cout << "[Event] BOSS encountered (board)\n";
		combatTriggered = true;
	}
}

void ExitTile::onEnter(Player* p) {
	std::cout << "[Event] Exit reached\n"; 
}