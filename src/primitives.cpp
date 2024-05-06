#include "primitives.hpp"

bool
ChessBoard::isJumpValid(const Pos& from, const Pos& to, const ChessFigure& stype ) const {
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
            int dx = to.row > from.row ? +1 : -1;
            int dy = to.col > from.col ? +1 : -1;
            Pos acc = from;
            acc.move(dx,dy);
            while ( !(acc == to) ) {
               if ( getFigure(acc) != ChessFigure::None ) {
                  return false;
               }
               acc.move(dx,dy);
            }
         }
         return true;
      case ChessFigure::Rook:
         if ( !(from.row == to.row || from.col == to.col ) ) {
            return false;
         }
         {
            int dx = to.row > from.row ? +1 : (to.row == from.row ? 0 : -1);
            int dy = to.col > from.col ? +1 : (to.col == from.col ? 0 : -1);
            Pos acc = from;
            acc.move(dx,dy);
            while ( !(acc == to) ) {
               if ( getFigure(acc) != ChessFigure::None ) {
                  return false;
               }
               acc.move(dx,dy);
            }
         }
         return true;
      case ChessFigure::Queen:
         return isJumpValid(from, to, ChessFigure::Rook) || isJumpValid(from, to, ChessFigure::Bishop);
      case ChessFigure::King:
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
ChessBoard::isTakeValid(const Pos& from, const Pos& to, const ChessFigure& stype ) const {
   switch ( stype ) {
      case ChessFigure::Pawn:
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
   const auto scolor = getColor(from);
   const auto stype = getFigure(from);
   if ( stype == ChessFigure::None || scolor != color_ ) {
      return false;
   }
   const auto tcolor = getColor(to);
   const auto ttype = getFigure(to);
   if ( ttype != ChessFigure::None && scolor == tcolor ) {
      return false;
   }
   return (ttype != ChessFigure::None || isEmpassant(from, to, stype))
          ? isTakeValid(from, to, stype)
          : isJumpValid(from, to, stype);
}

static bool
IsFigureChar(char chr) {
   if ( chr == ' ' ) {
      return false;
   }
   static const std::string FIGURE_CONVERTER_SAFE = "pnrqkPNBRQK";
   auto fpos = FIGURE_CONVERTER_SAFE.find(chr);
   if ( fpos == std::string::npos ) {
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
      if ( IsFigureChar(chr) ) {
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

   set(from, false, ChessFigure::None);
   set(to, color_, isPromotion(from, to, stype) ? promoteTo : stype);

   color_ = !color_;

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
