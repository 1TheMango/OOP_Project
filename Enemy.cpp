#include "Enemy.h"

Enemy::Enemy(std::string n, int m, int a, int d) 
	: Entity(n, m, a, d) {}

int Enemy::calculateDamage() {
	return d6.roll() + attack;
}

Monster::Monster(std::string t, int m, int a, int d) 
	: Enemy(t, m, a, d), type(t) {}

Boss::Boss(std::string n, int l, int m, int a, int d) 
	: Enemy(n, m, a, d), level(l) {
	name = "Boss " + n;
}

int Boss::calculateDamage() {
	return d6.roll() + d6.roll() + attack;
}