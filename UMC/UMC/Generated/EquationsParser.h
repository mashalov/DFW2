
// Generated from C:\Users\masha\source\repos\DFW2\UMC\UMC\Equations.g4 by ANTLR 4.8

#pragma once


#include "antlr4-runtime.h"




class  EquationsParser : public antlr4::Parser {
public:
  enum {
    FLOAT = 1, INTEGER = 2, MAIN = 3, INIT = 4, VARS = 5, CONST = 6, EXTERNAL = 7, 
    VARIABLE = 8, NEWLINE = 9, LINE_COMMENT = 10, EQUAL = 11, BAR = 12, 
    AND = 13, OR = 14, LB = 15, RB = 16, LCB = 17, RCB = 18, LSB = 19, RSB = 20, 
    DOT = 21, COMMA = 22, PLUS = 23, MINUS = 24, MUL = 25, DIV = 26, POW = 27, 
    HIGH = 28, LOW = 29, WS = 30
  };

  enum {
    RuleInput = 0, RuleMain = 1, RuleInit = 2, RuleVariables = 3, RuleVardefines = 4, 
    RuleEquationsys = 5, RuleVarlist = 6, RuleVardefineline = 7, RuleVardefine = 8, 
    RuleConstvardefine = 9, RuleExtvardefine = 10, RuleEquation = 11, RuleEquationline = 12, 
    RuleExpressionlist = 13, RuleIntlist = 14, RuleConstvalue = 15, RuleModlink = 16, 
    RuleModlinkbase = 17, RuleExpression = 18
  };

  EquationsParser(antlr4::TokenStream *input);
  ~EquationsParser();

  virtual std::string getGrammarFileName() const override;
  virtual const antlr4::atn::ATN& getATN() const override { return _atn; };
  virtual const std::vector<std::string>& getTokenNames() const override { return _tokenNames; }; // deprecated: use vocabulary instead.
  virtual const std::vector<std::string>& getRuleNames() const override;
  virtual antlr4::dfa::Vocabulary& getVocabulary() const override;


  class InputContext;
  class MainContext;
  class InitContext;
  class VariablesContext;
  class VardefinesContext;
  class EquationsysContext;
  class VarlistContext;
  class VardefinelineContext;
  class VardefineContext;
  class ConstvardefineContext;
  class ExtvardefineContext;
  class EquationContext;
  class EquationlineContext;
  class ExpressionlistContext;
  class IntlistContext;
  class ConstvalueContext;
  class ModlinkContext;
  class ModlinkbaseContext;
  class ExpressionContext; 

