#include "Renderer.h"

#include <algorithm>
#include <curses.h>
#include <functional>

namespace {

constexpr int kMinWidth = 80;
constexpr int kMinHeight = 24;

enum ColorPairId {
    kPairDefault = 1,
    kPairPanel,
    kPairWall,
    kPairFloor,
    kPairExit,
    kPairPlayer,
    kPairEnemy,
    kPairBoss,
    kPairPotion,
    kPairBlade,
    kPairShield,
    kPairHealthGood,
    kPairHealthLow,
    kPairWarning,
    kPairSelected,
    kPairVictory,
    kPairDefeat,
    kPairTitle
};

bool g_hasColors = false;

char tileGlyph(TileType tile) {
    switch (tile) {
        case TileType::Wall:
            return '#';
        case TileType::Floor:
            return '.';
        case TileType::Exit:
            return '>';
    }
    return '?';
}

int colorForTile(TileType tile) {
    if (!g_hasColors) {
        return 0;
    }
    switch (tile) {
        case TileType::Wall:
            return kPairWall;
        case TileType::Floor:
            return kPairFloor;
        case TileType::Exit:
            return kPairExit;
    }
    return kPairDefault;
}

int colorForItem(const Item& item) {
    if (!g_hasColors) {
        return 0;
    }
    switch (item.type) {
        case ItemType::Potion:
            return kPairPotion;
        case ItemType::Blade:
            return kPairBlade;
        case ItemType::Shield:
            return kPairShield;
    }
    return kPairDefault;
}

int colorForEnemy(const Enemy& enemy) {
    if (!g_hasColors) {
        return 0;
    }
    if (enemy.isBoss) {
        return kPairBoss;
    }
    switch (enemy.kind) {
        case EnemyKind::Goblin:
            return kPairEnemy;
        case EnemyKind::Skeleton:
            return kPairPanel;
        case EnemyKind::Orc:
            return kPairWarning;
        case EnemyKind::DreadLord:
            return kPairBoss;
    }
    return kPairEnemy;
}

void withColor(int colorPair, int attributes, const std::function<void()>& drawFn) {
    if (g_hasColors && colorPair > 0) {
        attron(COLOR_PAIR(colorPair));
    }
    if (attributes != 0) {
        attron(attributes);
    }
    drawFn();
    if (attributes != 0) {
        attroff(attributes);
    }
    if (g_hasColors && colorPair > 0) {
        attroff(COLOR_PAIR(colorPair));
    }
}

void drawBoxedSection(int y, int x, int h, int w, const char* title) {
    if (g_hasColors) {
        attron(COLOR_PAIR(kPairPanel));
    }
    mvaddch(y, x, '+');
    mvaddch(y, x + w - 1, '+');
    mvaddch(y + h - 1, x, '+');
    mvaddch(y + h - 1, x + w - 1, '+');
    mvhline(y, x + 1, '-', w - 2);
    mvhline(y + h - 1, x + 1, '-', w - 2);
    mvvline(y + 1, x, '|', h - 2);
    mvvline(y + 1, x + w - 1, '|', h - 2);
    if (title != nullptr) {
        attron(A_BOLD);
        mvprintw(y, x + 2, "[%s]", title);
        attroff(A_BOLD);
    }
    if (g_hasColors) {
        attroff(COLOR_PAIR(kPairPanel));
    }
}

void printClamped(int y, int x, int width, const std::string& text) {
    mvprintw(y, x, "%.*s", width, text.c_str());
}

void initializeColors() {
    g_hasColors = has_colors();
    if (!g_hasColors) {
        return;
    }

    start_color();
    use_default_colors();
    init_pair(kPairDefault, COLOR_WHITE, -1);
    init_pair(kPairPanel, COLOR_CYAN, -1);
    init_pair(kPairWall, COLOR_BLUE, -1);
    init_pair(kPairFloor, COLOR_WHITE, -1);
    init_pair(kPairExit, COLOR_YELLOW, -1);
    init_pair(kPairPlayer, COLOR_WHITE, -1);
    init_pair(kPairEnemy, COLOR_RED, -1);
    init_pair(kPairBoss, COLOR_MAGENTA, -1);
    init_pair(kPairPotion, COLOR_GREEN, -1);
    init_pair(kPairBlade, COLOR_YELLOW, -1);
    init_pair(kPairShield, COLOR_CYAN, -1);
    init_pair(kPairHealthGood, COLOR_GREEN, -1);
    init_pair(kPairHealthLow, COLOR_RED, -1);
    init_pair(kPairWarning, COLOR_YELLOW, -1);
    init_pair(kPairSelected, COLOR_BLACK, COLOR_WHITE);
    init_pair(kPairVictory, COLOR_GREEN, -1);
    init_pair(kPairDefeat, COLOR_RED, -1);
    init_pair(kPairTitle, COLOR_MAGENTA, -1);
}

int healthColor(const EntityStats& stats) {
    if (!g_hasColors) {
        return 0;
    }
    return stats.hp * 100 / std::max(1, stats.maxHp) <= 35 ? kPairHealthLow : kPairHealthGood;
}

}  // namespace

