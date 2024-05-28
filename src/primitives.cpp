#include "primitives.hpp"

#include <chrono>
#include <valgrind/callgrind.h>

template <class T> inline T tabs(const T& v) { return v >= 0 ? v : -v; }
template <class T> inline T tsgn(const T& v) { return v ? ( v >= 0 ? +1 :-1 ) : 0; }

const Pos KNIGHT_FIRST_DIR(+1,+2);
const Pos KNIGHT_FIRST_SHIFT(+1,-1);

Pos
Pos::dir() const {
   if ( !row || !col || tabs(row) == tabs(col) ) {
      return Pos(tsgn(row), tsgn(col));
   }
   return NULLPOS;
}

bool
Pos::isInDir( const Pos& dir ) const {
   if ( null() ) {
      return false;
   }
   char z = dir.row ? row / dir.row : col / dir.col;
   return row == z * dir.row && col == z * dir.col && z > 0;
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
   casts_ = {CHAR_INVALID, CHAR_INVALID, CHAR_INVALID, CHAR_INVALID};
   std::array<unsigned, 2> cpos = {0, 0};
   for ( const auto& elem : casts ) {
      if ( elem == CHAR_INVALID ) {
         continue;
      }
      bool color = isupper(elem);
      if ( cpos[color] >= CASTS_SIDES ) {
         return false;
      }
      casts_[(color ? 0 : CASTS_SIDES) + cpos[color]] = elem;
      cpos[color]++;
   }
   enpassant_ = enpassant.size() >= 1 ? enpassant[0] : CHAR_INVALID;
   clocks_[HALF_CLOCK] = halfMoveClock;
   clocks_[FULL_CLOCK] = fullClock;
   kings_ = { find(BLACK, ChessFigure::King), find(WHITE, ChessFigure::King) };
   return validHeavy();
}

bool
ChessBoard::initFEN(const std::string& str) {
   std::string fen, white, casts, enpassant;
   unsigned halfMoveClock, fullClock;
   std::stringstream(str) >> fen >> white >> casts >> enpassant >> halfMoveClock >> fullClock;
   return initFEN(fen, white, casts, enpassant, halfMoveClock, fullClock);
}

