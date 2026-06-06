#pragma once

#include "Dungeon.h"
#include "Renderer.h"

#include <string>
#include <vector>

class Game {
public:
    int run();

private:
    enum class CombatOutcome {
        EnemyDefeated,
        Escaped,
        PlayerDied
    };

    static constexpr int kFinalFloor = 4;

    Renderer renderer_;
    Player player_;
    DungeonFloor floor_;
    std::vector<std::string> messages_;
    bool quit_ = false;
    bool won_ = false;

    bool showTitleScreen();
    bool showEndScreen() const;
    void initialize();
    void loadFloor(int floorNumber);
    void gameLoop();
    bool handleMapInput(int input);
    bool tryMovePlayer(int dx, int dy);
    bool openInventory();
    void collectItemAtPlayer();
    int enemyIndexAt(const Position& pos) const;
    bool enemyOccupies(const Position& pos, int skipIndex = -1) const;
    CombatOutcome resolveCombat(int enemyIndex, bool enemyInitiated);
    void advanceEnemies();
    bool usePotion(int inventoryIndex = -1);
    void equipItem(int inventoryIndex);
    int countPotions() const;
    void syncPlayerStats();
    Position stepToward(const Position& from, const Position& to) const;
    int manhattanDistance(const Position& a, const Position& b) const;
    void addMessage(const std::string& message);
    int damageAmount(int attack, int defense) const;
};
