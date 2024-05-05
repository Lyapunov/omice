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
const std::string FIGURE_CONVERTER = " pnbrqkPNBRQK ";
const std::string FIGURE_CONVERTER_SAFE = " pn rqkPNBRQK ";

enum class ChessFigure {
   None,
   BlackPawn,
   BlackKnight,
   BlackBishop,
   BlackRook,
   BlackQueen,
   BlackKing,
   WhitePawn,
   WhiteKnight,
   WhiteBishop,
   WhiteRook,
   WhiteQueen,
   WhiteKing,
   NumberOfValues
};

enum class ChessFigureType {
   None,
   Pawn,
   Knight,
   Bishop,
   Rook,
   Queen,
   King,
   NumberOfValues
};

char toChar( const ChessFigure& fig ) {
   return FIGURE_CONVERTER[static_cast<unsigned>(fig)];
}

ChessFigure toFigure( char chr ) {
   auto pos = FIGURE_CONVERTER.find(chr);
   return pos == std::string::npos ? ChessFigure::None : static_cast<ChessFigure>(pos);
}

ChessFigureType toType( const ChessFigure& fig ) {
   if ( fig == ChessFigure::None ) {
      return ChessFigureType::None;
   }
   if ( fig >= ChessFigure::WhitePawn ) {
      return static_cast<ChessFigureType>(static_cast<unsigned>(ChessFigureType::Pawn) + ( static_cast<unsigned>(fig) - static_cast<unsigned>(ChessFigure::WhitePawn) ));
   }
   return static_cast<ChessFigureType>(static_cast<unsigned>(ChessFigureType::Pawn) + ( static_cast<unsigned>(fig) - static_cast<unsigned>(ChessFigure::BlackPawn) ));
}

ChessFigureType toType( char chr ) {
   return toType(toFigure(chr));
}

bool hasColor( const ChessFigure& fig, bool color ) { // white = true, black = false 
   return color == ( ChessFigure::WhitePawn <= fig );
}

struct ChessRow {
   ChessRow() : data_(0) { assert( static_cast<unsigned>(ChessFigure::NumberOfValues) < 16 ); } // update bit masks if fails! 
   ChessFigure get(unsigned col) const { return static_cast<ChessFigure>((data_ >> (col << 2)) & 15); }
   void clear() {
      data_ = 0;
   }
   void clear(unsigned col) {
      unsigned mask = ~(unsigned(15) << (col << 2));
      data_ &= mask;
   }
   void set(unsigned col, const ChessFigure& figure) {
      assert( col >= 0 && col < NUMBER_OF_COLS );
      clear(col);
      unsigned mask = static_cast<unsigned>(figure) << (col << 2);
      data_ |= mask;
   }
   void debugPrint(std::ostream& os, char separator) const {
     for ( unsigned col = 0; col < NUMBER_OF_COLS; col++ ) {
        os << separator << toChar(get(col));
     }
     os << separator;
   }
   unsigned count(const ChessFigure& fig) const {
      unsigned retval = 0;
      for ( unsigned col = 0; col < NUMBER_OF_COLS; col++ ) {
         if ( get(col) == fig ) {
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
            auto figure = toFigure(elem);
            if ( figure == ChessFigure::None ) {
               return false;
            }
            set(pos, figure);
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
      if ( count(ChessFigure::BlackKing) != 1 || count(ChessFigure::WhiteKing) != 1 ) {
         return false;
      }
      if ( data_[FIRST_ROW].count(ChessFigure::BlackPawn) ) {
         return false;
      }
      if ( data_[LAST_ROW].count(ChessFigure::WhitePawn) ) {
         return false;
      }
      return false;
   }
   void debugPrintRowSeparator(std::ostream& os) const {
      for ( unsigned col = 0; col < NUMBER_OF_COLS; col++ ) {
         os << BOARD_DRAW_CORNER << BOARD_DRAW_ROW_SEPARATOR;
      }
      os << BOARD_DRAW_CORNER << std::endl;
   }
   ChessFigure get(const Pos& pos) const { return data_[pos.row].get(pos.col); }
   void set(const Pos& pos, ChessFigure figure) {
      assert( pos.row >= 0 && pos.row < int(NUMBER_OF_ROWS) );
      data_[pos.row].set(pos.col, figure);
   }
   bool isEmpassant(const Pos& from, const Pos& to, const ChessFigureType& stype) const {
      return stype == ChessFigureType::Pawn && hasEnpassantColRank(enpassant_, to, color_);
   }
   bool isPromotion(const Pos& from, const Pos& to, const ChessFigure& source) const {
      return toType(source) == ChessFigureType::Pawn && to.row == static_cast<int>( color_ ? LAST_ROW : FIRST_ROW );
   }
   bool isFastPawn(const Pos& from, const Pos& to, const ChessFigure& source) const {
      return toType(source) == ChessFigureType::Pawn && abs(to.row - from.row) == 2;
   }

   bool isJumpValid(const Pos& from, const Pos& to, const ChessFigureType& stype ) const;
   bool isTakeValid(const Pos& from, const Pos& to, const ChessFigureType& stype ) const;
   bool isMoveValid(const Pos& from, const Pos& to) const;

   unsigned count(const ChessFigure& fig) const {
      unsigned retval = 0;
      for ( const auto& elem : data_ ) {
         retval += elem.count(fig);
      }
      return retval;
   }
   bool isFigureChar(char chr) {
      if ( chr == ' ' ) {
         return false;
      }

      auto fpos = FIGURE_CONVERTER_SAFE.find(chr);
      if ( fpos == std::string::npos ) {
         return false;
      }
      return true;
   }

   void doMove(const Pos& from, const Pos& to, const ChessFigureType promoteTo = ChessFigureType::Queen);
   bool move(const Pos& from, const Pos& to, const ChessFigureType promoteTo = ChessFigureType::Queen); 
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

std::ostream& operator<<(std::ostream& os, const ChessBoard& board) {
   board.debugPrint(os);
   return os;
}

#endif /* PRIMITIVES_H */
