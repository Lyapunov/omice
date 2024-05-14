#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <array>
#include <cassert>
#include <ostream>
#include <sstream>

constexpr int NUMBER_OF_ROWS = 8;
constexpr int NUMBER_OF_COLS = 8;
constexpr char NUMBER_OF_COLS_CHAR = 8;
constexpr unsigned NUMBER_OF_CASTS = 4;
constexpr unsigned NUMBER_OF_CLOCKS = 2;
constexpr unsigned NUMBER_OF_KINGS = 2;
constexpr int FIRST_ROW = 0;
constexpr int LAST_ROW = 7;
constexpr int FIRST_PAWN_ROW = 1;
constexpr int LAST_PAWN_ROW = 6;
constexpr int FIRST_EMP_ROW = 2;
constexpr int LAST_EMP_ROW = 5;
constexpr int HALF_ROW = 4;
constexpr unsigned CASTS_SIDES = 2;
constexpr int LONG_CASTLE_KING = 2;
constexpr int LONG_CASTLE_ROOK = 3;
constexpr int SHORT_CASTLE_KING = 6;
constexpr int SHORT_CASTLE_ROOK = 5;
constexpr unsigned HALF_CLOCK = 0;
constexpr unsigned FULL_CLOCK = 1;

constexpr char BOARD_DRAW_COL_SEPARATOR = '|';
constexpr char BOARD_DRAW_ROW_SEPARATOR = '-';
constexpr char BOARD_DRAW_CORNER= '*';
constexpr bool WHITE = true;
constexpr bool BLACK = false;
const std::string FIGURE_CONVERTER_BLACK = " pnbrqk";
const std::array<bool, 2> COLORS = {BLACK, WHITE};

enum class ChessFigure {
   None,
   Pawn,
   Knight,
   Bishop,
   Rook,
   Queen,
   King
};

char toChar( bool color, const ChessFigure fig ) {
   char chr = FIGURE_CONVERTER_BLACK[static_cast<unsigned>(fig)];
   return color ? toupper(chr) : chr;
}

ChessFigure toFigure( char chr ) {
   auto posw = FIGURE_CONVERTER_BLACK.find(tolower(chr));
   return posw != std::string::npos ? static_cast<ChessFigure>(posw) : ChessFigure::None;
}

struct Pos {
   Pos(char prow = 0, char pcol = 0) : row(prow), col(pcol) {}
   bool equals( const Pos& rhs ) const { return row == rhs.row && col == rhs.col; }
   void prevRow() { row--; col = 0; }
   void nextCol(int num = 1) { col+= num; }
   void move(const Pos& dir) { row += dir.row; col += dir.col; }
   bool valid() const { return row >= 0 && col >= 0 && row < NUMBER_OF_ROWS && col < NUMBER_OF_COLS; }
   char pcol() const { return 'a' + col; }
   char prow() const { return '1' + row; }
   unsigned char code() const { return (static_cast<unsigned char>(row) << 3) + static_cast<unsigned char>(col); }
   void debugPrint(std::ostream& os) const { os << pcol() << prow(); }
   Pos add(const Pos& rhs) const { return Pos(row+rhs.row, col+rhs.col); }
   Pos sub(const Pos& rhs) const { return Pos(row-rhs.row, col-rhs.col); }
   Pos towardCenter() const { return Pos(row < HALF_ROW ? row + 1 : row - 1, col); }
   Pos dir() const;
   bool isAxialDir() const { return !row || !col; }
   bool isDiagonal() const { return row && col; }
   bool null() const { return !row && !col; }
   bool isInDir( const Pos& dir ) const;
   ChessFigure minorType() const { return row && col ? ChessFigure::Bishop : ChessFigure::Rook; }
   char row;
   char col;
};
Pos PosFromCode(unsigned char code) { return Pos((code >> 3) & 7, code & 7); }

Pos INVALID = Pos(-1, -1);
Pos NULLPOS = Pos(0, 0);

std::ostream& operator<<(std::ostream& os, const Pos& pos);

struct Counter {
   void add(char col) { value_++; }
   char value_ = 0;
   static constexpr bool QUICK = false;
};

