#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <array>
#include <cassert>
#include <ostream>

constexpr unsigned NUMBER_OF_ROWS = 8;
constexpr unsigned NUMBER_OF_COLS = 8;
constexpr unsigned NUMBER_OF_CASTS = 4;
constexpr unsigned FIRST_ROW = 0;
constexpr unsigned LAST_ROW = 7;
constexpr unsigned FIRST_PAWN_ROW = 1;
constexpr unsigned LAST_PAWN_ROW = 6;
constexpr unsigned FIRST_EMP_ROW = 2;
constexpr unsigned LAST_EMP_ROW = 5;
constexpr unsigned HALF_ROW = 4;
constexpr unsigned CASTS_SIDES = 2;
constexpr unsigned LONG_CASTLE_KING = 2;
constexpr unsigned LONG_CASTLE_ROOK = 3;
constexpr unsigned SHORT_CASTLE_KING = 6;
constexpr unsigned SHORT_CASTLE_ROOK = 5;

constexpr char BOARD_DRAW_COL_SEPARATOR = '|';
constexpr char BOARD_DRAW_ROW_SEPARATOR = '-';
constexpr char BOARD_DRAW_CORNER= '*';
constexpr bool WHITE = true;
constexpr bool BLACK = false;
const std::string FIGURE_CONVERTER_BLACK = " pnbrqk";
const std::string FIGURE_CONVERTER_WHITE = " PNBRQK";
const std::array<bool, 2> COLORS = {BLACK, WHITE};
const std::array<int, 2> PAWNDIRS = {-1, +1};

enum class ChessFigure {
   None,
   Pawn,
   Knight,
   Bishop,
   Rook,
   Queen,
   King,
   NumberOfValues
};

char toChar( bool color, const ChessFigure fig ) {
   return color
          ? FIGURE_CONVERTER_WHITE[static_cast<unsigned>(fig)]
          : FIGURE_CONVERTER_BLACK[static_cast<unsigned>(fig)];
}

ChessFigure toFigure( char chr ) {
   auto posw = FIGURE_CONVERTER_WHITE.find(chr);
   if ( posw != std::string::npos ) {
      return static_cast<ChessFigure>(posw);
   }
   auto posb = FIGURE_CONVERTER_BLACK.find(chr);
   if ( posb != std::string::npos ) {
      return static_cast<ChessFigure>(posb);
   }
   return ChessFigure::None;
}

struct Pos {
   Pos(int prow = 0, int pcol = 0) : row(prow), col(pcol) {}
   bool equals( const Pos& rhs ) const { return row == rhs.row && col == rhs.col; }
   void prevRow() { row--; col = 0; }
   void nextCol(int num = 1) { col+= num; }
   void move(int dx, int dy) { row += dx; col += dy; }
   bool valid() const { return row >= 0 && col >= 0 && row < static_cast<int>(NUMBER_OF_ROWS) && col < static_cast<int>(NUMBER_OF_COLS); }
   void debugPrint(std::ostream& os) const {
      os << "{" << row << ", " << col << "}";
   }
   Pos offset(const Pos& rhs) const { return Pos(row+rhs.row, col+rhs.col); }
   Pos towardCenter() const { return Pos(row < static_cast<int>(HALF_ROW) ? row + 1 : row - 1, col); }
   int row;
   int col;
};

std::ostream& operator<<(std::ostream& os, const Pos& pos);

struct ChessRow {
   ChessRow() : data_(0) {}

   void set(unsigned col, bool color, const ChessFigure& fig) {
      assert( col >= 0 && col < NUMBER_OF_COLS );
      clear(col);
      unsigned mask = ((static_cast<unsigned>(fig) << 1)+color)<< (col << 2);
      data_ |= mask;
   }

   ChessFigure getFigure(unsigned col) const { return static_cast<ChessFigure>((data_ >> ((col << 2)+1)) & 7); }
   bool getColor(unsigned col) const { return (data_ >> (col << 2)) & 1; }

