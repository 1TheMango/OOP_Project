#pragma once

#include <SFML/Graphics.hpp>
#include "entity.h" // Needs Player definition for onEnter

// --- Tile Hierarchy (UML) ---------------------------------------------------
class Tile {
protected:
	sf::Sprite sprite;
public:
	virtual ~Tile() {}
	// Position/isVisited is handled by Board/Game logic for simplicity
	
	virtual bool isBlocked() const { return false; }
	virtual bool isMonster() const { return false; }
	virtual bool isBoss() const { return false; } // Added for BossTile
	virtual bool isExit() const { return false; }
	
	// onEnter(Player) as per UML
	virtual void onEnter(Player* p); // Definition in .cpp
	
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
	bool combatTriggered = false; // To prevent infinite battle loop on 'Run'
public: 
	bool isMonster() const override { return true; } 
	void onEnter(Player* p) override;
	bool shouldTriggerCombat() const { return combatTriggered; }
	void resetCombatTrigger() { combatTriggered = false; }
};

// New BossTile class per UML
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