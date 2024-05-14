#include "primitives.hpp"

#include <chrono>

const std::array<Pos, 4> PAWNDIRS = {Pos(1,-1), Pos(1,+1), Pos(1,0), Pos(2,0)};
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
   return !check(!color_);
}

bool
ChessBoard::isMoveValidInternal(const Pos& from, const Pos& to, const ChessFigure& stype, const ChessFigure& ttype) const {
   switch ( stype ) {
      case ChessFigure::Pawn:
         if ( to.sub(from).isDiagonal() ) {
            return abs(from.col - to.col) == 1
               && ( color_ ? from.row + 1 == to.row : from.row == to.row + 1 )
               && ( ttype != ChessFigure::None || isEnpassantTarget(to) );
         } else {
            return from.col == to.col
                && ttype == ChessFigure::None
                && ( color_
                ? from.row + 1 == to.row || ( from.row == FIRST_PAWN_ROW && from.row + 2 == to.row && getFigure(Pos(from.row+1,from.col)) == ChessFigure::None )
                : from.row == to.row + 1 || ( from.row == LAST_PAWN_ROW  && from.row == to.row + 2 && getFigure(Pos(to.row+1,to.col)) == ChessFigure::None ) );
         }
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
         return isMoveValidInternal(from, to, ChessFigure::Rook, ttype) || isMoveValidInternal(from, to, ChessFigure::Bishop, ttype);
      case ChessFigure::King:
         return abs(from.col - to.col) <= 1
             && abs(from.row - to.row) <= 1
             && !hasWatcher(!color_, to)
             && !getWatcherFromLine(!color_, from, from.sub(to)).valid();
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
ChessBoard::isMoveValid(const Pos& from, const Pos& to) const {
   if ( !from.valid() || !to.valid() || from == to ) {
      return false;
   }
   const auto scolor = getColor(from);
   const auto stype = getFigure(from);
   if ( stype == ChessFigure::None || scolor != color_ ) {
      return false;
   }
   if ( stype != ChessFigure::King && isPinned(from) && !to.sub(from).isInDir(from.sub(kings_[scolor]).dir()) ) {
      return false;
   }
   const auto tcolor = getColor(to);
   const auto ttype = getFigure(to);
   if ( ttype != ChessFigure::None && scolor == tcolor ) {
      return stype == ChessFigure::King && ttype == ChessFigure::Rook && isCastleValid(from, to);
   }
   return isMoveValidInternal(from, to, stype, ttype)
       && (stype == ChessFigure::King || !countWatchers(!color_, kings_[color_], 1, to));
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

unsigned char
ChessBoard::countWatchers(bool attackerColor, const Pos& pos, unsigned char maxval, const Pos& newBlocker) const {
   unsigned char retval = 0;
   if ( !pos.valid() ) {
      return retval;
   }

   // Knight
   for ( const auto& kdir: KNIGHTDIRS ) {
      auto tpos = pos.add(kdir);
      if ( newBlocker.valid() && tpos == newBlocker ) {
         continue;
      }
      if ( getFigure(tpos) == ChessFigure::Knight && getColor(tpos) == attackerColor ) {
         retval++;
         if ( retval >= maxval ) {
            return retval;
         }
      }
   }

   // Figures that are attacking through lines
   for ( const auto& dir : DIRS ) {
      Pos attacker = getWatcherFromLine(attackerColor, pos, dir);
      if ( attacker.valid() ) {
         if ( newBlocker.valid() && newBlocker.sub(pos).isInDir(dir) && attacker.sub(newBlocker).isInDir(dir) ) {
            continue;
         }
         retval++;
         if ( retval >= maxval ) {
            return retval;
         }
      }
   }

   return retval;
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

   if ( isEnpassantTarget(to) ) {
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

bool
ChessBoard::isMobilePiece(const Pos& pos, unsigned char check) const {
   const auto stype = getFigure(pos);
   const auto scolor = getColor(pos);
   if ( scolor != color_ ) {
      return false;
   }
   switch ( stype ) {
      case ChessFigure::Pawn:
         for ( const auto& dir : PAWNDIRS ) {
            if ( isMoveValid(pos, color_ ? pos.add(dir) : pos.sub(dir) ) ) {
               return true;
            }
         }
         return false;
      case ChessFigure::Knight:
         for ( const auto& dir : KNIGHTDIRS ) {
            if ( isMoveValid(pos, pos.add(dir)) ) {
               return true;
            }
         }
         return false;
      case ChessFigure::King:
         for ( unsigned i = 0; i < 2; i++ ) { // castles
            auto rpos = getCastPos(color_, i);
            if ( rpos.valid() ) {
               auto rcolor = getColor(rpos);
               auto rtype = getFigure(rpos);
               if ( rcolor == color_ || rtype == ChessFigure::Rook ) {
                  if ( isMoveValid(pos, rpos) ) {
                     return true;
                  }
               }
            }
         }
         for ( const auto& dir : DIRS ) {
            if ( isMoveValid(pos, pos.add(dir)) ) {
               return true;
            }
         }
         return false;
      case ChessFigure::Bishop:
      case ChessFigure::Rook:
      case ChessFigure::Queen:
         // if no check and Bishop, Rook or Queen can move multiple steps in a dir, it's enough to check just one move
         for ( const auto& dir : DIRS ) {
            Pos acc = pos.add(dir);
            if ( acc.valid() && isMoveValid(pos, acc) ) {
               return true;
            }
            if ( check ) {
               while ( acc.valid() && getFigure(acc) == ChessFigure::None ) {
                  acc.move(dir);
                  if ( isMoveValid(pos, acc) ) {
                     return true;
                  }
               }
            }
         }
         return false;
      default:
         return false;
   }
   return false;
}

void
ChessBoard::listMobilePieces(MiniPosVector& pawns, MiniPosVector& pieces) const {
   pawns.clear();
   pieces.clear();
   if ( valid() ) {
      // There ways to solve a check: a.) move with the king b.) block with another piece c.) capture the attacker
      unsigned int cktype = checkType(color_);
      if ( cktype == 2 ) { // double check: the king must move (even if it takes an attacker
         if ( isMobilePiece(kings_[color_], cktype) ) {
            push_back(pieces, kings_[color_]);
         }
      } else {
         for ( int row = 0; row < NUMBER_OF_ROWS; row++ ) {
            for ( int col = 0; col < NUMBER_OF_COLS; col++ ) {
               Pos pos(row, col); 
               auto pcolor = getColor(pos);
               auto ptype = getFigure(pos);
               if ( ptype != ChessFigure::None && pcolor == color_ ) {
                  if ( isMobilePiece(pos, cktype) ) {
                     if ( ptype == ChessFigure::Pawn ) {
                        push_back(pawns, pos);
                     } else {
                        push_back(pieces, pos);
                     }
                  }
               }
            }
         }
      }
   }
}

void
ChessBoard::debugPrint(std::ostream& os) const {
   if ( !valid() ) {
      os << "!!!INVALID!!!" << std::endl;
   }
   for ( int row = NUMBER_OF_ROWS - 1; row >= 0; row-- ) {
      os << " ";
      debugPrintRowSeparator(os);
      os << int(row+1);
      data_[row].debugPrint(os, BOARD_DRAW_COL_SEPARATOR);
      os <<std::endl;
   }
   os << " ";
   debugPrintRowSeparator(os);
   os << "  a b c d e f g h" << std::endl;
   os << std::endl;
   os << (color_ ? "w" : "b") << " /" << casts_[0] << casts_[1] << casts_[2] << casts_[3] << "/ " << enpassant_ << " " << unsigned(clocks_[FULL_CLOCK]) << "[" << unsigned(clocks_[HALF_CLOCK]) << "]" << std::endl;
   MiniPosVector pawns;
   MiniPosVector pieces;
   std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
   listMobilePieces(pawns, pieces);
   std::chrono::high_resolution_clock::time_point t2 = std::chrono::high_resolution_clock::now();
   std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(t2 - t1);
   std::cout << "It took me " << (time_span.count()*1000.0*1000.0) << " us";
   os << std::endl;
   os << "mp:" << pawns << std::endl;
   os << "mf:" << pieces << std::endl;
}

std::ostream& operator<<(std::ostream& os, const MiniPosVector& vec) {
   os << "{";
   bool first = true;
   for ( size_t i = 0; i < vec.size(); i++ ) {
      if ( !first ) {
         os << ", ";
      }
      os << get(vec, i);
      first = false;
   }
   os << "}";
   return os;
}

std::ostream& operator<<(std::ostream& os, const ChessBoard& board) {
   board.debugPrint(os);
   return os;
}

std::ostream& operator<<(std::ostream& os, const Pos& pos) {
   pos.debugPrint(os);
   return os;
}
