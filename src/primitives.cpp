#include "primitives.hpp"

const std::array<Pos, 8> KNIGHTDIRS = {Pos(1,2), Pos(-1,2), Pos(1,-2), Pos(-1,-2), Pos(2,1), Pos(-2,1), Pos(2,-1), Pos(-2,-1)};
const std::array<Pos, 8> DIRS = {Pos(1,0), Pos(1,1), Pos(0,1), Pos(-1,1), Pos(-1,0), Pos(-1,-1), Pos(0,-1), Pos(1,-1)};

Pos
Pos::dir() const {
   if ( !row && col ) {
      return Pos(0, col > 0 ? +1: -1);
   }
   if ( row && !col ) {
      return Pos(row > 0 ? +1 : -1, 0);
   }
   if ( row == col ) {
      char common = row > 0 ? +1 : -1;
      return Pos(common, common);
   }
   return NULLPOS;
}

bool
Pos::isInDir( const Pos& dir ) const {
   if ( null() ) {
      return false;
   }
   char z = dir.row ? row / dir.row : col / dir.col;
   return row == z * dir.row && col == z * dir.col;
}

bool
ChessBoard::initFEN(const std::string& fen, const std::string& white, const std::string& casts, const std::string& enpassant, unsigned char halfMoveClock, unsigned char fullClock) {
   for ( auto& elem : data_ ) {
      elem.clear();
   }
   Pos pos(NUMBER_OF_ROWS-1, 0);
   for ( const char elem : fen ) {
      if ( elem == '/' ) {
         pos.prevRow();
      } else if ( isdigit(elem) ) {
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
   if ( white[0] != 'w' && white[0] != 'b' ) {
      return false;
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
      casts_[(color ? 0 : CASTS_SIDES) + cpos[color]] = elem;
      cpos[color]++;
   }
   enpassant_ = enpassant.size() >= 1 ? enpassant[0] : '-';
   clocks_[HALF_CLOCK] = halfMoveClock;
   clocks_[FULL_CLOCK] = fullClock;
   kings_ = { find(BLACK, ChessFigure::King), find(WHITE, ChessFigure::King) };
   return true;
}

bool
ChessBoard::initFEN(const std::string& str) {
   std::string fen, white, casts, enpassant;
   unsigned halfMoveClock, fullClock;
   std::stringstream(str) >> fen >> white >> casts >> enpassant >> halfMoveClock >> fullClock;
   return initFEN(fen, white, casts, enpassant, halfMoveClock, fullClock);
}

bool
ChessBoard::valid() const {
   for ( const auto& color : COLORS ) {
      if ( count(color, ChessFigure::King) != 1 || getFigure(kings_[color]) != ChessFigure::King || getColor(kings_[color]) != color || data_[color ? LAST_ROW : FIRST_ROW].count(color, ChessFigure::Pawn) ) {
         return false;
      }
      for ( unsigned i = 0; i < CASTS_SIDES; i++ ) {
         const auto pos = getCastPos(color, i);
         if ( pos.valid() && !( getColor(pos) == color && getFigure(pos) == ChessFigure::Rook ) ) {
            return false;
         }
      }
   }
   return !hasWatcher(color_, kings_[!color_]);
}

bool
ChessBoard::isStepValid(const Pos& from, const Pos& to, const ChessFigure& stype) const {
   switch ( stype ) {
      case ChessFigure::Pawn:
         if ( from.col != to.col ) {
            return false;
         }
         if ( !( color_
                 ? from.row + 1 == to.row || ( from.row == FIRST_PAWN_ROW && from.row + 2 == to.row )
                 : from.row == to.row + 1 || ( from.row == LAST_PAWN_ROW  && from.row == to.row + 2 ) ) ) {
            return false;
         }
         return true;
      case ChessFigure::Knight:
         return (abs(from.row - to.row) == 1 && abs(from.col - to.col) == 2)
             || (abs(from.row - to.row) == 2 && abs(from.col - to.col) == 1);
      case ChessFigure::Bishop:
         if ( abs(from.row - to.row) != abs(from.col - to.col) ) {
            return false;
         }
         {
            Pos acc = from;
            Pos dir(to.row > from.row ? +1 : -1, to.col > from.col ? +1 : -1);
            acc.move(dir);
            while ( !(acc == to) ) {
               if ( getFigure(acc) != ChessFigure::None ) {
                  return false;
               }
               acc.move(dir);
            }
         }
         return true;
      case ChessFigure::Rook:
         if ( from.row != to.row && from.col != to.col ) {
            return false;
         }
         {
            Pos acc = from;
            Pos dir(to.row > from.row ? +1 : (to.row == from.row ? 0 : -1), to.col > from.col ? +1 : (to.col == from.col ? 0 : -1));
            acc.move(dir);
            while ( !(acc == to) ) {
               if ( getFigure(acc) != ChessFigure::None ) {
                  return false;
               }
               acc.move(dir);
            }
         }
         return true;
      case ChessFigure::Queen:
         return isStepValid(from, to, ChessFigure::Rook) || isStepValid(from, to, ChessFigure::Bishop);
      case ChessFigure::King:
         return abs(from.col - to.col) <= 1 && abs(from.row - to.row) <= 1 && !hasWatcher(!color_, to);
      default:
         return false;
   }
}

bool
ChessBoard::testCastleWalk(const Pos& from, const Pos& to, int row, int source, int target, bool king) const {
   for ( int lcol = std::min(source, target); lcol <= std::max(source, target); lcol++ ) {
      // is vacant?
      Pos lpos(row, lcol);
      if ( (!(lpos == to) && !(lpos == from) && getFigure(lpos) != ChessFigure::None) || (king && hasWatcher(!color_, lpos) ) ) {
         return false;
      }
   }
   return true;
}

bool
ChessBoard::isCastleValid(const Pos& from, const Pos& to) const {
   auto row = color_ ? FIRST_ROW : LAST_ROW;
   return from.row == int(row) && to.row == int(row)
       && testCastleWalk(from, to, row, from.col, to.col < from.col ? LONG_CASTLE_KING : SHORT_CASTLE_KING, true)
       && testCastleWalk(from, to, row, to.col,   to.col < from.col ? LONG_CASTLE_ROOK : SHORT_CASTLE_ROOK, false);
}

bool
ChessBoard::isTakeValid(const Pos& from, const Pos& to, const ChessFigure& stype ) const {
   return stype == ChessFigure::Pawn
          ? abs(from.col - to.col) == 1 && ( color_ ? from.row + 1 == to.row : from.row == to.row + 1 )
          : isStepValid(from, to, stype);
}

bool
ChessBoard::isMoveValid(const Pos& from, const Pos& to) const {
   if ( from == to ) {
      return false;
   }
   const auto scolor = getColor(from);
   const auto stype = getFigure(from);
   if ( stype == ChessFigure::None || scolor != color_ ) {
      return false;
   }
   if ( stype != ChessFigure::King && isPinned(from) ) {
      Pos dir = from.sub(kings_[scolor]).dir();
      if ( !to.sub(from).isInDir(dir) ) {
         return false;
      }
   }
   const auto tcolor = getColor(to);
   const auto ttype = getFigure(to);
   if ( ttype != ChessFigure::None && scolor == tcolor ) {
      return stype == ChessFigure::King && ttype == ChessFigure::Rook && isCastleValid(from, to);
   }
   return (ttype != ChessFigure::None || isEnpassant(from, to, stype))
          ? isTakeValid(from, to, stype)
          : isStepValid(from, to, stype);
}

Pos
ChessBoard::getWatcherFromLine(bool attackerColor, const Pos& pos, const Pos& dir) const {
   Pos acc = pos;
   acc.move(dir);

   // Pawn
   if ( dir.col && dir.row == (attackerColor ? -1 : +1) && getFigure(acc) == ChessFigure::Pawn && getColor(acc) == attackerColor ) {
      return pos;
   }

   // King
   if ( getFigure(acc) == ChessFigure::King && getColor(acc) == attackerColor ) {
      return pos;
   }

   // Bishop, Rook, Queen
   while ( acc.valid() && getFigure(acc) == ChessFigure::None ) {
      acc.move(dir);
   }
   auto minorType = dir.minorType();
   if ( acc.valid() && getColor(acc) == attackerColor ) {
      auto atype = getFigure(acc);
      if ( atype == ChessFigure::Queen || atype == minorType ) {
         return pos;
      }
   }
   return INVALID;
}

bool
ChessBoard::hasWatcher(bool attackerColor, const Pos& pos) const {
   if ( !pos.valid() ) {
      return false;
   }

   // Knight
   for ( const auto& kdir: KNIGHTDIRS ) {
      auto tpos = pos.offset(kdir);
      if ( getFigure(tpos) == ChessFigure::Knight && getColor(tpos) == attackerColor ) {
         return true;
      }
   }

   // Figures that are attacking through lines
   for ( const auto& dir : DIRS ) {
      if ( getWatcherFromLine(attackerColor, pos, dir).valid() ) {
         return true;
      }
   }

   return false;
}

bool
ChessBoard::isPinned(const Pos& pos) const {
   const auto pcolor = getColor(pos);
   const auto ptype = getFigure(pos);
   if ( ptype == ChessFigure::None) {
      return false;
   }
   Pos dir = pos.sub(kings_[pcolor]).dir();
   if ( dir.null() ) {
      return false;
   }
   Pos ipos = getWatcherFromLine(pcolor, kings_[pcolor], dir);
   if ( !(ipos == pos) ) {
      return false;
   }
   Pos wpos = getWatcherFromLine(!pcolor, pos, dir);
   if ( !wpos.valid() ) {
      return false;
   }
   const auto wcolor = getColor(wpos);
   const auto wtype = getFigure(wpos);
   if ( wcolor == pcolor ) {
      return false;
   }
   if ( wtype != ChessFigure::Queen && wtype != (dir.isAxialDir() ? ChessFigure::Rook : ChessFigure::Bishop) ) {
      return false;
   }
   return true;
}

static bool
IsFigureChar(char chr) {
   static const std::string FIGURE_CONVERTER_SAFE = "pnrqkPNBRQK";
   return chr != ' ' && FIGURE_CONVERTER_SAFE.find(chr) != std::string::npos;
}

bool
ChessBoard::move(const std::string& desc) {
   if ( !valid() ) {
      return false;
   }

   // format: nf6, Nf6, Ng8f6, g8f6, g8, g8=Q, g8=N ...
   char type = ' ';
   char prom = ' ';
   int acol = -1;
   int arow = -1;
   int bcol = -1;
   int brow = -1;
   unsigned casts = 0;
   for ( const auto& chr : desc ) {
      if ( IsFigureChar(chr) ) {
         if ( type == ' ' ) {
            type = toupper(chr);
         } else if ( prom == ' ' ) {
            prom = toupper(chr);
         } else {
            return false;
         }
      } else if ( chr >= '1' && chr <= '9' ) {
         if ( brow == -1 )  {
            brow = chr - '1';
         } else if ( arow == -1 ) {
            arow = brow;
            brow = chr - '1';
         } else {
            return false;
         }
      } else if ( chr >= 'a' && chr <= 'h' ) {
         if ( bcol == -1 )  {
            bcol = chr - 'a';
         } else if ( acol == -1 ) {
            acol = bcol;
            bcol = chr - 'a';
         } else {
            return false;
         }
      } else if ( chr == '=' ) {
         if ( type == ' ' ) {
           type = 'P';
         }
      } else if ( chr == 'O' || chr == 'o' ) {
         casts++;
      }
   }
   if ( casts > 0 ) {
      if ( type != ' ' || prom != ' ' || acol != -1 || arow != -1 || bcol != -1 || brow != -1) {
         return false;
      }
      Pos kpos = find(color_, ChessFigure::King);
      if ( !kpos.valid() ) {
         return false;
      }
      if ( casts == 2 || casts == 3 ) {
         // right-castle
         auto pos = getCastPos(color_, casts == 3 ? 0 : 1);
         if ( pos.valid() ) {
            auto scolor = getColor(pos);
            auto stype = getFigure(pos);
            if ( scolor != color_ || stype != ChessFigure::Rook ) {
               return false;
            }
         }
         return move(kpos, pos); 
      }
      return false;
   }
   if ( type == ' ' ) {
     type = 'P';
   }
   if ( bcol == -1 || brow == -1 ) {
      return false;
   }
   Pos target(brow, bcol);
   if ( acol >= 0 && arow >= 0 ) {
      return move(Pos(arow, acol), target, (prom == ' ' ? ChessFigure::Queen : toFigure(prom))); 
   } else {
      if ( type == ' ' ) {
         return false;
      }
      for ( int row = 0; row < NUMBER_OF_ROWS; row++ ) {
         if ( arow == -1 || arow == row ) {
            for ( int col = 0; col < NUMBER_OF_COLS; col++ ) {
               if ( acol == -1 || acol == col ) {
                  auto source = Pos(row, col);
                  auto scolor = getColor(source);
                  auto stype = getFigure(source);
                  if ( scolor == color_ ) {
                     if ( toFigure(type) == stype ) {
                        if ( isMoveValid(source, target) ) {
                           return move(source, target, (prom == ' ' ? ChessFigure::Queen : toFigure(prom))); 
                        }
                     }
                  }
               }
            }
         }
      }
   }
   return false;
}

void
ChessBoard::applyMove(const Pos& from, const Pos& to, const ChessFigure promoteTo) {
   const auto stype = getFigure(from);
   const auto scolor = getColor(from);

   unsigned sofs = (scolor ? 0 : CASTS_SIDES);
   if ( stype == ChessFigure::King ) {
      casts_[sofs] = '-';
      casts_[sofs+1] = '-';
   } else if ( stype == ChessFigure::Rook ) {
      if ( getCastPos(sofs) == from ) {
         casts_[sofs] = '-';
      }
      if ( getCastPos(sofs+1) == from ) {
         casts_[sofs+1] = '-';
      }
   }

   const auto ttype = getFigure(to);
   const auto tcolor = getColor(to);
   if ( stype == ChessFigure::King && ttype == ChessFigure::Rook && scolor == tcolor ) { // castling
      set(from, false, ChessFigure::None);
      set(to,   false, ChessFigure::None);
      set(Pos(from.row, to.col < from.col ? LONG_CASTLE_KING : SHORT_CASTLE_KING), color_, ChessFigure::King);
      set(Pos(from.row, to.col < from.col ? LONG_CASTLE_ROOK : SHORT_CASTLE_ROOK), color_, ChessFigure::Rook);
   } else {
      set(from, false, ChessFigure::None);
      set(to, color_, isPromotion(from, to, stype) ? promoteTo : stype);
   }

   if ( isEnpassant(from, to, stype ) ) {
      set(to.towardCenter(), false, ChessFigure::None);
   }

   color_ = !color_;

   if ( color_ == WHITE ) {
      clocks_[FULL_CLOCK]++;
   }

   if ( stype == ChessFigure::Pawn || ttype != ChessFigure::None ) {
      clocks_[HALF_CLOCK] = 0;
   } else {
      clocks_[HALF_CLOCK]++;
   }

   enpassant_ = isFastPawn(from, to, stype) ? to.pcol() : '-';
}

bool
ChessBoard::move(const Pos& from, const Pos& to, const ChessFigure promoteTo) {
   if ( promoteTo == ChessFigure::Pawn || !isMoveValid(from, to) ) {
      return false;
   }
   applyMove(from, to, promoteTo);
   return true;
}

std::ostream& operator<<(std::ostream& os, const ChessBoard& board) {
   board.debugPrint(os);
   return os;
}

std::ostream& operator<<(std::ostream& os, const Pos& pos) {
   pos.debugPrint(os);
   return os;
}
