#include "Entity.h"

Entity::Entity(std::string n, int m, int a, int d) 
	: name(n), hp(m), maxHp(m), attack(a), defense(d) {}

void Entity::takeDamage(int dmg) {
	if (defending) {
		dmg = dmg / 2;
		std::cout << name << " defended; damage halved to " << dmg << "\n";
	}
	hp -= dmg;
	if (hp < 0) hp = 0;
}