   void clear() {
      data_ = 0;
   }
   void clear(unsigned col) {
      unsigned mask = ~(unsigned(15) << (col << 2));
      data_ &= mask;
   }
   void debugPrint(std::ostream& os, char separator) const {
     for ( unsigned col = 0; col < NUMBER_OF_COLS; col++ ) {
        os << separator << toChar(getColor(col), getFigure(col));
     }
     os << separator;
   }
   unsigned count(const bool color, const ChessFigure& fig) const {
      unsigned retval = 0;
      for ( unsigned col = 0; col < NUMBER_OF_COLS; col++ ) {
         if ( getFigure(col) == fig && getColor(col) == color ) {
            retval++;
         }
      }
      return retval;
   }
   bool find(const bool color, const ChessFigure& fig, Pos& result) const {
      for ( unsigned col = 0; col < NUMBER_OF_COLS; col++ ) {
         if ( getFigure(col) == fig && getColor(col) == color ) {
            result.col = int(col);
            return true;
         }
      }
      return false;
   }
   unsigned data_;
};

bool operator==( const Pos& lhs, const Pos& rhs ) {
   return lhs.equals(rhs);
}

bool hasEnpassantColRank(char enpassant, const Pos& to, bool color) {
   if ( enpassant == '-' ) {
      return false;
   }
   if ( to.row != static_cast<int>(color ? LAST_EMP_ROW : FIRST_EMP_ROW ) ) {
      return false;
   }
   int ecol = int(enpassant - 'a');
   assert(ecol > 0 && ecol < static_cast<int>(NUMBER_OF_COLS));
   return to.col == ecol;
}

struct ChessBoard {
   ChessBoard() : data_(), color_(true), casts_({'-', '-', '-', '-'}), enpassant_('-') {}
   bool initFEN(const std::string& fen, const std::string& white, const std::string& casts, const std::string& enpassant) {
      Pos pos(NUMBER_OF_ROWS-1, 0);
      for ( const char elem : fen ) {
         if ( elem == '/' ) {
            pos.prevRow();
         } else if ( elem >= '0' && elem <= '9' ) {
            pos.nextCol(elem - '0');
         } else {
            auto fig = toFigure(elem);
            if ( fig == ChessFigure::None ) {
               return false;
            }
            set(pos, isupper(elem), fig);
            pos.nextCol();
         }
      }
      color_ = ( white.size() >= 1 && white[0] == 'w' );
      casts_ = {'-', '-', '-', '-'};
      std::array<unsigned, 2> cpos = {0, 0};
      for ( const auto& elem : casts ) {
         if ( elem == '-' ) {
            continue;
         }
         bool color = isupper(elem);
         if ( cpos[color] >= CASTS_SIDES ) {
            return false;
         }
         casts_[(color ? CASTS_SIDES : 0) + cpos[color]] = elem;
         cpos[color]++;
      }
      if ( casts.size() < NUMBER_OF_CASTS ) {
         return false;
      }
      for ( unsigned i = 0; i < NUMBER_OF_CASTS; i++ ) {
         if ( casts[i] != '-' && ( i < CASTS_SIDES ? !isupper(casts[i]) : isupper(casts[i]) ) ) {
            return false;
         }
         casts_[i] = casts[i];
      }
      if ( enpassant.size() > 1 ) {
         enpassant_ = enpassant[0];
      }
      return true;
   }
   void init() {
      clear();
      assert( initFEN("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR", "w", "AHah", "-") );
   }
   void clear() {
      for ( auto& elem : data_ ) {
         elem.clear();
      }
   }
   Pos getCastPos(unsigned i) const {
      if ( casts_[i] == '-' ) {
         return Pos(-1,-1);
      }
      bool color = i < CASTS_SIDES;
      auto col = toupper(casts_[i]) - 'A';
      auto row = color ? FIRST_ROW : LAST_ROW;
      return Pos(row, col);
   }
   Pos getCastPos(bool color, unsigned i) const {
      auto si = i + (color ? 0 : CASTS_SIDES);
      return getCastPos(si);
   }
   bool valid() const {
      for ( const auto& color : COLORS ) {
         if ( count(color, ChessFigure::King) != 1 ) {
            return false;
         }
         if ( data_[color ? LAST_ROW : FIRST_ROW].count(color, ChessFigure::Pawn) ) {
            return false;
         }
         for ( unsigned i = 0; i < CASTS_SIDES; i++ ) {
            const auto pos = getCastPos(color, i);
            if ( pos.valid() && !( getColor(pos) == color && getFigure(pos) == ChessFigure::Rook ) ) {
               return false;
            }
         }
      }
      Pos kpos;
      find(!color_, ChessFigure::King, kpos);
      if ( isInAttack(color_, kpos) ) {
         return false;
      }
      return true;
   }
   void debugPrintRowSeparator(std::ostream& os) const {
      for ( unsigned col = 0; col < NUMBER_OF_COLS; col++ ) {
         os << BOARD_DRAW_CORNER << BOARD_DRAW_ROW_SEPARATOR;
      }
      os << BOARD_DRAW_CORNER << std::endl;
   }
   bool getColor(const Pos& pos) const { return data_[pos.row].getColor(pos.col); }
   ChessFigure getFigure(const Pos& pos) const { return pos.valid() ? data_[pos.row].getFigure(pos.col) : ChessFigure::None; }
   void set(const Pos& pos, bool color, ChessFigure fig) {
      assert( pos.row >= 0 && pos.row < int(NUMBER_OF_ROWS) );
      data_[pos.row].set(pos.col, color, fig);
   }
   bool isEmpassant(const Pos& from, const Pos& to, const ChessFigure& stype) const {
      return stype == ChessFigure::Pawn && hasEnpassantColRank(enpassant_, to, color_);
   }
   bool isPromotion(const Pos& from, const Pos& to, const ChessFigure& stype) const {
      return stype == ChessFigure::Pawn && to.row == static_cast<int>( color_ ? LAST_ROW : FIRST_ROW );
   }
   bool isFastPawn(const Pos& from, const Pos& to, const ChessFigure& stype) const {
      return stype == ChessFigure::Pawn && abs(to.row - from.row) == 2;
   }

