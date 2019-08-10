#ifndef GAMEBOARD_H
#define GAMEBOARD_H

#include "tile.hpp"
#include <tuple>
#include <vector>

namespace Game {

struct GameBoard {
  using tile_data_array_t = std::vector<tile_t>;
  using gameboard_data_array_t = std::tuple<size_t, tile_data_array_t>;

  gameboard_data_array_t gbda;
  bool win{};
  bool moved{true};
  ull score{};
  ull largestTile{2};
  long long moveCount{-1};

  GameBoard() = default;
  explicit GameBoard(ull playsize);
  explicit GameBoard(ull playsize, tile_data_array_t prempt_board);
};

using load_gameboard_status_t = std::tuple<bool, GameBoard>;

bool hasWonOnGameboard(GameBoard gb);
long long MoveCountOnGameBoard(GameBoard gb);

void unblockTilesOnGameboard(GameBoard &gb);
bool canMoveOnGameboard(GameBoard &gb);
bool addTileOnGameboard(GameBoard &gb);
void registerMoveByOneOnGameboard(GameBoard &gb);

void tumbleTilesUpOnGameboard(GameBoard &gb);
void tumbleTilesDownOnGameboard(GameBoard &gb);
void tumbleTilesLeftOnGameboard(GameBoard &gb);
void tumbleTilesRightOnGameboard(GameBoard &gb);

std::string printStateOfGameBoard(GameBoard gb);

} // namespace Game

std::ostream &operator<<(std::ostream &os, const Game::GameBoard &gb);

#endif
