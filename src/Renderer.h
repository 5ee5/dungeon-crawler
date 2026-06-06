#pragma once

#include "Dungeon.h"

#include <string>
#include <vector>

struct CombatViewData {
    const Player* player = nullptr;
    const Enemy* enemy = nullptr;
    int selectedAction = 0;
    const std::vector<std::string>* log = nullptr;
    bool canFlee = true;
};

struct InventoryViewData {
    const Player* player = nullptr;
    int selectedItem = 0;
    const std::vector<std::string>* messages = nullptr;
};

class Renderer {
public:
    void initialize();
    void shutdown();
    bool hasEnoughSpace() const;
    void drawMap(const DungeonFloor& floor, const Player& player, const std::vector<std::string>& messages) const;
    void drawCombat(const CombatViewData& combat) const;
    void drawInventory(const InventoryViewData& inventory) const;
    void drawTitleScreen() const;
    void drawEndScreen(const std::string& title, const std::vector<std::string>& lines, bool canRestart) const;
    int pollInput() const;
};