void Renderer::initialize() {
    initscr();
    noecho();
    cbreak();
    keypad(stdscr, true);
    curs_set(0);
    initializeColors();
}

void Renderer::shutdown() {
    endwin();
}

bool Renderer::hasEnoughSpace() const {
    int rows = 0;
    int cols = 0;
    getmaxyx(stdscr, rows, cols);
    return cols >= kMinWidth && rows >= kMinHeight;
}

void Renderer::drawMap(const DungeonFloor& floor, const Player& player, const std::vector<std::string>& messages) const {
    clear();

    drawBoxedSection(0, 0, 22, 62, "Dungeon");
    for (int y = 0; y < floor.height; ++y) {
        for (int x = 0; x < floor.width; ++x) {
            withColor(colorForTile(floor.tileAt(x, y)), floor.tileAt(x, y) == TileType::Wall ? A_DIM : 0, [&]() {
                mvaddch(y + 1, x + 1, tileGlyph(floor.tileAt(x, y)));
            });
        }
    }

    for (const auto& item : floor.items) {
        withColor(colorForItem(item), A_BOLD, [&]() {
            mvaddch(item.pos.y + 1, item.pos.x + 1, item.glyph);
        });
    }
    for (const auto& enemy : floor.enemies) {
        withColor(colorForEnemy(enemy), A_BOLD, [&]() {
            mvaddch(enemy.pos.y + 1, enemy.pos.x + 1, enemy.glyph);
        });
    }
    withColor(g_hasColors ? kPairPlayer : 0, A_BOLD, [&]() {
        mvaddch(player.pos.y + 1, player.pos.x + 1, '@');
    });

    drawBoxedSection(0, 62, 10, 18, "Status");
    withColor(g_hasColors ? kPairDefault : 0, A_BOLD, [&]() {
        mvprintw(1, 64, "Floor: %d", player.floor);
    });
    withColor(healthColor(player.stats), A_BOLD, [&]() {
        mvprintw(2, 64, "HP: %d/%d", player.stats.hp, player.stats.maxHp);
    });
    mvprintw(3, 64, "ATK: %d", player.stats.attack);
    mvprintw(4, 64, "DEF: %d", player.stats.defense);
    withColor(g_hasColors ? kPairPotion : 0, 0, [&]() {
        int potions = 0;
        for (const auto& item : player.inventory) {
            if (item.type == ItemType::Potion) {
                ++potions;
            }
        }
        mvprintw(5, 64, "Potions: %d", potions);
    });
    mvprintw(6, 64, "Inv: %zu", player.inventory.size());
    withColor(g_hasColors ? kPairPanel : 0, A_BOLD, [&]() {
        mvprintw(7, 64, "Goal:");
    });
    withColor(player.floor < 4 ? (g_hasColors ? kPairExit : 0) : (g_hasColors ? kPairBoss : 0), A_BOLD, [&]() {
        mvprintw(8, 64, player.floor < 4 ? "Find the exit" : "Defeat boss");
    });

    drawBoxedSection(10, 62, 12, 18, "Log");
    const int logLines = 9;
    const int start = std::max(0, static_cast<int>(messages.size()) - logLines);
    for (int i = 0; i < logLines && start + i < static_cast<int>(messages.size()); ++i) {
        const std::string& line = messages[start + i];
        int color = 0;
        if (g_hasColors) {
            if (line.find("boss") != std::string::npos || line.find("Dread Lord") != std::string::npos) {
                color = kPairBoss;
            } else if (line.find("potion") != std::string::npos || line.find("Potion") != std::string::npos) {
                color = kPairPotion;
            } else if (line.find("wall") != std::string::npos || line.find("blocked") != std::string::npos) {
                color = kPairWarning;
            } else if (line.find("defeated") != std::string::npos || line.find("escape") != std::string::npos) {
                color = kPairExit;
            }
        }
        withColor(color, 0, [&]() {
            printClamped(11 + i, 64, 14, line);
        });
    }

    withColor(g_hasColors ? kPairPanel : 0, A_BOLD, [&]() {
        mvprintw(22, 0, "Move: WASD/Arrows   i: Inventory   q: Quit");
    });
    refresh();
}

