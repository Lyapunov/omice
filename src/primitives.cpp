#include "primitives.hpp"

bool
ChessBoard::isJumpValid(const Pos& from, const Pos& to, const ChessFigureType& stype ) const {
   switch ( stype ) {
      case ChessFigureType::Pawn:
         if ( from.col != to.col ) {
            return false;
         }
         if ( !( color_
                 ? from.row + 1 == to.row || ( from.row == FIRST_PAWN_ROW && from.row + 2 == to.row )
                 : from.row == to.row + 1 || ( from.row == LAST_PAWN_ROW  && from.row == to.row + 2 ) ) ) {
            return false;
         }
         return true;
      case ChessFigureType::Knight:
         return (abs(from.row - to.row) == 1 && abs(from.col - to.col) == 2)
             || (abs(from.row - to.row) == 2 && abs(from.col - to.col) == 1);
      case ChessFigureType::Bishop:
         if ( abs(from.row - to.row) != abs(from.col - to.col) ) {
            return false;
         }
         {
            int dx = to.row > from.row ? +1 : -1;
            int dy = to.col > from.col ? +1 : -1;
            Pos acc = from;
            acc.move(dx,dy);
            while ( !(acc == to) ) {
               if ( get(acc) != ChessFigure::None ) {
                  return false;
               }
               acc.move(dx,dy);
            }
         }
         return true;
      case ChessFigureType::Rook:
         if ( !(from.row == to.row || from.col == to.col ) ) {
            return false;
         }
         {
            int dx = to.row > from.row ? +1 : (to.row == from.row ? 0 : -1);
            int dy = to.col > from.col ? +1 : (to.col == from.col ? 0 : -1);
            Pos acc = from;
            acc.move(dx,dy);
            while ( !(acc == to) ) {
               if ( get(acc) != ChessFigure::None ) {
                  return false;
               }
               acc.move(dx,dy);
            }
         }
         return true;
      case ChessFigureType::Queen:
         return isJumpValid(from, to, ChessFigureType::Rook) || isJumpValid(from, to, ChessFigureType::Bishop);
      case ChessFigureType::King:
         if ( abs(from.col - to.col) <= 1 ) {
            return false;
         }
         if ( abs(from.row - to.row) <= 1 ) {
            return false;
         }
         // DON'T STEP INTO CHESS
         return true;
      default:
         return false;
   }
}

bool
ChessBoard::isTakeValid(const Pos& from, const Pos& to, const ChessFigureType& stype ) const {
   switch ( stype ) {
      case ChessFigureType::Pawn:
         if ( abs(from.col - to.col) != 1 ) {
            return false;
         }
         if ( !( color_ ? from.row + 1 == to.row : from.row == to.row + 1 ) ) {
            return false;
         }
         break;
      default:
         return isJumpValid(from, to, stype);
   }
   return true;
}

bool
ChessBoard::isMoveValid(const Pos& from, const Pos& to) const {
   if ( from == to ) {
      return false;
   }
   const auto source = get(from);
   if ( source == ChessFigure::None || !hasColor(source, color_) ) {
      return false;
   }
   const auto stype = toType(source);
   const auto target = get(to);
   if ( !( (target != ChessFigure::None || isEmpassant(from, to, stype))
           ? isTakeValid(from, to, stype)
           : isJumpValid(from, to, stype) ) ) {
      return false;
   }
   return true;
}

bool
ChessBoard::move(const std::string& desc) {
   // format: nf6, Nf6, Ng8f6, g8f6, g8, g8=Q, g8=N ...
   char type = ' ';
   char prom = ' ';
   int acol = -1;
   int arow = -1;
   int bcol = -1;
   int brow = -1;
   for ( const auto& chr : desc ) {
      if ( isFigureChar(chr) ) {
         if ( type == ' ' ) {
            type = toupper(chr);
         } else if ( prom == ' ' ) {
            prom = toupper(chr);
         } else {
            return false;
         }
      }
      if ( chr >= '1' && chr <= '9' ) {
         if ( brow == -1 )  {
            brow = chr - '1';
         } else if ( arow == -1 ) {
            arow = brow;
            brow = chr - '1';
         } else {
            return false;
         }
      }
      if ( chr >= 'a' && chr <= 'h' ) {
         if ( bcol == -1 )  {
            bcol = chr - 'a';
         } else if ( acol == -1 ) {
            acol = bcol;
            bcol = chr - 'a';
         } else {
            return false;
         }
      }
      if ( chr == '=' ) {
         if ( type == ' ' ) {
           type = 'P';
         }
      }
   }
   if ( type == ' ' ) {
     type = 'P';
   }
   if ( bcol == -1 || brow == -1 ) {
      return false;
   }
   Pos target(brow, bcol);
   if ( acol >= 0 && arow >= 0 ) {
      Pos source(arow, acol);
      return move(source, target, (prom == ' ' ? ChessFigureType::Queen : toType(prom))); 
   } else {
      if ( type == ' ' ) {
         return false;
      }
      for ( int row = 0; row < static_cast<int>(NUMBER_OF_ROWS); row++ ) {
         if ( arow == -1 || arow == row ) {
            for ( int col = 0; col < static_cast<int>(NUMBER_OF_COLS); col++ ) {
               if ( acol == -1 || acol == col ) {
                  auto source = Pos(row, col);
                  auto sval = get(Pos(row, col));
                  if ( hasColor(sval, color_) ) {
                     if ( toType(type) == toType(sval) ) {
                        if ( isMoveValid(source, target) ) {
                           return move(source, target, (prom == ' ' ? ChessFigureType::Queen : toType(prom))); 
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
ChessBoard::doMove(const Pos& from, const Pos& to, const ChessFigureType promoteTo) {
   const auto source = get(from);

   set(from, ChessFigure::None);
   if ( isPromotion(from, to, source) ) {
      set(to, static_cast<ChessFigure>(static_cast<int>(color_ ? ChessFigure::WhitePawn : ChessFigure::BlackPawn)-1+static_cast<int>(promoteTo)));
   } else {
      set(to, source);
   }

   color_ = !color_;

   enpassant_ = isFastPawn(from, to, source) ? static_cast<char>('a' + to.col) : '-';
}

bool
ChessBoard::move(const Pos& from, const Pos& to, const ChessFigureType promoteTo) {
   if ( promoteTo == ChessFigureType::Pawn ) {
      return false;
   }
   if ( !isMoveValid(from, to) ) {
      return false;
   }
   doMove(from, to, promoteTo);
   return true;
}
