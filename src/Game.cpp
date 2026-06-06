#include "Game.h"

#include <algorithm>
#include <curses.h>
#include <random>
#include <string>

namespace {

bool isConfirmKey(int input) {
    return input == '\n' || input == ' ' || input == KEY_ENTER || input == 10 || input == 13;
}

int randInt(int min, int max) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng);
}

}  // namespace

int Game::run() {
    renderer_.initialize();

    if (!renderer_.hasEnoughSpace()) {
        renderer_.drawEndScreen("Terminal Too Small", {
            "This game needs at least an 80x24 terminal.",
            "Resize the terminal and start again."
        }, false);
        renderer_.pollInput();
        renderer_.shutdown();
        return 1;
    }

    while (!quit_) {
        if (!showTitleScreen()) {
            break;
        }

        initialize();
        gameLoop();

        if (quit_) {
            break;
        }
        if (!showEndScreen()) {
            break;
        }
    }

    renderer_.shutdown();
    return 0;
}

bool Game::showTitleScreen() {
    while (true) {
        renderer_.drawTitleScreen();
        const int input = renderer_.pollInput();
        if (input == 'q' || input == 'Q') {
            quit_ = true;
            return false;
        }
        if (isConfirmKey(input)) {
            return true;
        }
    }
}

bool Game::showEndScreen() const {
    if (won_) {
        renderer_.drawEndScreen("Victory", {
            "You defeated the Dread Lord.",
            "The dungeon falls silent behind you."
        }, true);
    } else {
        renderer_.drawEndScreen("Defeat", {
            "Your expedition ends in the dungeon.",
            "The boss remains undefeated."
        }, true);
    }

    while (true) {
        const int input = renderer_.pollInput();
        if (input == 'r' || input == 'R' || isConfirmKey(input)) {
            return true;
        }
        if (input == 'q' || input == 'Q') {
            return false;
        }
    }
}

void Game::initialize() {
    player_ = Player{};
    player_.stats.maxHp = 34;
    player_.stats.hp = player_.stats.maxHp;
    player_.baseAttack = 6;
    player_.baseDefense = 2;
    player_.weaponName = "Rusty Sword";
    player_.armorName = "Worn Coat";
    syncPlayerStats();
    messages_.clear();
    quit_ = false;
    won_ = false;
    addMessage("Enter the dungeon.");
    loadFloor(1);
}

void Game::loadFloor(int floorNumber) {
    player_.floor = floorNumber;
    floor_ = generateFloor(floorNumber, kFinalFloor);
    player_.pos = floor_.start;
    addMessage(floor_.bossFloor ? "The final chamber is near." : "A new floor unfolds.");
}

void Game::gameLoop() {
    while (!quit_ && !won_ && player_.stats.hp > 0) {
        renderer_.drawMap(floor_, player_, messages_);
        const bool turnSpent = handleMapInput(renderer_.pollInput());
        if (turnSpent && !quit_ && !won_ && player_.stats.hp > 0) {
            advanceEnemies();
        }
    }
}

bool Game::handleMapInput(int input) {
    switch (input) {
        case 'q':
        case 'Q':
            quit_ = true;
            return false;
        case 'i':
        case 'I':
            return openInventory();
        case 'w':
        case 'W':
        case KEY_UP:
            return tryMovePlayer(0, -1);
        case 's':
        case 'S':
        case KEY_DOWN:
            return tryMovePlayer(0, 1);
        case 'a':
        case 'A':
        case KEY_LEFT:
            return tryMovePlayer(-1, 0);
        case 'd':
        case 'D':
        case KEY_RIGHT:
            return tryMovePlayer(1, 0);
        default:
            return false;
    }
}

bool Game::tryMovePlayer(int dx, int dy) {
    Position next{player_.pos.x + dx, player_.pos.y + dy};
    if (!floor_.isWalkable(next.x, next.y)) {
        addMessage("A wall blocks the way.");
        return false;
    }

    const int enemyIndex = enemyIndexAt(next);
    if (enemyIndex >= 0) {
        const CombatOutcome outcome = resolveCombat(enemyIndex, false);
        if (outcome != CombatOutcome::EnemyDefeated) {
            return outcome == CombatOutcome::PlayerDied;
        }
    }

    player_.pos = next;
    collectItemAtPlayer();

    if (player_.pos == floor_.exit && !floor_.bossFloor) {
        addMessage("You descend deeper.");
        loadFloor(player_.floor + 1);
        return false;
    }

    if (floor_.bossFloor && floor_.enemies.empty() && player_.pos == floor_.exit) {
        won_ = true;
    }
    return true;
}