struct Finder {
   void add(char col) { value_ = col; }
   char value_ = -1;
   static constexpr bool QUICK = true;
};

template <unsigned BITS = 6>
struct MiniVector {
   static constexpr size_t BYTESIZE = 8;
   static constexpr size_t CAPACITY = (sizeof(unsigned long) * BYTESIZE -4) / BITS;
   static constexpr unsigned long BITMASK = (1 << BITS)-1;
   unsigned char size() const {return get(CAPACITY);}
   unsigned char get(size_t i) const { return (storage_ >> (BITS * i)) & BITMASK; }
   void clear() { storage_ = 0; }
   void clear(size_t i) { storage_ &= ~(BITMASK << (BITS*i)); }
   void set(size_t i, unsigned char num) {
      clear(i);
      storage_ |= static_cast<unsigned long>(num & BITMASK) << (BITS*i);
   }
   void setSize(unsigned char num) { set(CAPACITY, num); }
   void push_back(unsigned char num) {
      unsigned char siz = size();
      if ( unsigned(siz + 1) < CAPACITY ) {
         setSize(siz+1);
         set(siz, num);
      }
   }
   unsigned char pop_back() {
      unsigned char siz = size();
      if ( siz == 0 ) {
         return 0;
      }
      setSize(siz-1);
      return get(siz-1);
   }
   unsigned long storage_ = 0;
};
typedef MiniVector<6> MiniPosVector;
Pos get(const MiniPosVector& vec, size_t i) { return PosFromCode(vec.get(i)); }
void set(MiniPosVector& vec, size_t i, const Pos& pos) { vec.set(i, pos.code()); }
void push_back(MiniPosVector& vec, const Pos& pos) { vec.push_back(pos.code()); }
std::ostream& operator<<(std::ostream& os, const MiniPosVector& vec);

struct ChessRow {
   ChessRow() : data_(0) {}

   void clear() { data_ = 0; }
   void clear(unsigned col) { data_ &= ~(unsigned(15) << (col << 2)); }

   void set(unsigned col, bool color, const ChessFigure& fig) {
      clear(col);
      data_ |= (((static_cast<unsigned>(fig) << 1)+color) << (col << 2));
   }

   ChessFigure getFigure(unsigned col) const { return static_cast<ChessFigure>((data_ >> ((col << 2)+1)) & 7); }
   bool getColor(unsigned col) const { return (data_ >> (col << 2)) & 1; }

   template <class Collector>
   char iter(const bool color, const ChessFigure& fig) const {
      Collector coll;
      auto acc = data_;
      for ( char col = 0; col < NUMBER_OF_COLS_CHAR; col++ ) {
         if ( static_cast<ChessFigure>((acc >> 1) & 7) == fig && (acc & 1) == color ) {
            coll.add(col);
            if ( Collector::QUICK ) {
               return coll.value_;
            }
         }
         acc >>= 4;
      }
      return coll.value_;
   }
   unsigned count(const bool color, const ChessFigure& fig) const { return iter<Counter>(color, fig); }
   bool find(const bool color, const ChessFigure& fig, Pos& result) const { return ( result.col = iter<Finder>(color, fig) ) >= 0; }

   void debugPrint(std::ostream& os, char separator) const {
     for ( int col = 0; col < NUMBER_OF_COLS; col++ ) {
        os << separator << toChar(getColor(col), getFigure(col));
     }
     os << separator;
   }

   unsigned data_;
};

bool operator==( const Pos& lhs, const Pos& rhs ) {
   return lhs.equals(rhs);
}

struct ChessBoard {
   ChessBoard() : data_(), color_(true), casts_({'-', '-', '-', '-'}), enpassant_('-'), clocks_({0,0}) {}

   bool initFEN(const std::string& fen, const std::string& white, const std::string& casts, const std::string& enpassant, unsigned char halfMoveClock, unsigned char fullClock); 
   bool initFEN(const std::string& str);

