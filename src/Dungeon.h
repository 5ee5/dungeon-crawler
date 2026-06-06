#pragma once

#include "Entities.h"

#include <vector>

enum class TileType {
    Wall,
    Floor,
    Exit
};

struct Room {
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

struct DungeonFloor {
    int width = 60;
    int height = 20;
    std::vector<TileType> tiles;
    std::vector<Room> rooms;
    std::vector<Enemy> enemies;
    std::vector<Item> items;
    Position start;
    Position exit;
    bool bossFloor = false;

    TileType tileAt(int x, int y) const;
    TileType& tileAt(int x, int y);
    bool isWalkable(int x, int y) const;
};

DungeonFloor generateFloor(int floorNumber, int finalFloor);