bool Game::openInventory() {
    int selected = 0;
    while (true) {
        if (player_.inventory.empty()) {
            selected = 0;
        } else {
            selected = std::clamp(selected, 0, static_cast<int>(player_.inventory.size()) - 1);
        }

        renderer_.drawInventory(InventoryViewData{&player_, selected, &messages_});
        const int input = renderer_.pollInput();
        if (input == 'q' || input == 'Q' || input == 27) {
            return false;
        }
        if (input == 'w' || input == 'W' || input == KEY_UP) {
            selected = std::max(0, selected - 1);
            continue;
        }
        if (input == 's' || input == 'S' || input == KEY_DOWN) {
            if (!player_.inventory.empty()) {
                selected = std::min(static_cast<int>(player_.inventory.size()) - 1, selected + 1);
            }
            continue;
        }
        if (!isConfirmKey(input) || player_.inventory.empty()) {
            continue;
        }

        const ItemType type = player_.inventory[selected].type;
        if (type == ItemType::Potion) {
            if (usePotion(selected)) {
                return true;
            }
        } else {
            equipItem(selected);
            return true;
        }
    }
}

void Game::collectItemAtPlayer() {
    auto it = std::find_if(floor_.items.begin(), floor_.items.end(), [&](const Item& item) {
        return item.pos == player_.pos;
    });
    if (it == floor_.items.end()) {
        return;
    }

    player_.inventory.push_back(*it);
    addMessage("Picked up " + it->name + ".");
    floor_.items.erase(it);
}

int Game::enemyIndexAt(const Position& pos) const {
    for (int i = 0; i < static_cast<int>(floor_.enemies.size()); ++i) {
        if (floor_.enemies[i].pos == pos) {
            return i;
        }
    }
    return -1;
}

bool Game::enemyOccupies(const Position& pos, int skipIndex) const {
    for (int i = 0; i < static_cast<int>(floor_.enemies.size()); ++i) {
        if (i == skipIndex) {
            continue;
        }
        if (floor_.enemies[i].pos == pos) {
            return true;
        }
    }
    return false;
}