   void init() {
      assert( initFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "AHah", "-", 0, 1) );
   }
   Pos getCastPos(unsigned i) const {
      return casts_[i] == '-' ? INVALID : Pos(i < CASTS_SIDES ? FIRST_ROW : LAST_ROW, toupper(casts_[i]) - 'A');
   }
   Pos getCastPos(bool color, unsigned i) const {
      return getCastPos(i + (color ? 0 : CASTS_SIDES));
   }
   bool valid() const;
   void debugPrintRowSeparator(std::ostream& os) const {
      for ( int col = 0; col < NUMBER_OF_COLS; col++ ) {
         os << BOARD_DRAW_CORNER << BOARD_DRAW_ROW_SEPARATOR;
      }
      os << BOARD_DRAW_CORNER << std::endl;
   }
   bool getColor(const Pos& pos) const { return data_[pos.row].getColor(pos.col); }
   ChessFigure getFigure(const Pos& pos) const { return pos.valid() ? data_[pos.row].getFigure(pos.col) : ChessFigure::None; }
   void set(const Pos& pos, bool color, ChessFigure fig) {
      assert( pos.row >= 0 && pos.row < NUMBER_OF_ROWS );
      data_[pos.row].set(pos.col, color, fig);
      if ( fig == ChessFigure::King ) {
         kings_[color] = pos;
      }
   }
   bool isEnpassantTarget(const Pos& to) const {
      return to.row == (color_ ? LAST_EMP_ROW : FIRST_EMP_ROW ) && to.pcol() == enpassant_;
   }
   bool isPromotion(const Pos& from, const Pos& to, const ChessFigure& stype) const {
      return stype == ChessFigure::Pawn && to.row == ( color_ ? LAST_ROW : FIRST_ROW );
   }
   bool isFastPawn(const Pos& from, const Pos& to, const ChessFigure& stype) const {
      return stype == ChessFigure::Pawn && abs(to.row - from.row) == 2;
   }
   bool testCastleWalk(const Pos& from, const Pos& to, int row, int source, int target, bool king) const;

   bool isMoveValidInternal(const Pos& from, const Pos& to, const ChessFigure& stype, const ChessFigure& ttype ) const;
   bool isCastleValid(const Pos& from, const Pos& to) const;
   bool isMoveValid(const Pos& from, const Pos& to) const;
   bool isPinned(const Pos& pos) const;
   Pos getWatcherFromLine(bool attackerColor, const Pos& pos, const Pos& dir) const;
   unsigned char countWatchers(const bool color, const Pos& pos, unsigned char maxval = 255, const Pos& newBlocker = INVALID) const;
   bool hasWatcher(const bool color, const Pos& pos) const { return countWatchers(color, pos, 1); }
   bool check(bool color) const { return countWatchers(!color, kings_[color], 1); }
   unsigned char checkType(bool color) const { return countWatchers(!color, kings_[color], 2); }

   unsigned count(const bool color, const ChessFigure& fig) const {
      unsigned retval = 0;
      for ( const auto& elem : data_ ) {
         retval += elem.count(color, fig);
      }
      return retval;
   }

   Pos find(const bool color, const ChessFigure& fig) const {
      Pos result;
      for ( int row = 0; row < NUMBER_OF_ROWS ; row++ ) {
         if ( data_[row].find(color, fig, result) ) {
            result.row = row;
            return result;
         }
      }
      return INVALID;
   }

   void applyMove(const Pos& from, const Pos& to, const ChessFigure promoteTo = ChessFigure::Queen);
   bool move(const Pos& from, const Pos& to, const ChessFigure promoteTo = ChessFigure::Queen); 
   bool move(const std::string& desc);
   bool isMobilePiece(const Pos& pos, unsigned char cktype) const;
   void listMobilePieces(MiniPosVector& pawns, MiniPosVector& pieces) const;
   void debugPrint(std::ostream& os) const;

   std::array<ChessRow, NUMBER_OF_ROWS> data_;
   bool color_; // white = true, blue = false
   std::array<char, NUMBER_OF_CASTS> casts_;
   char enpassant_;
   std::array<unsigned char, NUMBER_OF_CLOCKS> clocks_;
   std::array<Pos, NUMBER_OF_KINGS> kings_;
};

std::ostream& operator<<(std::ostream& os, const ChessBoard& board);

#endif /* PRIMITIVES_H */