  class  InputContext : public antlr4::ParserRuleContext {
  public:
    InputContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    MainContext *main();
    antlr4::tree::TerminalNode *EOF();
    VariablesContext *variables();
    InitContext *init();
    std::vector<antlr4::tree::TerminalNode *> NEWLINE();
    antlr4::tree::TerminalNode* NEWLINE(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  InputContext* input();

  class  MainContext : public antlr4::ParserRuleContext {
  public:
    MainContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *MAIN();
    EquationsysContext *equationsys();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  MainContext* main();

  class  InitContext : public antlr4::ParserRuleContext {
  public:
    InitContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *INIT();
    EquationsysContext *equationsys();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  InitContext* init();

  class  VariablesContext : public antlr4::ParserRuleContext {
  public:
    VariablesContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *VARS();
    VardefinesContext *vardefines();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  VariablesContext* variables();

  class  VardefinesContext : public antlr4::ParserRuleContext {
  public:
    VardefinesContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LCB();
    antlr4::tree::TerminalNode *RCB();
    std::vector<VardefinelineContext *> vardefineline();
    VardefinelineContext* vardefineline(size_t i);
    std::vector<antlr4::tree::TerminalNode *> NEWLINE();
    antlr4::tree::TerminalNode* NEWLINE(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  VardefinesContext* vardefines();

  class  EquationsysContext : public antlr4::ParserRuleContext {
  public:
    EquationsysContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *LCB();
    antlr4::tree::TerminalNode *RCB();
    std::vector<EquationlineContext *> equationline();
    EquationlineContext* equationline(size_t i);
    std::vector<antlr4::tree::TerminalNode *> NEWLINE();
    antlr4::tree::TerminalNode* NEWLINE(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EquationsysContext* equationsys();

  class  VarlistContext : public antlr4::ParserRuleContext {
  public:
    VarlistContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> VARIABLE();
    antlr4::tree::TerminalNode* VARIABLE(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  VarlistContext* varlist();

  class  VardefinelineContext : public antlr4::ParserRuleContext {
  public:
    VardefinelineContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    VardefineContext *vardefine();
    std::vector<antlr4::tree::TerminalNode *> NEWLINE();
    antlr4::tree::TerminalNode* NEWLINE(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  VardefinelineContext* vardefineline();

  class  VardefineContext : public antlr4::ParserRuleContext {
  public:
    VardefineContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    ConstvardefineContext *constvardefine();
    ExtvardefineContext *extvardefine();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  VardefineContext* vardefine();

  class  ConstvardefineContext : public antlr4::ParserRuleContext {
  public:
    ConstvardefineContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *CONST();
    VarlistContext *varlist();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConstvardefineContext* constvardefine();

  class  ExtvardefineContext : public antlr4::ParserRuleContext {
  public:
    ExtvardefineContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *EXTERNAL();
    VarlistContext *varlist();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExtvardefineContext* extvardefine();

  class  EquationContext : public antlr4::ParserRuleContext {
  public:
    EquationsParser::ExpressionContext *left = nullptr;;
    EquationsParser::ExpressionContext *right = nullptr;;
    EquationContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *EQUAL();
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EquationContext* equation();

  class  EquationlineContext : public antlr4::ParserRuleContext {
  public:
    EquationlineContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    EquationContext *equation();
    std::vector<antlr4::tree::TerminalNode *> NEWLINE();
    antlr4::tree::TerminalNode* NEWLINE(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  EquationlineContext* equationline();

  class  ExpressionlistContext : public antlr4::ParserRuleContext {
  public:
    ExpressionlistContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ExpressionlistContext* expressionlist();

  class  IntlistContext : public antlr4::ParserRuleContext {
  public:
    IntlistContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> INTEGER();
    antlr4::tree::TerminalNode* INTEGER(size_t i);
    std::vector<antlr4::tree::TerminalNode *> COMMA();
    antlr4::tree::TerminalNode* COMMA(size_t i);


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  IntlistContext* intlist();

  class  ConstvalueContext : public antlr4::ParserRuleContext {
  public:
    ConstvalueContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *FLOAT();
    antlr4::tree::TerminalNode *INTEGER();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ConstvalueContext* constvalue();

  class  ModlinkContext : public antlr4::ParserRuleContext {
  public:
    ModlinkContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    std::vector<antlr4::tree::TerminalNode *> VARIABLE();
    antlr4::tree::TerminalNode* VARIABLE(size_t i);
    antlr4::tree::TerminalNode *LSB();
    IntlistContext *intlist();
    antlr4::tree::TerminalNode *RSB();
    antlr4::tree::TerminalNode *DOT();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ModlinkContext* modlink();

  class  ModlinkbaseContext : public antlr4::ParserRuleContext {
  public:
    ModlinkbaseContext(antlr4::ParserRuleContext *parent, size_t invokingState);
    virtual size_t getRuleIndex() const override;
    antlr4::tree::TerminalNode *BAR();
    ModlinkContext *modlink();


    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
   
  };

  ModlinkbaseContext* modlinkbase();

  class  ExpressionContext : public antlr4::ParserRuleContext {
  public:
    ExpressionContext(antlr4::ParserRuleContext *parent, size_t invokingState);
   
    ExpressionContext() = default;
    void copyFrom(ExpressionContext *context);
    using antlr4::ParserRuleContext::copyFrom;

    virtual size_t getRuleIndex() const override;

   
  };

  class  AndorContext : public ExpressionContext {
  public:
    AndorContext(ExpressionContext *ctx);

    EquationsParser::ExpressionContext *left = nullptr;
    antlr4::Token *op = nullptr;
    EquationsParser::ExpressionContext *right = nullptr;
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *OR();
    antlr4::tree::TerminalNode *AND();

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ModellinkContext : public ExpressionContext {
  public:
    ModellinkContext(ExpressionContext *ctx);

    ModlinkContext *modlink();

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  HighlowContext : public ExpressionContext {
  public:
    HighlowContext(ExpressionContext *ctx);

    EquationsParser::ExpressionContext *left = nullptr;
    antlr4::Token *op = nullptr;
    EquationsParser::ExpressionContext *right = nullptr;
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *HIGH();
    antlr4::tree::TerminalNode *LOW();

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  ModellinkbaseContext : public ExpressionContext {
  public:
    ModellinkbaseContext(ExpressionContext *ctx);

    ModlinkbaseContext *modlinkbase();

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  FunctionContext : public ExpressionContext {
  public:
    FunctionContext(ExpressionContext *ctx);

    antlr4::Token *func = nullptr;
    antlr4::tree::TerminalNode *LB();
    antlr4::tree::TerminalNode *RB();
    antlr4::tree::TerminalNode *VARIABLE();
    ExpressionlistContext *expressionlist();

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  VariableContext : public ExpressionContext {
  public:
    VariableContext(ExpressionContext *ctx);

    antlr4::tree::TerminalNode *VARIABLE();

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  PowContext : public ExpressionContext {
  public:
    PowContext(ExpressionContext *ctx);

    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *POW();

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  RealconstContext : public ExpressionContext {
  public:
    RealconstContext(ExpressionContext *ctx);

    ConstvalueContext *constvalue();

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  UnaryContext : public ExpressionContext {
  public:
    UnaryContext(ExpressionContext *ctx);

    antlr4::Token *op = nullptr;
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *PLUS();
    antlr4::tree::TerminalNode *MINUS();

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  InfixContext : public ExpressionContext {
  public:
    InfixContext(ExpressionContext *ctx);

    EquationsParser::ExpressionContext *left = nullptr;
    antlr4::Token *op = nullptr;
    EquationsParser::ExpressionContext *right = nullptr;
    std::vector<ExpressionContext *> expression();
    ExpressionContext* expression(size_t i);
    antlr4::tree::TerminalNode *MUL();
    antlr4::tree::TerminalNode *DIV();
    antlr4::tree::TerminalNode *PLUS();
    antlr4::tree::TerminalNode *MINUS();

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  class  BracesContext : public ExpressionContext {
  public:
    BracesContext(ExpressionContext *ctx);

    antlr4::tree::TerminalNode *LB();
    ExpressionContext *expression();
    antlr4::tree::TerminalNode *RB();

    virtual antlrcpp::Any accept(antlr4::tree::ParseTreeVisitor *visitor) override;
  };

  ExpressionContext* expression();
  ExpressionContext* expression(int precedence);

  virtual bool sempred(antlr4::RuleContext *_localctx, size_t ruleIndex, size_t predicateIndex) override;
  bool expressionSempred(ExpressionContext *_localctx, size_t predicateIndex);

private:
  static std::vector<antlr4::dfa::DFA> _decisionToDFA;
  static antlr4::atn::PredictionContextCache _sharedContextCache;
  static std::vector<std::string> _ruleNames;
  static std::vector<std::string> _tokenNames;

  static std::vector<std::string> _literalNames;
  static std::vector<std::string> _symbolicNames;
  static antlr4::dfa::Vocabulary _vocabulary;
  static antlr4::atn::ATN _atn;
  static std::vector<uint16_t> _serializedATN;


  struct Initializer {
    Initializer();
  };
  static Initializer _init;
};

