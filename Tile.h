#ifndef TILE_H
#define TILE_H

#include <SFML/Graphics.hpp>
#include "Player.h"

class Tile {
protected:
	sf::Sprite sprite;
public:
	virtual ~Tile() {}
	
	virtual bool isBlocked() const { return false; }
	virtual bool isMonster() const { return false; }
	virtual bool isBoss() const { return false; }
	virtual bool isExit() const { return false; }
	
	virtual void onEnter(Player* p) { (void)p; }
	
	sf::Sprite& getSprite() { return sprite; }
};

class EmptyTile : public Tile {
public:
	void onEnter(Player* p) override;
};

class BlockedTile : public Tile {
public:
	bool isBlocked() const override { return true; }
};

class MonsterTile : public Tile {
private:
	bool combatTriggered = false;
public:
	bool isMonster() const override { return true; }
	void onEnter(Player* p) override;
	bool shouldTriggerCombat() const { return combatTriggered; }
	void resetCombatTrigger() { combatTriggered = false; }
};

class BossTile : public Tile {
private:
	bool combatTriggered = false;
public:
	bool isBoss() const override { return true; }
	void onEnter(Player* p) override;
	bool shouldTriggerCombat() const { return combatTriggered; }
	void resetCombatTrigger() { combatTriggered = false; }
};

class ExitTile : public Tile {
public:
	bool isExit() const override { return true; }
	void onEnter(Player* p) override;
};

#endif