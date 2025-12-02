#include "include/Tile.h"
#include <iostream>

void EmptyTile::onEnter(Player* p) {
	(void)p;
}

void MonsterTile::onEnter(Player* p) {
	(void)p;
	if (!combatTriggered) {
		std::cout << "[Event] Monster encountered (board)\n";
		combatTriggered = true;
	}
}

void BossTile::onEnter(Player* p) {
	(void)p;
	if (!combatTriggered) {
		std::cout << "[Event] BOSS encountered (board)\n";
		combatTriggered = true;
	}
}

void ExitTile::onEnter(Player* p) {
	(void)p;
	std::cout << "[Event] Exit reached\n";
}