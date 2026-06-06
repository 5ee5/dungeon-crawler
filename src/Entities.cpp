#include "Entities.h"

#include <random>

std::string itemName(ItemType type) {
    switch (type) {
        case ItemType::Potion:
            return "Potion";
        case ItemType::Blade:
            return "Blade";
        case ItemType::Shield:
            return "Shield";
    }
    return "Unknown";
}

std::string itemDescription(const Item& item) {
    switch (item.type) {
        case ItemType::Potion:
            return "Restore " + std::to_string(item.value) + " HP";
        case ItemType::Blade:
            return "+" + std::to_string(item.value) + " ATK";
        case ItemType::Shield:
            return "+" + std::to_string(item.value) + " DEF";
    }
    return "";
}

static std::mt19937& rng() {
    static std::mt19937 engine(std::random_device{}());
    return engine;
}

Item makeRandomItem(const Position& pos, int floor, bool guaranteedPotion) {
    Item item;
    item.pos = pos;

    std::uniform_int_distribution<int> kindDist(0, 2);
    int kind = guaranteedPotion ? 0 : kindDist(rng());
    item.type = static_cast<ItemType>(kind);
    item.name = itemName(item.type);

    switch (item.type) {
        case ItemType::Potion:
            item.value = 10 + floor * 4;
            item.glyph = '!';
            break;
        case ItemType::Blade:
            item.value = 1 + (floor / 2);
            item.glyph = ')';
            break;
        case ItemType::Shield:
            item.value = 1 + (floor / 2);
            item.glyph = ']';
            break;
    }
    item.description = itemDescription(item);
    return item;
}

Enemy makeEnemy(const Position& pos, int floor, bool boss) {
    Enemy enemy;
    enemy.pos = pos;
    enemy.isBoss = boss;

    if (boss) {
        enemy.name = "Dread Lord";
        enemy.glyph = 'B';
        enemy.kind = EnemyKind::DreadLord;
        enemy.stats.maxHp = 45 + floor * 8;
        enemy.stats.hp = enemy.stats.maxHp;
        enemy.stats.attack = 9 + floor * 2;
        enemy.stats.defense = 4 + floor;
        return enemy;
    }

    std::uniform_int_distribution<int> archetypeDist(0, 2);
    switch (archetypeDist(rng())) {
        case 0:
            enemy.name = "Goblin";
            enemy.glyph = 'g';
            enemy.kind = EnemyKind::Goblin;
            enemy.stats.maxHp = 10 + floor * 2;
            enemy.stats.attack = 4 + floor;
            enemy.stats.defense = 1 + floor / 2;
            break;
        case 1:
            enemy.name = "Skeleton";
            enemy.glyph = 's';
            enemy.kind = EnemyKind::Skeleton;
            enemy.stats.maxHp = 12 + floor * 2;
            enemy.stats.attack = 3 + floor;
            enemy.stats.defense = 2 + floor / 2;
            break;
        case 2:
            enemy.name = "Orc";
            enemy.glyph = 'o';
            enemy.kind = EnemyKind::Orc;
            enemy.stats.maxHp = 15 + floor * 3;
            enemy.stats.attack = 5 + floor;
            enemy.stats.defense = 2 + floor / 2;
            break;
    }

    enemy.stats.hp = enemy.stats.maxHp;
    return enemy;
}
