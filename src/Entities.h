#pragma once

#include <string>
#include <vector>

struct Position {
    int x = 0;
    int y = 0;

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
};

struct EntityStats {
    int maxHp = 0;
    int hp = 0;
    int attack = 0;
    int defense = 0;
};

enum class ItemType {
    Potion,
    Blade,
    Shield
};

enum class EnemyKind {
    Goblin,
    Skeleton,
    Orc,
    DreadLord
};

struct Item {
    Position pos;
    ItemType type = ItemType::Potion;
    int value = 0;
    char glyph = '!';
    std::string name;
    std::string description;
};

struct Enemy {
    Position pos;
    EntityStats stats;
    std::string name;
    char glyph = 'g';
    EnemyKind kind = EnemyKind::Goblin;
    bool isBoss = false;
    bool enraged = false;
    bool revived = false;
};

struct Player {
    Position pos;
    EntityStats stats;
    int floor = 1;
    int baseAttack = 0;
    int baseDefense = 0;
    int weaponBonus = 0;
    int armorBonus = 0;
    std::string weaponName = "Rusty Sword";
    std::string armorName = "Worn Coat";
    std::vector<Item> inventory;
};

std::string itemName(ItemType type);
std::string itemDescription(const Item& item);
Item makeRandomItem(const Position& pos, int floor, bool guaranteedPotion);
Enemy makeEnemy(const Position& pos, int floor, bool boss);
