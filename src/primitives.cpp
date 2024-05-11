#include "primitives.hpp"

const std::array<Pos, 8> KNIGHTDIRS = {Pos(1,2), Pos(-1,2), Pos(1,-2), Pos(-1,-2), Pos(2,1), Pos(-2,1), Pos(2,-1), Pos(-2,-1)};

bool
ChessBoard::isStepValid(const Pos& from, const Pos& to, const ChessFigure& stype ) const {
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
            int dr = to.row > from.row ? +1 : -1;
            int dc = to.col > from.col ? +1 : -1;
            Pos acc = from;
            acc.move(dr,dc);
            while ( !(acc == to) ) {
               if ( getFigure(acc) != ChessFigure::None ) {
                  return false;
               }
               acc.move(dr,dc);
            }
         }
         return true;
      case ChessFigure::Rook:
         if ( from.row != to.row && from.col != to.col ) {
            return false;
         }
         {
            int dr = to.row > from.row ? +1 : (to.row == from.row ? 0 : -1);
            int dc = to.col > from.col ? +1 : (to.col == from.col ? 0 : -1);
            Pos acc = from;
            acc.move(dr,dc);
            while ( !(acc == to) ) {
               if ( getFigure(acc) != ChessFigure::None ) {
                  return false;
               }
               acc.move(dr,dc);
            }
         }
         return true;
      case ChessFigure::Queen:
         return isStepValid(from, to, ChessFigure::Rook) || isStepValid(from, to, ChessFigure::Bishop);
      case ChessFigure::King:
         return abs(from.col - to.col) <= 1 && abs(from.row - to.row) <= 1 && !isInAttackLine(!color_, to);
      default:
         return false;
   }
}

bool
ChessBoard::testCastleWalk(const Pos& from, const Pos& to, int row, int source, int target, bool king) const {
   for ( int lcol = std::min(source, target); lcol <= std::max(source, target); lcol++ ) {
      // is vacant?
      Pos lpos(row, lcol);
      if ( (!(lpos == to) && !(lpos == from) && getFigure(lpos) != ChessFigure::None) || (king && isInAttackLine(!color_, lpos) ) ) {
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
   const auto tcolor = getColor(to);
   const auto ttype = getFigure(to);
   if ( ttype != ChessFigure::None && scolor == tcolor ) {
      return stype == ChessFigure::King && ttype == ChessFigure::Rook && isCastleValid(from, to);
   }
   return (ttype != ChessFigure::None || isEnpassant(from, to, stype))
          ? isTakeValid(from, to, stype)
          : isStepValid(from, to, stype);
}

bool
ChessBoard::isInAttackLine(bool attackerColor, const Pos& pos) const {
   if ( !pos.valid() ) {
      return false;
   }
   // Pawn
   for ( const auto& pdir: PAWNDIRS ) {
      auto tpos = pos.offset(Pos(attackerColor ? -1 : +1, pdir));
      if ( getFigure(tpos) == ChessFigure::Pawn && getColor(tpos) == attackerColor ) {
         return true;
      }
   }

   // Knight
   for ( const auto& kdir: KNIGHTDIRS ) {
      auto tpos = pos.offset(kdir);
      if ( getFigure(tpos) == ChessFigure::Knight && getColor(tpos) == attackerColor ) {
         return true;
      }
   }

   for ( int dr = -1; dr <= 1; dr++ ) {
      for ( int dc = -1; dc <= 1; dc++ ) {
         if ( !dr && !dc ) {
            continue;
         }
         Pos acc = pos;
         acc.move(dr, dc);

         // King
         if ( getFigure(acc) == ChessFigure::King && getColor(acc) == attackerColor ) {
            return true;
         }

         auto minorType = dr && dc ? ChessFigure::Bishop : ChessFigure::Rook;

         // Bishop, Rook, Queen
         while ( acc.valid() && getFigure(acc) == ChessFigure::None ) {
            acc.move(dr, dc);
         }
         if ( acc.valid() && getColor(acc) == attackerColor ) {
            auto atype = getFigure(acc);
            if ( atype == ChessFigure::Queen || atype == minorType ) {
               return true;
            }
         }
      }
   }

   return false;
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
      for ( int row = 0; row < static_cast<int>(NUMBER_OF_ROWS); row++ ) {
         if ( arow == -1 || arow == row ) {
            for ( int col = 0; col < static_cast<int>(NUMBER_OF_COLS); col++ ) {
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
ChessBoard::doMove(const Pos& from, const Pos& to, const ChessFigure promoteTo) {
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
      if ( to.col < from.col ) {
         // long castle
         set(Pos(from.row, LONG_CASTLE_KING), color_, ChessFigure::King);
         set(Pos(from.row, LONG_CASTLE_ROOK), color_, ChessFigure::Rook);
      } else {
         // short castle
         set(Pos(from.row, SHORT_CASTLE_KING), color_, ChessFigure::King);
         set(Pos(from.row, SHORT_CASTLE_ROOK), color_, ChessFigure::Rook);
      }
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

   enpassant_ = isFastPawn(from, to, stype) ? static_cast<char>('a' + to.col) : '-';
}

bool
ChessBoard::move(const Pos& from, const Pos& to, const ChessFigure promoteTo) {
   if ( promoteTo == ChessFigure::Pawn ) {
      return false;
   }
   if ( !isMoveValid(from, to) ) {
      return false;
   }
   doMove(from, to, promoteTo);
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
