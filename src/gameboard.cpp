#include "gameboard.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <random>
#include <sstream>

namespace Game {

namespace {

class RandInt {
public:
  using clock = std::chrono::system_clock;
  RandInt() : dist{0, std::numeric_limits<int>::max()} {
    seed(clock::now().time_since_epoch().count());
  }
  RandInt(const int low, const int high) : dist{low, high} {
    seed(clock::now().time_since_epoch().count());
  }
  int operator()() { return dist(re); }
  void seed(const unsigned int s) { re.seed(s); }

private:
  std::minstd_rand re;
  std::uniform_int_distribution<> dist;
};

int getPlaySizeOfGameboard(gameboard_data_array_t gbda) {
  return gbda.playsize;
}

int point2D_to_1D_index(gameboard_data_array_t gbda, point2D_t pt) {
  int x, y;
  std::tie(x, y) = pt.get();
  return x + getPlaySizeOfGameboard(gbda) * y;
}

Tile getTileOnGameboard(gameboard_data_array_t gbda, point2D_t pt) {
  return gbda.board[point2D_to_1D_index(gbda, pt)];
}

void setTileOnGameboard(gameboard_data_array_t &gbda, point2D_t pt, Tile tile) {
  gbda.board[point2D_to_1D_index(gbda, pt)] = tile;
}

ull getTileValueOnGameboard(gameboard_data_array_t gbda, point2D_t pt) {
  return gbda.board[point2D_to_1D_index(gbda, pt)].value;
}

void setTileValueOnGameboard(gameboard_data_array_t &gbda, point2D_t pt,
                             ull value) {
  gbda.board[point2D_to_1D_index(gbda, pt)].value = value;
}

bool getTileBlockedOnGameboard(gameboard_data_array_t gbda, point2D_t pt) {
  return gbda.board[point2D_to_1D_index(gbda, pt)].blocked;
}

template<int num_of_bars>
std::array<std::string, num_of_bars> make_patterned_bars(int playsize) {
  auto temp_bars = std::array<std::string, num_of_bars>{};
  using bar_pattern_t = std::tuple<std::string, std::string, std::string>;

  const auto bar_pattern_list = {std::make_tuple("┌", "┬", "┐"),
                                 std::make_tuple("├", "┼", "┤"),
                                 std::make_tuple("└", "┴", "┘")};

  // generate types of horizontal bars...
  const auto generate_x_bar_pattern = [playsize](const bar_pattern_t t) {
    enum { PATTERN_HEAD, PATTERN_MID, PATTERN_TAIL };
    constexpr auto sp = "  ";
    constexpr auto separator = "──────";
    std::ostringstream temp_richtext;
    temp_richtext << sp << std::get<PATTERN_HEAD>(t);
    for (int i = 0; i < playsize; i++) {
      const auto is_not_last_column = (i < playsize - 1);
      temp_richtext << separator
                    << (is_not_last_column ? std::get<PATTERN_MID>(t) :
                                             std::get<PATTERN_TAIL>(t));
    }
    temp_richtext << "\n";
    return temp_richtext.str();
  };
  std::transform(std::begin(bar_pattern_list), std::end(bar_pattern_list),
                 std::begin(temp_bars), generate_x_bar_pattern);
  return temp_bars;
}

std::string drawSelf(gameboard_data_array_t gbda) {
  enum { TOP_BAR, XN_BAR, BASE_BAR, MAX_TYPES_OF_BARS };
  const auto vertibar =
      make_patterned_bars<MAX_TYPES_OF_BARS>(getPlaySizeOfGameboard(gbda));
  std::ostringstream str_os;
  for (int y = 0; y < getPlaySizeOfGameboard(gbda); y++) {
    const auto is_first_row = (y == 0);
    str_os << (is_first_row ? std::get<TOP_BAR>(vertibar) :
                              std::get<XN_BAR>(vertibar));
    for (int x = 0; x < getPlaySizeOfGameboard(gbda); x++) {
      const auto is_first_col = (x == 0);
      const auto sp = (is_first_col ? "  " : " ");
      str_os << sp;
      str_os << "│ ";
      str_os << drawTileString(getTileOnGameboard(gbda, point2D_t{x, y}));
    }
    str_os << " │";
    str_os << "\n";
  }
  str_os << std::get<BASE_BAR>(vertibar);
  str_os << "\n";
  return str_os.str();
}

std::string printStateOfGameBoardDataArray(gameboard_data_array_t gbda) {
  std::ostringstream os;
  for (int y = 0; y < getPlaySizeOfGameboard(gbda); y++) {
    for (int x = 0; x < getPlaySizeOfGameboard(gbda); x++) {
      const auto current_point = point2D_t{x, y};
      os << getTileValueOnGameboard(gbda, current_point) << ":"
         << getTileBlockedOnGameboard(gbda, current_point) << ",";
    }
    os << "\n";
  }
  return os.str();
}

bool is_point_in_board_play_area(point2D_t pt, int playsize) {
  int x, y;
  std::tie(x, y) = pt.get();
  return !(y < 0 || y > playsize - 1 || x < 0 || x > playsize - 1);
}

bool check_recursive_offset_in_game_bounds(point2D_t pt, point2D_t pt_offset,
                                           int playsize) {
  int x, y, x2, y2;
  std::tie(x, y) = pt.get();
  std::tie(x2, y2) = pt_offset.get();
  const auto positive_direction = (y2 + x2 == 1);
  const auto negative_direction = (y2 + x2 == -1);
  const auto is_positive_y_direction_flagged = (y2 == 1);
  const auto is_negative_y_direction_flagged = (y2 == -1);
  const auto is_inside_outer_bounds =
      (positive_direction &&
       (is_positive_y_direction_flagged ? y : x) < playsize - 2);
  const auto is_inside_inner_bounds =
      (negative_direction && (is_negative_y_direction_flagged ? y : x) > 1);
  return (is_inside_outer_bounds || is_inside_inner_bounds);
}

void unblockTilesOnGameboardDataArray(gameboard_data_array_t &gbda) {
  std::transform(std::begin(gbda.board), std::end(gbda.board),
                 std::begin(gbda.board), [](const Tile t) {
                   return Tile{t.value, false};
                 });
}

bool canMoveOnGameboardDataArray(gameboard_data_array_t &gbda) {
  auto index_counter{0};

  const auto can_move_to_offset = [=, &index_counter](const Tile t) {
    const auto current_point =
        point2D_t{index_counter % getPlaySizeOfGameboard(gbda),
                  index_counter / getPlaySizeOfGameboard(gbda)};
    index_counter++;
    const auto list_of_offsets = {point2D_t{1, 0}, point2D_t{0, 1}};
    const auto current_point_value = t.value;

    const auto offset_in_range_with_same_value = [=](const point2D_t offset) {
      const auto offset_check = {
          current_point + offset, // Positive adjacent check
          current_point - offset}; // Negative adjacent Check
      for (const auto current_offset : offset_check) {
        if (is_point_in_board_play_area(current_offset,
                                        getPlaySizeOfGameboard(gbda))) {
          return getTileValueOnGameboard(gbda, current_offset) ==
                 current_point_value;
        }
      }
      return false;
    };

    return ((current_point_value == 0u) ||
            std::any_of(std::begin(list_of_offsets), std::end(list_of_offsets),
                        offset_in_range_with_same_value));
  };
  return std::any_of(std::begin(gbda.board), std::end(gbda.board),
                     can_move_to_offset);
}

std::vector<point2D_t>
collectFreeTilesOnGameboard(gameboard_data_array_t gbda) {
  std::vector<point2D_t> freeTiles;
  auto index_counter{0};
  const auto gatherFreePoint = [gbda, &freeTiles,
                                &index_counter](const Tile t) {
    const auto current_point =
        point2D_t{index_counter % getPlaySizeOfGameboard(gbda),
                  index_counter / getPlaySizeOfGameboard(gbda)};
    if (!t.value) {
      freeTiles.push_back(current_point);
    }
    index_counter++;
  };
  std::for_each(std::begin(gbda.board), std::end(gbda.board), gatherFreePoint);
  return freeTiles;
}

bool addTileOnGameboardDataArray(gameboard_data_array_t &gbda) {
  constexpr auto CHANCE_OF_VALUE_FOUR_OVER_TWO = 89; // Percentage
  const auto freeTiles = collectFreeTilesOnGameboard(gbda);

  if (!freeTiles.size()) {
    return true;
  }

  const auto randomFreeTile = freeTiles.at(RandInt{}() % freeTiles.size());
  const auto value_four_or_two =
      RandInt{}() % 100 > CHANCE_OF_VALUE_FOUR_OVER_TWO ? 4 : 2;
  setTileValueOnGameboard(gbda, randomFreeTile, value_four_or_two);

  return false;
}

bool collaspeTilesOnGameboard(gameboard_data_array_t &gbda, point2D_t pt,
                              point2D_t pt_offset) {
  Tile currentTile = getTileOnGameboard(gbda, pt);
  Tile targetTile = getTileOnGameboard(gbda, pt + pt_offset);

  currentTile.value = 0;
  targetTile.value *= 2;
  targetTile.blocked = true;

  setTileOnGameboard(gbda, pt, currentTile);
  setTileOnGameboard(gbda, pt + pt_offset, targetTile);
  return true;
}

bool shiftTilesOnGameboard(gameboard_data_array_t &gbda, point2D_t pt,
                           point2D_t pt_offset) {
  Tile currentTile = getTileOnGameboard(gbda, pt);
  Tile targetTile = getTileOnGameboard(gbda, pt + pt_offset);

  targetTile.value = currentTile.value;
  currentTile.value = 0;

  setTileOnGameboard(gbda, pt, currentTile);
  setTileOnGameboard(gbda, pt + pt_offset, targetTile);
  return true;
}

enum class COLLASPE_OR_SHIFT_T {
  ACTION_NONE,
  ACTION_COLLASPE,
  ACTION_SHIFT,
  MAX_NUM_OF_ACTIONS
};

using bool_collaspe_shift_t = std::tuple<bool, COLLASPE_OR_SHIFT_T>;

bool_collaspe_shift_t
collasped_or_shifted_tilesOnGameboard(gameboard_data_array_t &gbda,
                                      point2D_t pt, point2D_t pt_offset) {
  const auto currentTile = getTileOnGameboard(gbda, pt);
  const auto targetTile = getTileOnGameboard(gbda, pt + pt_offset);
  const auto does_value_exist_in_target_point = targetTile.value;
  const auto is_value_same_as_target_value =
      (currentTile.value == targetTile.value);
  const auto no_tiles_are_blocked =
      (!currentTile.blocked && !targetTile.blocked);
  const auto is_there_a_current_value_but_no_target_value =
      (currentTile.value && !targetTile.value);
  auto action_taken = bool{};

  if (does_value_exist_in_target_point && is_value_same_as_target_value &&
      no_tiles_are_blocked) {
    action_taken = collaspeTilesOnGameboard(gbda, pt, pt_offset);
    return std::make_tuple(action_taken, COLLASPE_OR_SHIFT_T::ACTION_COLLASPE);
  } else if (is_there_a_current_value_but_no_target_value) {
    action_taken = shiftTilesOnGameboard(gbda, pt, pt_offset);
    return std::make_tuple(action_taken, COLLASPE_OR_SHIFT_T::ACTION_SHIFT);
  }
  return std::make_tuple(action_taken, COLLASPE_OR_SHIFT_T::ACTION_NONE);
}

void discoverLargestTileValueOnGameboard(GameBoard gb, Tile targetTile) {
  gb.largestTile =
      gb.largestTile < targetTile.value ? targetTile.value : gb.largestTile;
}

void discoverWinningTileValueOnGameboard(GameBoard gb, Tile targetTile) {
  if (!hasWonOnGameboard(gb)) {
    constexpr auto GAME_TILE_WINNING_SCORE = 2048;
    if (targetTile.value == GAME_TILE_WINNING_SCORE) {
      gb.win = true;
    }
  }
}

bool updateGameBoardStats(GameBoard &gb, Tile targetTile) {
  gb.score += targetTile.value;
  discoverLargestTileValueOnGameboard(gb, targetTile);
  discoverWinningTileValueOnGameboard(gb, targetTile);
  return true;
}

void moveOnGameboard(GameBoard &gb, point2D_t pt, point2D_t pt_offset) {
  auto did_gameboard_collaspe_or_shift_anything = bool{};
  auto action_was_taken = COLLASPE_OR_SHIFT_T::ACTION_NONE;
  std::tie(did_gameboard_collaspe_or_shift_anything, action_was_taken) =
      collasped_or_shifted_tilesOnGameboard(gb.gbda, pt, pt_offset);
  if (did_gameboard_collaspe_or_shift_anything) {
    gb.moved = true;
    if (action_was_taken == COLLASPE_OR_SHIFT_T::ACTION_COLLASPE) {
      updateGameBoardStats(gb, getTileOnGameboard(gb.gbda, pt + pt_offset));
    }
  }
  if (check_recursive_offset_in_game_bounds(pt, pt_offset,
                                            getPlaySizeOfGameboard(gb.gbda))) {
    moveOnGameboard(gb, pt + pt_offset, pt_offset);
  }
}

} // namespace

bool hasWonOnGameboard(GameBoard gb) {
  return gb.win;
}

long long MoveCountOnGameBoard(GameBoard gb) {
  return gb.moveCount;
}

void unblockTilesOnGameboard(GameBoard &gb) {
  unblockTilesOnGameboardDataArray(gb.gbda);
}

bool canMoveOnGameboard(GameBoard &gb) {
  return canMoveOnGameboardDataArray(gb.gbda);
}

void registerMoveByOneOnGameboard(GameBoard &gb) {
  gb.moveCount++;
  gb.moved = false;
}

bool addTileOnGameboard(GameBoard &gb) {
  return addTileOnGameboardDataArray(gb.gbda);
}

void tumbleTilesUpOnGameboard(GameBoard &gb) {
  for (int x = 0; x < getPlaySizeOfGameboard(gb.gbda); x++) {
    int y = 1;
    while (y < getPlaySizeOfGameboard(gb.gbda)) {
      const auto current_point = point2D_t{x, y};
      if (getTileValueOnGameboard(gb.gbda, current_point)) {
        moveOnGameboard(gb, current_point, point2D_t{0, -1});
      }
      y++;
    }
  }
}

void tumbleTilesDownOnGameboard(GameBoard &gb) {
  for (int x = 0; x < getPlaySizeOfGameboard(gb.gbda); x++) {
    int y = getPlaySizeOfGameboard(gb.gbda) - 2;
    while (y >= 0) {
      const auto current_point = point2D_t{x, y};
      if (getTileValueOnGameboard(gb.gbda, current_point)) {
        moveOnGameboard(gb, current_point, point2D_t{0, 1});
      }
      y--;
    }
  }
}

void tumbleTilesLeftOnGameboard(GameBoard &gb) {
  for (int y = 0; y < getPlaySizeOfGameboard(gb.gbda); y++) {
    int x = 1;
    while (x < getPlaySizeOfGameboard(gb.gbda)) {
      const auto current_point = point2D_t{x, y};
      if (getTileValueOnGameboard(gb.gbda, current_point)) {
        moveOnGameboard(gb, current_point, {-1, 0});
      }
      x++;
    }
  }
}

void tumbleTilesRightOnGameboard(GameBoard &gb) {
  for (int y = 0; y < getPlaySizeOfGameboard(gb.gbda); y++) {
    int x = getPlaySizeOfGameboard(gb.gbda) - 2;
    while (x >= 0) {
      const auto current_point = point2D_t{x, y};
      if (getTileValueOnGameboard(gb.gbda, current_point)) {
        moveOnGameboard(gb, current_point, point2D_t{1, 0});
      }
      x--;
    }
  }
}

std::string printStateOfGameBoard(GameBoard gb) {
  return printStateOfGameBoardDataArray(gb.gbda);
}

} // namespace Game

using namespace Game;

std::ostream &operator<<(std::ostream &os, const GameBoard &gb) {
  return os << drawSelf(gb.gbda);
}