Game::CombatOutcome Game::resolveCombat(int enemyIndex, bool enemyInitiated) {
    Enemy& enemy = floor_.enemies[enemyIndex];
    int selected = 0;
    addMessage(enemyInitiated ? enemy.name + " forces a fight." : "Combat begins with " + enemy.name + ".");

    auto enemyAttack = [&]() {
        if (player_.stats.hp <= 0 || enemy.stats.hp <= 0) {
            return;
        }

        switch (enemy.kind) {
            case EnemyKind::Goblin: {
                if (enemy.stats.hp <= enemy.stats.maxHp / 2 && randInt(0, 99) < 35) {
                    addMessage("Goblin darts back and waits.");
                    return;
                }
                const int damage = damageAmount(enemy.stats.attack + 1, player_.stats.defense);
                player_.stats.hp -= damage;
                addMessage("Goblin darts in for " + std::to_string(damage) + ".");
                break;
            }
            case EnemyKind::Skeleton: {
                if (!enemy.revived && enemy.stats.hp <= enemy.stats.maxHp / 3) {
                    enemy.revived = true;
                    enemy.stats.hp = std::min(enemy.stats.maxHp, enemy.stats.hp + 7);
                    addMessage("Skeleton knits its bones back together.");
                    return;
                }
                const int damage = damageAmount(enemy.stats.attack + 1, std::max(0, player_.stats.defense - 1));
                player_.stats.hp -= damage;
                addMessage("Skeleton launches bone shards for " + std::to_string(damage) + ".");
                break;
            }
            case EnemyKind::Orc: {
                if (!enemy.enraged && enemy.stats.hp <= enemy.stats.maxHp / 2) {
                    enemy.enraged = true;
                    enemy.stats.attack += 2;
                    addMessage("Orc flies into a rage.");
                    return;
                }
                const int damage = damageAmount(enemy.stats.attack + 2, player_.stats.defense);
                player_.stats.hp -= damage;
                addMessage("Orc smashes you for " + std::to_string(damage) + ".");
                break;
            }
            case EnemyKind::DreadLord: {
                const int roll = randInt(0, 99);
                if (roll < 35) {
                    const int damage = damageAmount(enemy.stats.attack + 3, std::max(0, player_.stats.defense - 1));
                    player_.stats.hp -= damage;
                    enemy.stats.hp = std::min(enemy.stats.maxHp, enemy.stats.hp + 4);
                    addMessage("Dread Lord drains " + std::to_string(damage) + " HP.");
                } else {
                    const int damage = damageAmount(enemy.stats.attack + 2, player_.stats.defense);
                    player_.stats.hp -= damage;
                    addMessage("Dread Lord cleaves for " + std::to_string(damage) + ".");
                }
                break;
            }
        }
    };

    if (enemyInitiated) {
        enemyAttack();
        if (player_.stats.hp <= 0) {
            return CombatOutcome::PlayerDied;
        }
    }

    while (player_.stats.hp > 0 && enemy.stats.hp > 0) {
        renderer_.drawCombat(CombatViewData{&player_, &enemy, selected, &messages_, !enemy.isBoss});
        const int input = renderer_.pollInput();

        if (input == 'w' || input == 'W' || input == KEY_UP) {
            selected = (selected + 2) % 3;
            continue;
        }
        if (input == 's' || input == 'S' || input == KEY_DOWN) {
            selected = (selected + 1) % 3;
            continue;
        }
        if (!isConfirmKey(input)) {
            continue;
        }

        bool turnSpent = false;
        if (selected == 0) {
            const int damage = damageAmount(player_.stats.attack, enemy.stats.defense);
            enemy.stats.hp -= damage;
            addMessage("You hit " + enemy.name + " for " + std::to_string(damage) + ".");
            turnSpent = true;
        } else if (selected == 1) {
            turnSpent = usePotion();
        } else if (selected == 2) {
            if (enemy.isBoss) {
                addMessage("The boss bars escape.");
            } else {
                addMessage("You escape.");
                return CombatOutcome::Escaped;
            }
        }

        if (!turnSpent) {
            continue;
        }
        if (enemy.stats.hp <= 0) {
            addMessage("You defeated " + enemy.name + ".");
            break;
        }

        enemyAttack();
    }

    if (player_.stats.hp <= 0) {
        return CombatOutcome::PlayerDied;
    }

    floor_.enemies.erase(floor_.enemies.begin() + enemyIndex);
    if (floor_.bossFloor) {
        addMessage("The way out is clear.");
    }
    return CombatOutcome::EnemyDefeated;
}

void Game::advanceEnemies() {
    for (int i = 0; i < static_cast<int>(floor_.enemies.size()) && player_.stats.hp > 0 && !won_; ++i) {
        Enemy& enemy = floor_.enemies[i];
        const int distance = manhattanDistance(enemy.pos, player_.pos);
        if (distance == 1) {
            resolveCombat(i, true);
            return;
        }

        Position next = enemy.pos;
        switch (enemy.kind) {
            case EnemyKind::Goblin:
                if (distance <= 6) {
                    if (enemy.stats.hp <= enemy.stats.maxHp / 2 && distance <= 2) {
                        next.x += (enemy.pos.x < player_.pos.x) ? -1 : (enemy.pos.x > player_.pos.x ? 1 : 0);
                        next.y += (enemy.pos.y < player_.pos.y) ? -1 : (enemy.pos.y > player_.pos.y ? 1 : 0);
                    } else {
                        next = stepToward(enemy.pos, player_.pos);
                    }
                } else if (randInt(0, 99) < 30) {
                    const Position wander[4] = {{enemy.pos.x + 1, enemy.pos.y}, {enemy.pos.x - 1, enemy.pos.y},
                                                {enemy.pos.x, enemy.pos.y + 1}, {enemy.pos.x, enemy.pos.y - 1}};
                    next = wander[randInt(0, 3)];
                }
                break;
            case EnemyKind::Skeleton:
                if (distance <= 8 && randInt(0, 99) < 70) {
                    next = stepToward(enemy.pos, player_.pos);
                }
                break;
            case EnemyKind::Orc:
                if (distance <= 8) {
                    next = stepToward(enemy.pos, player_.pos);
                }
                break;
            case EnemyKind::DreadLord:
                next = stepToward(enemy.pos, player_.pos);
                break;
        }

        if (next == player_.pos) {
            resolveCombat(i, true);
            return;
        }
        if (!floor_.isWalkable(next.x, next.y) || enemyOccupies(next, i)) {
            continue;
        }
        enemy.pos = next;
    }
}

