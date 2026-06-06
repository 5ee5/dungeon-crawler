# Dungeon Crawler

`Dungeon Crawler` is a terminal-based roguelike written in C++ with `ncurses`. You explore procedurally generated floors, manage consumables and equipment, fight enemy archetypes with different behavior, and push toward a final boss encounter.

## Features

- Procedurally generated room-and-corridor dungeon floors
- Colored `ncurses` interface for exploration, combat, inventory, and title/end screens
- Turn-based movement and modal combat encounters
- Inventory system with consumable potions and equipable blades/shields
- Distinct enemy behaviors for goblins, skeletons, orcs, and the final boss
- Multi-floor progression with a boss floor and restartable runs

## Requirements

- C++17-compatible compiler
- CMake 3.16 or newer
- `ncurses`
- Terminal with at least `80x24` available space

## Installing ncurses

Common package names:

- Ubuntu/Debian: `sudo apt install libncurses-dev`
- Fedora: `sudo dnf install ncurses-devel`
- Arch Linux: `sudo pacman -S ncurses`
- macOS with Homebrew: `brew install ncurses`

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Run

```bash
./build/dungeon_crawler
```

## Controls

- `WASD` or arrow keys: move / navigate menus
- `i`: open inventory
- `Enter` or `Space`: confirm action
- `q`: quit the current screen or game
- `r`: restart from the ending screen

## How to Play

Move through each floor, collect items, and survive enemy encounters until you reach the final boss.

- Touching an enemy starts combat.
- Potions restore health when used from combat or the inventory screen.
- Blades improve attack when equipped.
- Shields improve defense when equipped.
- Normal floors end at the exit tile `>`.
- The last floor contains the boss. Defeat it and reach the cleared exit to win.

## Enemy Types

- `Goblin`: fast, opportunistic, and may pull back when hurt
- `Skeleton`: steady pressure with a recovery phase when weakened
- `Orc`: aggressive melee fighter that becomes more dangerous mid-fight
- `Dread Lord`: boss enemy with harder hits and self-healing pressure

## Project Layout

- `src/`: game code split into dungeon generation, entities, game loop, renderer, and entrypoint
- `CMakeLists.txt`: build configuration
- `build/`: local generated build output, not intended for version control

## Known Limitations

- No save/load system
- Balance is still early and may need tuning
- Intended for local terminal play, not networked or packaged distribution
- Requires an `80x24` terminal; smaller terminals will not render properly

## License

This project is released under the MIT License. See [LICENSE](LICENSE).

