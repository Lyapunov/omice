#ifndef PRIMITIVES_H
#define PRIMITIVES_H

#include <array>
#include <cassert>
#include <ostream>

constexpr unsigned NUMBER_OF_ROWS = 8;
constexpr unsigned NUMBER_OF_COLS = 8;
constexpr unsigned NUMBER_OF_CASTS = 4;
constexpr unsigned FIRST_ROW = 0;
constexpr unsigned FIRST_PAWN_ROW = 1;
constexpr unsigned LAST_ROW = 7;
constexpr unsigned LAST_PAWN_ROW = 6;
constexpr char BOARD_DRAW_COL_SEPARATOR = '|';
constexpr char BOARD_DRAW_ROW_SEPARATOR = '-';
constexpr char BOARD_DRAW_CORNER= '*';
constexpr bool WHITE = true;
constexpr bool BLACK = false;
const std::string FIGURE_CONVERTER_BLACK = " pnbrqk";
const std::string FIGURE_CONVERTER_WHITE = " PNBRQK";
const std::array<bool, 2> COLORS = {BLACK, WHITE};

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
   unsigned data_;
};

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
   int row;
   int col;
};

bool operator==( const Pos& lhs, const Pos& rhs ) {
   return lhs.equals(rhs);
}

bool hasEnpassantColRank(char enpassant, const Pos& to, bool color) {
   if ( enpassant == '-' ) {
      return false;
   }
   if ( to.row != static_cast<int>(color ? LAST_PAWN_ROW : FIRST_PAWN_ROW ) ) {
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
      if ( casts.size() >= NUMBER_OF_CASTS ) {
         for ( unsigned i = 0; i < NUMBER_OF_CASTS; i++ ) {
            casts_[i] = casts[i];
         }
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
   bool valid() const {
      for ( const auto& color : COLORS ) {
         if ( count(color, ChessFigure::King) != 1 ) {
            return false;
         }
         if ( data_[color ? LAST_ROW : FIRST_ROW].count(color, ChessFigure::Pawn) ) {
            return false;
         }
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
   ChessFigure getFigure(const Pos& pos) const { return data_[pos.row].getFigure(pos.col); }
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
   bool isMoveValid(const Pos& from, const Pos& to) const;

   unsigned count(const bool color, const ChessFigure& fig) const {
      unsigned retval = 0;
      for ( const auto& elem : data_ ) {
         retval += elem.count(color, fig);
      }
      return retval;
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
   }
   std::array<ChessRow, NUMBER_OF_ROWS> data_;
   bool color_; // white = true, blue = false
   std::array<char, NUMBER_OF_CASTS> casts_;
   char enpassant_;
};

std::ostream& operator<<(std::ostream& os, const ChessBoard& board);
std::ostream& operator<<(std::ostream& os, const Pos& pos);

#endif /* PRIMITIVES_H */