   bool isJumpValid(const Pos& from, const Pos& to, const ChessFigure& stype ) const;
   bool isTakeValid(const Pos& from, const Pos& to, const ChessFigure& stype ) const;
   bool isCastleValid(const Pos& from, const Pos& to) const;
   bool isMoveValid(const Pos& from, const Pos& to) const;
   bool isInAttack(const bool color, const Pos& pos) const;

   unsigned count(const bool color, const ChessFigure& fig) const {
      unsigned retval = 0;
      for ( const auto& elem : data_ ) {
         retval += elem.count(color, fig);
      }
      return retval;
   }

   bool find(const bool color, const ChessFigure& fig, Pos& result) const {
      for ( int row = NUMBER_OF_ROWS - 1; row >= 0; row-- ) {
         if ( data_[row].find(color, fig, result) ) {
            result.row = row;
            return true;
         }
      }
      return false;
   }

   void doMove(const Pos& from, const Pos& to, const ChessFigure promoteTo = ChessFigure::Queen);
   bool move(const Pos& from, const Pos& to, const ChessFigure promoteTo = ChessFigure::Queen); 
   bool move(const std::string& desc);

   void debugPrint(std::ostream& os) const {
      for ( int row = NUMBER_OF_ROWS - 1; row >= 0; row-- ) {
         debugPrintRowSeparator(os);
         data_[row].debugPrint(os, BOARD_DRAW_COL_SEPARATOR);
         os << std::endl;
      }
      debugPrintRowSeparator(os);
      os << "/" << casts_[0] << casts_[1] << casts_[2] << casts_[3] << "/ " << enpassant_ << std::endl;
   }
   std::array<ChessRow, NUMBER_OF_ROWS> data_;
   bool color_; // white = true, blue = false
   std::array<char, NUMBER_OF_CASTS> casts_;
   char enpassant_;
};

std::ostream& operator<<(std::ostream& os, const ChessBoard& board);

#endif /* PRIMITIVES_H */
