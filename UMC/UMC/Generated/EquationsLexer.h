
// Generated from C:\Users\masha\source\repos\DFW2\UMC\UMC\Equations.g4 by ANTLR 4.9.2

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
  ~EquationsLexer();

  virtual std::string getGrammarFileName() const override;
  virtual const std::vector<std::string>& getRuleNames() const override;

  virtual const std::vector<std::string>& getChannelNames() const override;
  virtual const std::vector<std::string>& getModeNames() const override;
  virtual const std::vector<std::string>& getTokenNames() const override; // deprecated, use vocabulary instead
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;

  virtual const std::vector<uint16_t> getSerializedATN() const override;
  virtual const antlr4::atn::ATN& getATN() const override;

private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;
  static std::vector<std::string> _channelNames;
  static std::vector<std::string> _modeNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

