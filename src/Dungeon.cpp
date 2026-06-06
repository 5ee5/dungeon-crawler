#include "Dungeon.h"

#include <algorithm>
#include <random>

namespace {

std::mt19937& rng() {
    static std::mt19937 engine(std::random_device{}());
    return engine;
}

int randInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng());
}

Position roomCenter(const Room& room) {
    return {room.x + room.w / 2, room.y + room.h / 2};
}

bool overlaps(const Room& a, const Room& b) {
    return a.x <= b.x + b.w + 1 &&
           a.x + a.w + 1 >= b.x &&
           a.y <= b.y + b.h + 1 &&
           a.y + a.h + 1 >= b.y;
}

void carveRoom(DungeonFloor& floor, const Room& room) {
    for (int y = room.y; y < room.y + room.h; ++y) {
        for (int x = room.x; x < room.x + room.w; ++x) {
            floor.tileAt(x, y) = TileType::Floor;
        }
    }
}

void carveHCorridor(DungeonFloor& floor, int x1, int x2, int y) {
    if (x1 > x2) {
        std::swap(x1, x2);
    }
    for (int x = x1; x <= x2; ++x) {
        floor.tileAt(x, y) = TileType::Floor;
    }
}

void carveVCorridor(DungeonFloor& floor, int y1, int y2, int x) {
    if (y1 > y2) {
        std::swap(y1, y2);
    }
    for (int y = y1; y <= y2; ++y) {
        floor.tileAt(x, y) = TileType::Floor;
    }
}

bool occupied(const Position& pos, const std::vector<Enemy>& enemies, const std::vector<Item>& items, const Position& start, const Position& exit) {
    if (pos == start || pos == exit) {
        return true;
    }
    for (const auto& enemy : enemies) {
        if (enemy.pos == pos) {
            return true;
        }
    }
    for (const auto& item : items) {
        if (item.pos == pos) {
            return true;
        }
    }
    return false;
}

Position randomFloorPosition(const DungeonFloor& floor) {
    while (true) {
        const Room& room = floor.rooms[randInt(0, static_cast<int>(floor.rooms.size()) - 1)];
        Position pos{randInt(room.x + 1, room.x + room.w - 2), randInt(room.y + 1, room.y + room.h - 2)};
        if (floor.isWalkable(pos.x, pos.y)) {
            return pos;
        }
    }
}

}  // namespace

TileType DungeonFloor::tileAt(int x, int y) const {
    return tiles[y * width + x];
}

TileType& DungeonFloor::tileAt(int x, int y) {
    return tiles[y * width + x];
}

bool DungeonFloor::isWalkable(int x, int y) const {
    if (x < 0 || y < 0 || x >= width || y >= height) {
        return false;
    }
    const TileType tile = tileAt(x, y);
    return tile == TileType::Floor || tile == TileType::Exit;
}

DungeonFloor generateFloor(int floorNumber, int finalFloor) {
    DungeonFloor floor;
    floor.width = 60;
    floor.height = 20;
    floor.tiles.assign(floor.width * floor.height, TileType::Wall);
    floor.bossFloor = (floorNumber == finalFloor);

    const int roomAttempts = 30;
    const int targetRooms = 8 + floorNumber;

    for (int i = 0; i < roomAttempts && static_cast<int>(floor.rooms.size()) < targetRooms; ++i) {
        Room room;
        room.w = randInt(6, 12);
        room.h = randInt(4, 7);
        room.x = randInt(1, floor.width - room.w - 2);
        room.y = randInt(1, floor.height - room.h - 2);

        bool valid = true;
        for (const auto& existing : floor.rooms) {
            if (overlaps(room, existing)) {
                valid = false;
                break;
            }
        }
        if (!valid) {
            continue;
        }

        carveRoom(floor, room);
        if (!floor.rooms.empty()) {
            Position prev = roomCenter(floor.rooms.back());
            Position curr = roomCenter(room);
            if (randInt(0, 1) == 0) {
                carveHCorridor(floor, prev.x, curr.x, prev.y);
                carveVCorridor(floor, prev.y, curr.y, curr.x);
            } else {
                carveVCorridor(floor, prev.y, curr.y, prev.x);
                carveHCorridor(floor, prev.x, curr.x, curr.y);
            }
        }
        floor.rooms.push_back(room);
    }

    if (floor.rooms.empty()) {
        Room fallback{2, 2, 10, 8};
        carveRoom(floor, fallback);
        floor.rooms.push_back(fallback);
    }

    floor.start = roomCenter(floor.rooms.front());
    floor.exit = roomCenter(floor.rooms.back());
    floor.tileAt(floor.exit.x, floor.exit.y) = TileType::Exit;

    const int enemyCount = floor.bossFloor ? 0 : 4 + floorNumber * 2;
    for (int i = 0; i < enemyCount; ++i) {
        Position pos = randomFloorPosition(floor);
        while (occupied(pos, floor.enemies, floor.items, floor.start, floor.exit)) {
            pos = randomFloorPosition(floor);
        }
        floor.enemies.push_back(makeEnemy(pos, floorNumber, false));
    }

    const int itemCount = 3 + floorNumber;
    for (int i = 0; i < itemCount; ++i) {
        Position pos = randomFloorPosition(floor);
        while (occupied(pos, floor.enemies, floor.items, floor.start, floor.exit)) {
            pos = randomFloorPosition(floor);
        }
        floor.items.push_back(makeRandomItem(pos, floorNumber, i == 0));
    }

    if (floor.bossFloor) {
        floor.enemies.push_back(makeEnemy(floor.exit, floorNumber + 1, true));
    }

    return floor;
}