bool Game::usePotion(int inventoryIndex) {
    if (player_.stats.hp >= player_.stats.maxHp) {
        addMessage("You are already at full health.");
        return false;
    }

    int index = inventoryIndex;
    if (index < 0 || index >= static_cast<int>(player_.inventory.size()) || player_.inventory[index].type != ItemType::Potion) {
        index = -1;
        for (int i = 0; i < static_cast<int>(player_.inventory.size()); ++i) {
            if (player_.inventory[i].type == ItemType::Potion) {
                index = i;
                break;
            }
        }
    }

    if (index < 0) {
        addMessage("No potions left.");
        return false;
    }

    const int amount = player_.inventory[index].value;
    player_.stats.hp = std::min(player_.stats.maxHp, player_.stats.hp + amount);
    player_.inventory.erase(player_.inventory.begin() + index);
    addMessage("You drink a potion and recover " + std::to_string(amount) + " HP.");
    return true;
}

void Game::equipItem(int inventoryIndex) {
    if (inventoryIndex < 0 || inventoryIndex >= static_cast<int>(player_.inventory.size())) {
        return;
    }

    Item item = player_.inventory[inventoryIndex];
    player_.inventory.erase(player_.inventory.begin() + inventoryIndex);

    if (item.type == ItemType::Blade) {
        if (player_.weaponBonus > 0) {
            player_.inventory.push_back(Item{{}, ItemType::Blade, player_.weaponBonus, ')', player_.weaponName,
                                             "+" + std::to_string(player_.weaponBonus) + " ATK"});
        }
        player_.weaponBonus = item.value;
        player_.weaponName = item.name;
        addMessage("Equipped " + item.name + ".");
    } else if (item.type == ItemType::Shield) {
        if (player_.armorBonus > 0) {
            player_.inventory.push_back(Item{{}, ItemType::Shield, player_.armorBonus, ']', player_.armorName,
                                             "+" + std::to_string(player_.armorBonus) + " DEF"});
        }
        player_.armorBonus = item.value;
        player_.armorName = item.name;
        addMessage("Equipped " + item.name + ".");
    }

    syncPlayerStats();
}

int Game::countPotions() const {
    int count = 0;
    for (const auto& item : player_.inventory) {
        if (item.type == ItemType::Potion) {
            ++count;
        }
    }
    return count;
}

void Game::syncPlayerStats() {
    player_.stats.attack = player_.baseAttack + player_.weaponBonus;
    player_.stats.defense = player_.baseDefense + player_.armorBonus;
}

Position Game::stepToward(const Position& from, const Position& to) const {
    Position horizontal = from;
    Position vertical = from;
    if (from.x < to.x) {
        ++horizontal.x;
    } else if (from.x > to.x) {
        --horizontal.x;
    }
    if (from.y < to.y) {
        ++vertical.y;
    } else if (from.y > to.y) {
        --vertical.y;
    }

    if (std::abs(to.x - from.x) >= std::abs(to.y - from.y)) {
        if ((horizontal == player_.pos || floor_.isWalkable(horizontal.x, horizontal.y)) && !enemyOccupies(horizontal)) {
            return horizontal;
        }
        if ((vertical == player_.pos || floor_.isWalkable(vertical.x, vertical.y)) && !enemyOccupies(vertical)) {
            return vertical;
        }
    } else {
        if ((vertical == player_.pos || floor_.isWalkable(vertical.x, vertical.y)) && !enemyOccupies(vertical)) {
            return vertical;
        }
        if ((horizontal == player_.pos || floor_.isWalkable(horizontal.x, horizontal.y)) && !enemyOccupies(horizontal)) {
            return horizontal;
        }
    }
    return from;
}

int Game::manhattanDistance(const Position& a, const Position& b) const {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

void Game::addMessage(const std::string& message) {
    messages_.push_back(message);
    if (messages_.size() > 100) {
        messages_.erase(messages_.begin(), messages_.begin() + 20);
    }
}

int Game::damageAmount(int attack, int defense) const {
    return std::max(1, attack - defense / 2);
}
