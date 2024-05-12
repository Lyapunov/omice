#include <cctype>
#include <iostream>
#include <fstream>
#include <map>
#include <string>
#include <vector>

#include "primitives.hpp"

int main(int argc, char* argv[]) {
   // INPUT FILE PROCESSOR MODE
   if ( argc >= 3 && std::string(argv[1]) == "input" ) {
      std::map<std::string, ChessBoard> boards;
      std::map<std::string, std::vector<std::string>> variants;
      {
         ChessBoard board;
         std::vector<std::string> variant;
         bool valid = true;

         auto fname = std::string(argv[2]);
         std::ifstream ifs(argv[2]);
         std::string line;
         bool tag = false;
         std::string tagtext;
         bool num = false;
         std::string numtext;
         bool tok = false;
         std::string toktext;
         bool fen = false;
         std::string fentext;

         while ( getline(ifs, line) ) {
            if ( num ) {
               if ( valid && int(variant.size()) != (std::stoi(numtext)-1)*2 ) {
                  std::cout << "ERROR: " << tagtext << " bad number " << numtext << " vs. " << variant.size() << std::endl;
                  valid = false;
               }
               num = false;
            }
            if ( tok ) {
               variant.push_back(toktext);
               if ( valid && !board.move(toktext) ) {
                  std::cout << "ERROR: " << tagtext << " cannot apply move " << toktext << std::endl;
                  valid = false;
               }
               if ( !board.valid() ) {
                  std::cout << "ERROR: " << tagtext << " move " << toktext << " led to failure" << std::endl;
                  valid = false;
               }
               tok = false;
            }
            auto tpos = line.find('#');
            if ( tpos != std::string::npos ) {
               line = line.substr(0, tpos);
            }
            for ( const auto& elem : line ) {
               if ( fen ) {
                  if ( elem == '}' ) {
                     fen = false;
                     board.initFEN(fentext);
                  } else {
                     fentext += elem;
                  }
               } else if ( tag ) {
                  if ( elem == ')' ) {
                     tag = false;
                  } else {
                     tagtext += elem;
                  }
               } else if (num) {
                  if ( elem >= '0' && elem <= '9' ) {
                     numtext += elem;
                  } else {
                     if ( valid && int(variant.size()) != (std::stoi(numtext)-1)*2 ) {
                        std::cout << "ERROR: " << tagtext << " bad number " << numtext << " vs. " << variant.size() << std::endl;
                        valid = false;
                     }
                     num = false;
                  }
               } else if (tok) {
                  if ( isspace(elem) ) {
                     variant.push_back(toktext);
                     if ( valid && !board.move(toktext) ) {
                        std::cout << "ERROR: " << tagtext << " cannot apply move " << toktext << std::endl;
                        valid = false;
                     }
                     if ( !board.valid() ) {
                        std::cout << "ERROR: " << tagtext << " move " << toktext << " led to failure" << std::endl;
                        valid = false;
                     }
                     tok = false;
                  } else {
                     toktext += elem;
                  }
               } else {
                  if ( isspace(elem) ) {
                  } else if ( elem == '(' ) {
                     if ( !tagtext.empty() && valid ) {
                        boards[tagtext] = board;
                        variants[tagtext] = variant;
                     }
                     valid = true;
                     tag = true;
                     tagtext = "";
                     board.init();
                     variant.clear();
                  } else if ( elem == '{' ) {
                     fen = true;
                     fentext = "";
                  } else if ( elem >= '0' && elem <= '9' ) {
                     num = true;
                     numtext = elem;
                  } else {
                     tok = true;
                     toktext = elem;
                  }
               }
            }
         }
         if ( !tagtext.empty() && valid ) {
            boards[tagtext] = board;
            variants[tagtext] = variant;
         }
         for ( const auto& elem : boards ) {
            std::cout << "=== " << elem.first << std::endl;
            std::cout << elem.second << std::endl;
         }
      }
      return 0;
   }

   return 0;
}