void Renderer::drawCombat(const CombatViewData& combat) const {
    clear();

    drawBoxedSection(1, 10, 20, 60, "Combat");
    withColor(healthColor(combat.player->stats), A_BOLD, [&]() {
        mvprintw(3, 14, "Player HP: %d/%d   ATK: %d   DEF: %d",
                 combat.player->stats.hp, combat.player->stats.maxHp,
                 combat.player->stats.attack, combat.player->stats.defense);
    });
    withColor(colorForEnemy(*combat.enemy), A_BOLD, [&]() {
        mvprintw(5, 14, "%s HP: %d/%d   ATK: %d   DEF: %d",
                 combat.enemy->name.c_str(), combat.enemy->stats.hp, combat.enemy->stats.maxHp,
                 combat.enemy->stats.attack, combat.enemy->stats.defense);
    });

    const char* actions[3] = {"Attack", "Use Potion", combat.canFlee ? "Flee" : "Flee (blocked)"};
    for (int i = 0; i < 3; ++i) {
        int color = 0;
        int attrs = 0;
        if (i == combat.selectedAction) {
            color = g_hasColors ? kPairSelected : 0;
            attrs = A_BOLD;
        } else if (i == 1) {
            color = g_hasColors ? kPairPotion : 0;
        } else if (i == 2 && !combat.canFlee) {
            color = g_hasColors ? kPairWarning : 0;
        }
        withColor(color, attrs, [&]() {
            mvprintw(8 + i, 14, "%s", actions[i]);
        });
    }

    withColor(g_hasColors ? kPairPotion : 0, 0, [&]() {
        int potions = 0;
        for (const auto& item : combat.player->inventory) {
            if (item.type == ItemType::Potion) {
                ++potions;
            }
        }
        mvprintw(12, 14, "Potions: %d", potions);
    });
    withColor(g_hasColors ? kPairPanel : 0, 0, [&]() {
        mvprintw(13, 14, "Enter/Space: confirm");
    });

    const int logStart = std::max(0, static_cast<int>(combat.log->size()) - 5);
    for (int i = 0; i < 5 && logStart + i < static_cast<int>(combat.log->size()); ++i) {
        const std::string& line = (*combat.log)[logStart + i];
        int color = 0;
        if (g_hasColors) {
            if (line.find("hits you") != std::string::npos || line.find("bars escape") != std::string::npos) {
                color = kPairWarning;
            } else if (line.find("defeated") != std::string::npos) {
                color = kPairExit;
            } else if (line.find("potion") != std::string::npos || line.find("Potion") != std::string::npos) {
                color = kPairPotion;
            }
        }
        withColor(color, 0, [&]() {
            printClamped(15 + i, 14, 52, line);
        });
    }

    refresh();
}

