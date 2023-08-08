
// Generated from /home/eugene/projects/DFW2/UMC/UMC/Equations.g4 by ANTLR 4.13.0

#pragma once


#include "antlr4-runtime.h"




class  EquationsLexer : public antlr4::Lexer {
public:
  enum {
    FLOAT = 1, INTEGER = 2, MAIN = 3, INIT = 4, VARS = 5, CONST = 6, EXTERNAL = 7, 
    VARIABLE = 8, NEWLINE = 9, LINE_COMMENT = 10, EQUAL = 11, BAR = 12, 
    AND = 13, OR = 14, LB = 15, RB = 16, LCB = 17, RCB = 18, LSB = 19, RSB = 20, 
    DOT = 21, COMMA = 22, PLUS = 23, MINUS = 24, MUL = 25, DIV = 26, POW = 27, 
    HIGH = 28, LOW = 29, WS = 30
  };

  explicit EquationsLexer(antlr4::CharStream *input);

  ~EquationsLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