bool
ChessBoard::validHeavy() const {
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
                ? from.row + 1 == to.row || ( from.row == FIRST_PAWN_ROW && from.row + 2 == to.row && isEmpty(Pos(from.row+1,from.col)) )
                : from.row == to.row + 1 || ( from.row == LAST_PAWN_ROW  && from.row == to.row + 2 && isEmpty(Pos(to.row+1,to.col)) ) );
         }
      case ChessFigure::Knight:
         return abs(from.row - to.row) * abs(from.col - to.col) == 2;
      case ChessFigure::Bishop:
      case ChessFigure::Rook:
      case ChessFigure::Queen:
         {
            Pos dir = to.sub(from).dir();
            if ( dir.null() || ( stype != ChessFigure::Queen && stype != dir.minorType() ) ) {
               return false;
            }
            Pos acc = from.add(dir);
            while ( !(acc == to) ) {
               if ( !isEmpty(acc) ) {
                  return false;
               }
               acc.move(dir);
            }
         }
         return true;
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
      if ( (!(lpos == to) && !(lpos == from) && !isEmpty(lpos)) || (king && hasWatcher(!color_, lpos)) ) {
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
ChessBoard::isMoveValid(const Pos& from, const Pos& to, bool pinned, unsigned char checkDanger) const {
   if ( !from.valid() || !to.valid() || from == to ) {
      return false;
   }
   const auto ssq = getSquare(from);
   const ChessFigure stype = static_cast<ChessFigure>(ssq >> 1);
   const bool scolor = (ssq & 1);
   if ( stype == ChessFigure::None || scolor != color_ ) {
      return false;
   }
   if ( pinned && !to.sub(from).isInDir(from.sub(kings_[scolor]).dir()) ) {
      return false;
   }
   const auto tsq = getSquare(to);
   const ChessFigure ttype = static_cast<ChessFigure>(tsq >> 1);
   const bool tcolor = (tsq & 1);
   if ( ttype != ChessFigure::None && tcolor == color_ ) {
      return stype == ChessFigure::King && ttype == ChessFigure::Rook && isCastleValid(from, to);
   }
   return isMoveValidInternal(from, to, stype, ttype)
       && (stype == ChessFigure::King || !checkDanger || !countWatchers(!color_, kings_[color_], 1, to));
}
Pos
ChessBoard::getPieceFromLine(const Pos& pos, const Pos& dir) const {
   Pos acc = pos.add(dir);

   // Bishop, Rook, Queen
   while ( acc.valid() && isEmpty(acc) ) {
      acc.move(dir);
   }
   return acc;
}

Pos
ChessBoard::getWatcherFromLine(bool attackerColor, const Pos& pos, const Pos& dir) const {
   Pos acc = getPieceFromLine(pos, dir);
   if ( acc.valid() ) {
      if ( getColor(acc) == attackerColor ) {
         auto atype = getFigure(acc);
         if ( atype == ChessFigure::Queen || atype == dir.minorType() ) {
            return acc;
         }
         if ( pos.add(dir) == acc ) {
            if ( atype == ChessFigure::King || (atype == ChessFigure::Pawn && dir.isPawnDir(attackerColor)) ) {
               return acc;
            }
         }
      }
   }
   return INVALID;
}

unsigned char
ChessBoard::countWatchers(bool attackerColor, const Pos& pos, unsigned char maxval, const Pos& newBlocker, Pos& attackerPos) const {
   unsigned char retval = 0;
   if ( !pos.valid() ) {
      return retval;
   }

   // Knight
   Pos kpos = color_ ? pos.add(KNIGHT_FIRST_DIR) : pos.sub(KNIGHT_FIRST_DIR);
   Pos kshift = color_ ? KNIGHT_FIRST_SHIFT : KNIGHT_FIRST_SHIFT.neg();
   for ( size_t i = 0; i < 8; i ++ ) {
      if ( getFigure(kpos) == ChessFigure::Knight && getColor(kpos) == attackerColor && !( newBlocker.valid() && kpos == newBlocker ) ) {
         attackerPos = kpos;
         if ( ++retval >= maxval ) {
            return retval;
         }
      }
      kpos.move(kshift);
      kshift.knightShiftRot();
   }

   // Figures that are attacking through lines
   Pos dir;
   for ( dir.row = -1; dir.row <= +1; dir.row++ ) {
      for ( dir.col = -1; dir.col <= +1; dir.col++ ) {
         if ( !dir.row && !dir.col ) {
            continue;
         }
         Pos attacker = getWatcherFromLine(attackerColor, pos, dir);
         if ( attacker.valid() ) {
            if ( newBlocker.valid() && ( newBlocker == attacker || ( newBlocker.sub(pos).isInDir(dir) && attacker.sub(newBlocker).isInDir(dir) ) ) ) {
               continue;
            }
            attackerPos = attacker;
            if ( ++retval >= maxval ) {
               return retval;
            }
         }
      }
   }

   return retval;
}

unsigned char
ChessBoard::countWatchers(const bool color, const Pos& pos, unsigned char maxval, const Pos& newBlocker) const {
   Pos attacker = INVALID;
   return countWatchers(color, pos, maxval, newBlocker, attacker);
}

bool
ChessBoard::isPinned(const Pos& pos) const {
   Pos dir = pos.sub(kings_[color_]).dir();
   if ( dir.null() ) {
      return false;
   }
   Pos ipos = getPieceFromLine(kings_[color_], dir);
   if ( !(ipos == pos) ) {
      return false;
   }
   Pos wpos = getWatcherFromLine(!color_, pos, dir);
   if ( !wpos.valid() ) {
      return false;
   }
   const auto wcolor = getColor(wpos);
   const auto wtype = getFigure(wpos);
   return wcolor != color_ && ( wtype == ChessFigure::Queen || wtype == (dir.isAxialDir() ? ChessFigure::Rook : ChessFigure::Bishop) );
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
   const auto ssq = getSquare(from);
   const ChessFigure stype = static_cast<ChessFigure>(ssq >> 1);
   const bool scolor = (ssq & 1);

   unsigned sofs = (scolor ? 0 : CASTS_SIDES);
   if ( stype == ChessFigure::King ) {
      casts_[sofs] = CHAR_INVALID;
      casts_[sofs+1] = CHAR_INVALID;
   } else if ( stype == ChessFigure::Rook ) {
      if ( getCastPos(sofs) == from ) {
         casts_[sofs] = CHAR_INVALID;
      }
      if ( getCastPos(sofs+1) == from ) {
         casts_[sofs+1] = CHAR_INVALID;
      }
   }

   const auto tsq = getSquare(to);
   const ChessFigure ttype = static_cast<ChessFigure>(tsq >> 1);
   const bool tcolor = (tsq & 1);
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

   enpassant_ = isFastPawn(from, to, stype) ? to.pcol() : CHAR_INVALID;
}

bool
ChessBoard::move(const Pos& from, const Pos& to, const ChessFigure promoteTo) {
   if ( promoteTo == ChessFigure::Pawn || !isMoveValid(from, to) ) {
      return false;
   }
   applyMove(from, to, promoteTo);
   return true;
}

Pos
intersect(const Pos& pos, const Pos& dir, const Pos& king, const Pos& checker) {
   auto cdir = checker.sub(king).dir();
   auto r = checker.sub(pos);

   char det = dir.row * cdir.col - dir.col * cdir.row;
   if ( det == 0 ) {
      return checker;  // if the defender is moving along the line of the check, blocking is not possible
   }

   char biga = (r.col * -cdir.row + r.row * cdir.col);
   if ( biga % det != 0 ) {
      return checker; // not a square on the checkerboard
   }
   char aval = biga / det;

   return pos.add(dir.mul(aval));
}

bool
ChessBoard::isMobilePiece(const Pos& pos, const ChessFigure& stype, unsigned char check, const Pos& checker) const {
   bool pinned = stype != ChessFigure::King && isPinned(pos);
   bool easy = !pinned && !check;
   switch ( stype ) {
      case ChessFigure::Pawn:
         {
            Pos dir(1, 0); // if moving +2 is possible, then moving +1 is also possible
            if ( easy && isEmpty(color_ ? pos.add(dir) : pos.sub(dir)) ) { // optimization only
               return true;
            }
            for ( dir.col = -1; dir.col <= 1; dir.col++ ) {
               if ( isMoveValid(pos, color_ ? pos.add(dir) : pos.sub(dir), pinned, check) ) {
                  return true;
               }
            }
         }
         return false;
      case ChessFigure::Knight:
         {
            if ( !pinned ) {
               Pos kpos = color_ ? pos.add(KNIGHT_FIRST_DIR) : pos.sub(KNIGHT_FIRST_DIR);
               Pos kshift = color_ ? KNIGHT_FIRST_SHIFT : KNIGHT_FIRST_SHIFT.neg();
               for ( size_t i = 0; i < 8; i ++ ) {
                  if ( kpos.valid() && isEmpty(kpos) ) {
                     return true;
                  }
                  kpos.move(kshift);
                  kshift.knightShiftRot();
               }
            }
         }
         return false;
      case ChessFigure::King:
         {
            Pos dir;
            for ( dir.row = -1; dir.row <= +1; dir.row++ ) {
               for ( dir.col = -1; dir.col <= +1; dir.col++ ) {
                  if ( !dir.row && !dir.col ) {
                     continue;
                  }
                  if ( isMoveValid(pos, pos.add(dir), false, check) ) {
                     return true;
                  }
               }
            }
         }
         if ( !check ) {
            for ( unsigned i = 0; i < 2; i++ ) { // castles
               auto rpos = getCastPos(color_, i);
               if ( rpos.valid() && isMoveValid(pos, rpos, false, check) ) {
                  return true;
               }
            }
         }
         return false;
      case ChessFigure::Bishop:
      case ChessFigure::Rook:
      case ChessFigure::Queen:
         // if no check and Bishop, Rook or Queen can move multiple steps in a dir, it's enough to check just one move
         {
            Pos dir;
            for ( dir.row = -1; dir.row <= +1; dir.row++ ) {
               for ( dir.col = -1; dir.col <= +1; dir.col++ ) {
                  if ( !dir.row && !dir.col ) {
                     continue;
                  }
                  if ( ( stype == ChessFigure::Rook && !dir.isAxialDir() ) || ( stype == ChessFigure::Bishop && dir.isAxialDir() ) ) {
                     continue;
                  }
                  Pos test = check ? intersect(pos, dir, kings_[color_], checker) : pos.add(dir);
                  if ( test.valid() && ( easy ? ( isEmpty(test) || getColor(test) != color_ ) : isMoveValid(pos, test, pinned, check) ) ) {
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
      Pos checker;
      unsigned int cktype = getChecker(color_, checker);
      if ( cktype == 2 ) { // double check: the king must move / take
         if ( isMobilePiece(kings_[color_], ChessFigure::King, cktype, checker) ) {
            push_back(pieces, kings_[color_]);
         }
      } else {
         Pos pos;
         for ( pos.row = 0; pos.row < NUMBER_OF_ROWS; pos.row++ ) {
            for ( pos.col = 0; pos.col < NUMBER_OF_COLS; pos.col++ ) {
               auto ptype = getFigure(pos);
               if ( ptype != ChessFigure::None && getColor(pos) == color_ ) {
                  if ( isMobilePiece(pos, ptype, cktype, checker) ) {
                     push_back(ptype == ChessFigure::Pawn ? pawns : pieces, pos);
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
      return;
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
   CALLGRIND_START_INSTRUMENTATION;
   listMobilePieces(pawns, pieces);
   CALLGRIND_STOP_INSTRUMENTATION;
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