void Renderer::drawInventory(const InventoryViewData& inventory) const {
    clear();
    drawBoxedSection(1, 6, 22, 68, "Inventory");
    mvprintw(3, 10, "Weapon: %s (+%d)", inventory.player->weaponName.c_str(), inventory.player->weaponBonus);
    mvprintw(4, 10, "Armor:  %s (+%d)", inventory.player->armorName.c_str(), inventory.player->armorBonus);
    mvprintw(5, 10, "ATK: %d   DEF: %d   HP: %d/%d",
             inventory.player->stats.attack, inventory.player->stats.defense,
             inventory.player->stats.hp, inventory.player->stats.maxHp);

    drawBoxedSection(7, 8, 11, 42, "Items");
    if (inventory.player->inventory.empty()) {
        withColor(g_hasColors ? kPairWarning : 0, 0, [&]() {
            mvprintw(10, 12, "Inventory is empty.");
        });
    } else {
        const int windowSize = 8;
        const int start = std::max(0, inventory.selectedItem - windowSize + 1);
        for (int row = 0; row < windowSize && start + row < static_cast<int>(inventory.player->inventory.size()); ++row) {
            const Item& item = inventory.player->inventory[start + row];
            const int y = 8 + row;
            const bool selected = (start + row == inventory.selectedItem);
            withColor(selected ? (g_hasColors ? kPairSelected : 0) : colorForItem(item), selected ? A_BOLD : 0, [&]() {
                mvprintw(y, 10, "%c %s", item.glyph, item.name.c_str());
            });
            printClamped(y, 24, 24, item.description);
        }
    }

    drawBoxedSection(7, 51, 11, 21, "Controls");
    mvprintw(9, 54, "W/S: Select");
    mvprintw(10, 54, "Enter: Use");
    mvprintw(11, 54, "q: Close");
    mvprintw(13, 54, "Potions heal.");
    mvprintw(14, 54, "Blades equip.");
    mvprintw(15, 54, "Shields equip.");

    drawBoxedSection(18, 8, 4, 64, "Log");
    const int start = std::max(0, static_cast<int>(inventory.messages->size()) - 2);
    for (int i = 0; i < 2 && start + i < static_cast<int>(inventory.messages->size()); ++i) {
        printClamped(19 + i, 10, 60, (*inventory.messages)[start + i]);
    }
    refresh();
}

void Renderer::drawTitleScreen() const {
    clear();
    drawBoxedSection(4, 12, 16, 56, "Dungeon Crawler");
    withColor(g_hasColors ? kPairTitle : 0, A_BOLD, [&]() {
        mvprintw(7, 20, "DUNGEON CRAWLER");
    });
    mvprintw(9, 20, "Fight through random rooms and beat the final boss.");
    mvprintw(11, 20, "Enemy types now behave differently.");
    mvprintw(12, 20, "Inventory lets you equip blades and shields.");
    mvprintw(13, 20, "You can restart runs from the ending screen.");
    withColor(g_hasColors ? kPairPanel : 0, A_BOLD, [&]() {
        mvprintw(16, 20, "Press Enter to begin   q to quit");
    });
    refresh();
}

void Renderer::drawEndScreen(const std::string& title, const std::vector<std::string>& lines, bool canRestart) const {
    clear();
    drawBoxedSection(5, 12, 14, 56, title.c_str());
    const int titleColor = title == "Victory" ? (g_hasColors ? kPairVictory : 0)
                         : title == "Defeat" ? (g_hasColors ? kPairDefeat : 0)
                         : (g_hasColors ? kPairWarning : 0);
    withColor(titleColor, A_BOLD, [&]() {
        mvprintw(6, 16, "%s", title.c_str());
    });
    for (int i = 0; i < static_cast<int>(lines.size()); ++i) {
        withColor(titleColor, 0, [&]() {
            printClamped(8 + i, 16, 48, lines[i]);
        });
    }
    withColor(g_hasColors ? kPairPanel : 0, 0, [&]() {
        mvprintw(16, 16, canRestart ? "r: restart   q: quit" : "Press any key to exit.");
    });
    refresh();
}

int Renderer::pollInput() const {
    return getch();
}
