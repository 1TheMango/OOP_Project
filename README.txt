Rogue Emblem is a RPG game inspired by Fire Emblem and Dungeons and Dragons. In the game, you may play as one of three classes of fighters, a soldier, a mage, or an archer. The player will be placed down in a level with the movement and battle mechanics based on rolls of die just 
like Dungeons and Dragons. Each level contains a boss which the player must beat in order to move on to the next level.

Mentioned below are what each class does in the project:

1. Board - Manages the 10x10 game grid, storing and rendering tiles, handling tile placement and replacement.

2. Tile (+ subclasses) - Represents different tile types (Empty, Blocked, Monster, Boss, Exit) with collision detection and event triggers.

3. Entity - Base class for all combat entities, handles HP, attack, defense, and damage calculation.

4. Player (+ subclasses) - Player character with position tracking and class specializations (Soldier, Archer, Mage), each with unique stats and special abilities.

5. Enemy (+ subclasses) - Enemy entities including regular Monsters and stronger Bosses with scaling difficulty.

6. CombatSystem - Turn-based combat manager handling attacks, abilities, defense, and run attempts using d20 dice rolls.

7. Dice - Random number generator for d6 and d20 dice rolls used throughout gameplay.

8. UIButton - Interactive button component for menus and combat actions.

9. GameState - Enum managing game flow between menu, exploration, battle, game over, and victory states.

