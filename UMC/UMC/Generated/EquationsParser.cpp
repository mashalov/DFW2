
// Generated from C:\Users\masha\source\repos\DFW2\UMC\UMC\Equations.g4 by ANTLR 4.8


#include "EquationsVisitor.h"

#include "EquationsParser.h"


using namespace antlrcpp;
using namespace antlr4;

EquationsParser::EquationsParser(TokenStream *input) : Parser(input) {
  _interpreter = new atn::ParserATNSimulator(this, _atn, _decisionToDFA, _sharedContextCache);
}

EquationsParser::~EquationsParser() {
  delete _interpreter;
}

std::string EquationsParser::getGrammarFileName() const {
  return "Equations.g4";
}

const std::vector<std::string>& EquationsParser::getRuleNames() const {
  return _ruleNames;
}

dfa::Vocabulary& EquationsParser::getVocabulary() const {
  return _vocabulary;
}


//----------------- InputContext ------------------------------------------------------------------

EquationsParser::InputContext::InputContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

EquationsParser::MainContext* EquationsParser::InputContext::main() {
  return getRuleContext<EquationsParser::MainContext>(0);
}

tree::TerminalNode* EquationsParser::InputContext::EOF() {
  return getToken(EquationsParser::EOF, 0);
}

EquationsParser::VariablesContext* EquationsParser::InputContext::variables() {
  return getRuleContext<EquationsParser::VariablesContext>(0);
}

EquationsParser::InitContext* EquationsParser::InputContext::init() {
  return getRuleContext<EquationsParser::InitContext>(0);
}

std::vector<tree::TerminalNode *> EquationsParser::InputContext::NEWLINE() {
  return getTokens(EquationsParser::NEWLINE);
}

tree::TerminalNode* EquationsParser::InputContext::NEWLINE(size_t i) {
  return getToken(EquationsParser::NEWLINE, i);
}


size_t EquationsParser::InputContext::getRuleIndex() const {
  return EquationsParser::RuleInput;
}


antlrcpp::Any EquationsParser::InputContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitInput(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::InputContext* EquationsParser::input() {
  InputContext *_localctx = _tracker.createInstance<InputContext>(_ctx, getState());
  enterRule(_localctx, 0, EquationsParser::RuleInput);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(39);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == EquationsParser::VARS) {
      setState(38);
      variables();
    }
    setState(41);
    main();
    setState(43);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == EquationsParser::INIT) {
      setState(42);
      init();
    }
    setState(48);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == EquationsParser::NEWLINE) {
      setState(45);
      match(EquationsParser::NEWLINE);
      setState(50);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(51);
    match(EquationsParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MainContext ------------------------------------------------------------------

EquationsParser::MainContext::MainContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* EquationsParser::MainContext::MAIN() {
  return getToken(EquationsParser::MAIN, 0);
}

EquationsParser::EquationsysContext* EquationsParser::MainContext::equationsys() {
  return getRuleContext<EquationsParser::EquationsysContext>(0);
}


size_t EquationsParser::MainContext::getRuleIndex() const {
  return EquationsParser::RuleMain;
}


antlrcpp::Any EquationsParser::MainContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitMain(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::MainContext* EquationsParser::main() {
  MainContext *_localctx = _tracker.createInstance<MainContext>(_ctx, getState());
  enterRule(_localctx, 2, EquationsParser::RuleMain);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(53);
    match(EquationsParser::MAIN);
    setState(54);
    equationsys();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- InitContext ------------------------------------------------------------------

EquationsParser::InitContext::InitContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* EquationsParser::InitContext::INIT() {
  return getToken(EquationsParser::INIT, 0);
}

EquationsParser::EquationsysContext* EquationsParser::InitContext::equationsys() {
  return getRuleContext<EquationsParser::EquationsysContext>(0);
}


size_t EquationsParser::InitContext::getRuleIndex() const {
  return EquationsParser::RuleInit;
}


antlrcpp::Any EquationsParser::InitContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitInit(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::InitContext* EquationsParser::init() {
  InitContext *_localctx = _tracker.createInstance<InitContext>(_ctx, getState());
  enterRule(_localctx, 4, EquationsParser::RuleInit);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(56);
    match(EquationsParser::INIT);
    setState(57);
    equationsys();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VariablesContext ------------------------------------------------------------------

EquationsParser::VariablesContext::VariablesContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* EquationsParser::VariablesContext::VARS() {
  return getToken(EquationsParser::VARS, 0);
}

EquationsParser::VardefinesContext* EquationsParser::VariablesContext::vardefines() {
  return getRuleContext<EquationsParser::VardefinesContext>(0);
}


size_t EquationsParser::VariablesContext::getRuleIndex() const {
  return EquationsParser::RuleVariables;
}


antlrcpp::Any EquationsParser::VariablesContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitVariables(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::VariablesContext* EquationsParser::variables() {
  VariablesContext *_localctx = _tracker.createInstance<VariablesContext>(_ctx, getState());
  enterRule(_localctx, 6, EquationsParser::RuleVariables);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(59);
    match(EquationsParser::VARS);
    setState(60);
    vardefines();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VardefinesContext ------------------------------------------------------------------

EquationsParser::VardefinesContext::VardefinesContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* EquationsParser::VardefinesContext::LCB() {
  return getToken(EquationsParser::LCB, 0);
}

tree::TerminalNode* EquationsParser::VardefinesContext::RCB() {
  return getToken(EquationsParser::RCB, 0);
}

std::vector<EquationsParser::VardefinelineContext *> EquationsParser::VardefinesContext::vardefineline() {
  return getRuleContexts<EquationsParser::VardefinelineContext>();
}

EquationsParser::VardefinelineContext* EquationsParser::VardefinesContext::vardefineline(size_t i) {
  return getRuleContext<EquationsParser::VardefinelineContext>(i);
}

std::vector<tree::TerminalNode *> EquationsParser::VardefinesContext::NEWLINE() {
  return getTokens(EquationsParser::NEWLINE);
}

tree::TerminalNode* EquationsParser::VardefinesContext::NEWLINE(size_t i) {
  return getToken(EquationsParser::NEWLINE, i);
}


size_t EquationsParser::VardefinesContext::getRuleIndex() const {
  return EquationsParser::RuleVardefines;
}


antlrcpp::Any EquationsParser::VardefinesContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitVardefines(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::VardefinesContext* EquationsParser::vardefines() {
  VardefinesContext *_localctx = _tracker.createInstance<VardefinesContext>(_ctx, getState());
  enterRule(_localctx, 8, EquationsParser::RuleVardefines);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(62);
    match(EquationsParser::LCB);
    setState(66);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 3, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(63);
        vardefineline(); 
      }
      setState(68);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 3, _ctx);
    }
    setState(72);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == EquationsParser::NEWLINE) {
      setState(69);
      match(EquationsParser::NEWLINE);
      setState(74);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(75);
    match(EquationsParser::RCB);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- EquationsysContext ------------------------------------------------------------------

EquationsParser::EquationsysContext::EquationsysContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* EquationsParser::EquationsysContext::LCB() {
  return getToken(EquationsParser::LCB, 0);
}

tree::TerminalNode* EquationsParser::EquationsysContext::RCB() {
  return getToken(EquationsParser::RCB, 0);
}

std::vector<EquationsParser::EquationlineContext *> EquationsParser::EquationsysContext::equationline() {
  return getRuleContexts<EquationsParser::EquationlineContext>();
}

EquationsParser::EquationlineContext* EquationsParser::EquationsysContext::equationline(size_t i) {
  return getRuleContext<EquationsParser::EquationlineContext>(i);
}

std::vector<tree::TerminalNode *> EquationsParser::EquationsysContext::NEWLINE() {
  return getTokens(EquationsParser::NEWLINE);
}

tree::TerminalNode* EquationsParser::EquationsysContext::NEWLINE(size_t i) {
  return getToken(EquationsParser::NEWLINE, i);
}


size_t EquationsParser::EquationsysContext::getRuleIndex() const {
  return EquationsParser::RuleEquationsys;
}


antlrcpp::Any EquationsParser::EquationsysContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitEquationsys(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::EquationsysContext* EquationsParser::equationsys() {
  EquationsysContext *_localctx = _tracker.createInstance<EquationsysContext>(_ctx, getState());
  enterRule(_localctx, 10, EquationsParser::RuleEquationsys);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(77);
    match(EquationsParser::LCB);
    setState(81);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 5, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(78);
        equationline(); 
      }
      setState(83);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 5, _ctx);
    }
    setState(87);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == EquationsParser::NEWLINE) {
      setState(84);
      match(EquationsParser::NEWLINE);
      setState(89);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(90);
    match(EquationsParser::RCB);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VarlistContext ------------------------------------------------------------------

EquationsParser::VarlistContext::VarlistContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> EquationsParser::VarlistContext::VARIABLE() {
  return getTokens(EquationsParser::VARIABLE);
}

tree::TerminalNode* EquationsParser::VarlistContext::VARIABLE(size_t i) {
  return getToken(EquationsParser::VARIABLE, i);
}

std::vector<tree::TerminalNode *> EquationsParser::VarlistContext::COMMA() {
  return getTokens(EquationsParser::COMMA);
}

tree::TerminalNode* EquationsParser::VarlistContext::COMMA(size_t i) {
  return getToken(EquationsParser::COMMA, i);
}


size_t EquationsParser::VarlistContext::getRuleIndex() const {
  return EquationsParser::RuleVarlist;
}


antlrcpp::Any EquationsParser::VarlistContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitVarlist(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::VarlistContext* EquationsParser::varlist() {
  VarlistContext *_localctx = _tracker.createInstance<VarlistContext>(_ctx, getState());
  enterRule(_localctx, 12, EquationsParser::RuleVarlist);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(92);
    match(EquationsParser::VARIABLE);
    setState(97);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == EquationsParser::COMMA) {
      setState(93);
      match(EquationsParser::COMMA);
      setState(94);
      match(EquationsParser::VARIABLE);
      setState(99);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VardefinelineContext ------------------------------------------------------------------

EquationsParser::VardefinelineContext::VardefinelineContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

EquationsParser::VardefineContext* EquationsParser::VardefinelineContext::vardefine() {
  return getRuleContext<EquationsParser::VardefineContext>(0);
}

std::vector<tree::TerminalNode *> EquationsParser::VardefinelineContext::NEWLINE() {
  return getTokens(EquationsParser::NEWLINE);
}

tree::TerminalNode* EquationsParser::VardefinelineContext::NEWLINE(size_t i) {
  return getToken(EquationsParser::NEWLINE, i);
}


size_t EquationsParser::VardefinelineContext::getRuleIndex() const {
  return EquationsParser::RuleVardefineline;
}


antlrcpp::Any EquationsParser::VardefinelineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitVardefineline(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::VardefinelineContext* EquationsParser::vardefineline() {
  VardefinelineContext *_localctx = _tracker.createInstance<VardefinelineContext>(_ctx, getState());
  enterRule(_localctx, 14, EquationsParser::RuleVardefineline);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(101); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(100);
      match(EquationsParser::NEWLINE);
      setState(103); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == EquationsParser::NEWLINE);
    setState(105);
    vardefine();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VardefineContext ------------------------------------------------------------------

EquationsParser::VardefineContext::VardefineContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

EquationsParser::ConstvardefineContext* EquationsParser::VardefineContext::constvardefine() {
  return getRuleContext<EquationsParser::ConstvardefineContext>(0);
}

EquationsParser::ExtvardefineContext* EquationsParser::VardefineContext::extvardefine() {
  return getRuleContext<EquationsParser::ExtvardefineContext>(0);
}


size_t EquationsParser::VardefineContext::getRuleIndex() const {
  return EquationsParser::RuleVardefine;
}


antlrcpp::Any EquationsParser::VardefineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitVardefine(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::VardefineContext* EquationsParser::vardefine() {
  VardefineContext *_localctx = _tracker.createInstance<VardefineContext>(_ctx, getState());
  enterRule(_localctx, 16, EquationsParser::RuleVardefine);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    setState(109);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case EquationsParser::CONST: {
        enterOuterAlt(_localctx, 1);
        setState(107);
        constvardefine();
        break;
      }

      case EquationsParser::EXTERNAL: {
        enterOuterAlt(_localctx, 2);
        setState(108);
        extvardefine();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ConstvardefineContext ------------------------------------------------------------------

EquationsParser::ConstvardefineContext::ConstvardefineContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* EquationsParser::ConstvardefineContext::CONST() {
  return getToken(EquationsParser::CONST, 0);
}

EquationsParser::VarlistContext* EquationsParser::ConstvardefineContext::varlist() {
  return getRuleContext<EquationsParser::VarlistContext>(0);
}


size_t EquationsParser::ConstvardefineContext::getRuleIndex() const {
  return EquationsParser::RuleConstvardefine;
}


antlrcpp::Any EquationsParser::ConstvardefineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitConstvardefine(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::ConstvardefineContext* EquationsParser::constvardefine() {
  ConstvardefineContext *_localctx = _tracker.createInstance<ConstvardefineContext>(_ctx, getState());
  enterRule(_localctx, 18, EquationsParser::RuleConstvardefine);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(111);
    match(EquationsParser::CONST);
    setState(112);
    varlist();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExtvardefineContext ------------------------------------------------------------------

EquationsParser::ExtvardefineContext::ExtvardefineContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* EquationsParser::ExtvardefineContext::EXTERNAL() {
  return getToken(EquationsParser::EXTERNAL, 0);
}

EquationsParser::VarlistContext* EquationsParser::ExtvardefineContext::varlist() {
  return getRuleContext<EquationsParser::VarlistContext>(0);
}


size_t EquationsParser::ExtvardefineContext::getRuleIndex() const {
  return EquationsParser::RuleExtvardefine;
}


antlrcpp::Any EquationsParser::ExtvardefineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitExtvardefine(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::ExtvardefineContext* EquationsParser::extvardefine() {
  ExtvardefineContext *_localctx = _tracker.createInstance<ExtvardefineContext>(_ctx, getState());
  enterRule(_localctx, 20, EquationsParser::RuleExtvardefine);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(114);
    match(EquationsParser::EXTERNAL);
    setState(115);
    varlist();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- EquationContext ------------------------------------------------------------------

EquationsParser::EquationContext::EquationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* EquationsParser::EquationContext::EQUAL() {
  return getToken(EquationsParser::EQUAL, 0);
}

std::vector<EquationsParser::ExpressionContext *> EquationsParser::EquationContext::expression() {
  return getRuleContexts<EquationsParser::ExpressionContext>();
}

EquationsParser::ExpressionContext* EquationsParser::EquationContext::expression(size_t i) {
  return getRuleContext<EquationsParser::ExpressionContext>(i);
}


size_t EquationsParser::EquationContext::getRuleIndex() const {
  return EquationsParser::RuleEquation;
}


antlrcpp::Any EquationsParser::EquationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitEquation(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::EquationContext* EquationsParser::equation() {
  EquationContext *_localctx = _tracker.createInstance<EquationContext>(_ctx, getState());
  enterRule(_localctx, 22, EquationsParser::RuleEquation);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(117);
    dynamic_cast<EquationContext *>(_localctx)->left = expression(0);
    setState(118);
    match(EquationsParser::EQUAL);
    setState(119);
    dynamic_cast<EquationContext *>(_localctx)->right = expression(0);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- EquationlineContext ------------------------------------------------------------------

EquationsParser::EquationlineContext::EquationlineContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

EquationsParser::EquationContext* EquationsParser::EquationlineContext::equation() {
  return getRuleContext<EquationsParser::EquationContext>(0);
}

std::vector<tree::TerminalNode *> EquationsParser::EquationlineContext::NEWLINE() {
  return getTokens(EquationsParser::NEWLINE);
}

tree::TerminalNode* EquationsParser::EquationlineContext::NEWLINE(size_t i) {
  return getToken(EquationsParser::NEWLINE, i);
}


size_t EquationsParser::EquationlineContext::getRuleIndex() const {
  return EquationsParser::RuleEquationline;
}


antlrcpp::Any EquationsParser::EquationlineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitEquationline(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::EquationlineContext* EquationsParser::equationline() {
  EquationlineContext *_localctx = _tracker.createInstance<EquationlineContext>(_ctx, getState());
  enterRule(_localctx, 24, EquationsParser::RuleEquationline);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(122); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(121);
      match(EquationsParser::NEWLINE);
      setState(124); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == EquationsParser::NEWLINE);
    setState(126);
    equation();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpressionlistContext ------------------------------------------------------------------

EquationsParser::ExpressionlistContext::ExpressionlistContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<EquationsParser::ExpressionContext *> EquationsParser::ExpressionlistContext::expression() {
  return getRuleContexts<EquationsParser::ExpressionContext>();
}

EquationsParser::ExpressionContext* EquationsParser::ExpressionlistContext::expression(size_t i) {
  return getRuleContext<EquationsParser::ExpressionContext>(i);
}

std::vector<tree::TerminalNode *> EquationsParser::ExpressionlistContext::COMMA() {
  return getTokens(EquationsParser::COMMA);
}

tree::TerminalNode* EquationsParser::ExpressionlistContext::COMMA(size_t i) {
  return getToken(EquationsParser::COMMA, i);
}


size_t EquationsParser::ExpressionlistContext::getRuleIndex() const {
  return EquationsParser::RuleExpressionlist;
}


antlrcpp::Any EquationsParser::ExpressionlistContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitExpressionlist(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::ExpressionlistContext* EquationsParser::expressionlist() {
  ExpressionlistContext *_localctx = _tracker.createInstance<ExpressionlistContext>(_ctx, getState());
  enterRule(_localctx, 26, EquationsParser::RuleExpressionlist);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(128);
    expression(0);
    setState(133);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == EquationsParser::COMMA) {
      setState(129);
      match(EquationsParser::COMMA);
      setState(130);
      expression(0);
      setState(135);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- IntlistContext ------------------------------------------------------------------

EquationsParser::IntlistContext::IntlistContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> EquationsParser::IntlistContext::INTEGER() {
  return getTokens(EquationsParser::INTEGER);
}

tree::TerminalNode* EquationsParser::IntlistContext::INTEGER(size_t i) {
  return getToken(EquationsParser::INTEGER, i);
}

std::vector<tree::TerminalNode *> EquationsParser::IntlistContext::COMMA() {
  return getTokens(EquationsParser::COMMA);
}

tree::TerminalNode* EquationsParser::IntlistContext::COMMA(size_t i) {
  return getToken(EquationsParser::COMMA, i);
}


size_t EquationsParser::IntlistContext::getRuleIndex() const {
  return EquationsParser::RuleIntlist;
}


antlrcpp::Any EquationsParser::IntlistContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitIntlist(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::IntlistContext* EquationsParser::intlist() {
  IntlistContext *_localctx = _tracker.createInstance<IntlistContext>(_ctx, getState());
  enterRule(_localctx, 28, EquationsParser::RuleIntlist);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(136);
    match(EquationsParser::INTEGER);
    setState(141);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == EquationsParser::COMMA) {
      setState(137);
      match(EquationsParser::COMMA);
      setState(138);
      match(EquationsParser::INTEGER);
      setState(143);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ConstvalueContext ------------------------------------------------------------------

EquationsParser::ConstvalueContext::ConstvalueContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* EquationsParser::ConstvalueContext::FLOAT() {
  return getToken(EquationsParser::FLOAT, 0);
}

tree::TerminalNode* EquationsParser::ConstvalueContext::INTEGER() {
  return getToken(EquationsParser::INTEGER, 0);
}


size_t EquationsParser::ConstvalueContext::getRuleIndex() const {
  return EquationsParser::RuleConstvalue;
}


antlrcpp::Any EquationsParser::ConstvalueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitConstvalue(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::ConstvalueContext* EquationsParser::constvalue() {
  ConstvalueContext *_localctx = _tracker.createInstance<ConstvalueContext>(_ctx, getState());
  enterRule(_localctx, 30, EquationsParser::RuleConstvalue);
  size_t _la = 0;

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(144);
    _la = _input->LA(1);
    if (!(_la == EquationsParser::FLOAT

    || _la == EquationsParser::INTEGER)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ModlinkContext ------------------------------------------------------------------

EquationsParser::ModlinkContext::ModlinkContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> EquationsParser::ModlinkContext::VARIABLE() {
  return getTokens(EquationsParser::VARIABLE);
}

tree::TerminalNode* EquationsParser::ModlinkContext::VARIABLE(size_t i) {
  return getToken(EquationsParser::VARIABLE, i);
}

tree::TerminalNode* EquationsParser::ModlinkContext::LSB() {
  return getToken(EquationsParser::LSB, 0);
}

EquationsParser::IntlistContext* EquationsParser::ModlinkContext::intlist() {
  return getRuleContext<EquationsParser::IntlistContext>(0);
}

tree::TerminalNode* EquationsParser::ModlinkContext::RSB() {
  return getToken(EquationsParser::RSB, 0);
}

tree::TerminalNode* EquationsParser::ModlinkContext::DOT() {
  return getToken(EquationsParser::DOT, 0);
}


size_t EquationsParser::ModlinkContext::getRuleIndex() const {
  return EquationsParser::RuleModlink;
}


antlrcpp::Any EquationsParser::ModlinkContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitModlink(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::ModlinkContext* EquationsParser::modlink() {
  ModlinkContext *_localctx = _tracker.createInstance<ModlinkContext>(_ctx, getState());
  enterRule(_localctx, 32, EquationsParser::RuleModlink);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(146);
    match(EquationsParser::VARIABLE);
    setState(147);
    match(EquationsParser::LSB);
    setState(148);
    intlist();
    setState(149);
    match(EquationsParser::RSB);
    setState(150);
    match(EquationsParser::DOT);
    setState(151);
    match(EquationsParser::VARIABLE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ModlinkbaseContext ------------------------------------------------------------------

EquationsParser::ModlinkbaseContext::ModlinkbaseContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* EquationsParser::ModlinkbaseContext::BAR() {
  return getToken(EquationsParser::BAR, 0);
}

EquationsParser::ModlinkContext* EquationsParser::ModlinkbaseContext::modlink() {
  return getRuleContext<EquationsParser::ModlinkContext>(0);
}


size_t EquationsParser::ModlinkbaseContext::getRuleIndex() const {
  return EquationsParser::RuleModlinkbase;
}


antlrcpp::Any EquationsParser::ModlinkbaseContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitModlinkbase(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::ModlinkbaseContext* EquationsParser::modlinkbase() {
  ModlinkbaseContext *_localctx = _tracker.createInstance<ModlinkbaseContext>(_ctx, getState());
  enterRule(_localctx, 34, EquationsParser::RuleModlinkbase);

  auto onExit = finally([=] {
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(153);
    match(EquationsParser::BAR);
    setState(154);
    modlink();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpressionContext ------------------------------------------------------------------

EquationsParser::ExpressionContext::ExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t EquationsParser::ExpressionContext::getRuleIndex() const {
  return EquationsParser::RuleExpression;
}

void EquationsParser::ExpressionContext::copyFrom(ExpressionContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- AndorContext ------------------------------------------------------------------

std::vector<EquationsParser::ExpressionContext *> EquationsParser::AndorContext::expression() {
  return getRuleContexts<EquationsParser::ExpressionContext>();
}

EquationsParser::ExpressionContext* EquationsParser::AndorContext::expression(size_t i) {
  return getRuleContext<EquationsParser::ExpressionContext>(i);
}

tree::TerminalNode* EquationsParser::AndorContext::OR() {
  return getToken(EquationsParser::OR, 0);
}

tree::TerminalNode* EquationsParser::AndorContext::AND() {
  return getToken(EquationsParser::AND, 0);
}

EquationsParser::AndorContext::AndorContext(ExpressionContext *ctx) { copyFrom(ctx); }


antlrcpp::Any EquationsParser::AndorContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitAndor(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ModellinkContext ------------------------------------------------------------------

EquationsParser::ModlinkContext* EquationsParser::ModellinkContext::modlink() {
  return getRuleContext<EquationsParser::ModlinkContext>(0);
}

EquationsParser::ModellinkContext::ModellinkContext(ExpressionContext *ctx) { copyFrom(ctx); }


antlrcpp::Any EquationsParser::ModellinkContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitModellink(this);
  else
    return visitor->visitChildren(this);
}
//----------------- HighlowContext ------------------------------------------------------------------

std::vector<EquationsParser::ExpressionContext *> EquationsParser::HighlowContext::expression() {
  return getRuleContexts<EquationsParser::ExpressionContext>();
}

EquationsParser::ExpressionContext* EquationsParser::HighlowContext::expression(size_t i) {
  return getRuleContext<EquationsParser::ExpressionContext>(i);
}

tree::TerminalNode* EquationsParser::HighlowContext::HIGH() {
  return getToken(EquationsParser::HIGH, 0);
}

tree::TerminalNode* EquationsParser::HighlowContext::LOW() {
  return getToken(EquationsParser::LOW, 0);
}

EquationsParser::HighlowContext::HighlowContext(ExpressionContext *ctx) { copyFrom(ctx); }


antlrcpp::Any EquationsParser::HighlowContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitHighlow(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ModellinkbaseContext ------------------------------------------------------------------

EquationsParser::ModlinkbaseContext* EquationsParser::ModellinkbaseContext::modlinkbase() {
  return getRuleContext<EquationsParser::ModlinkbaseContext>(0);
}

EquationsParser::ModellinkbaseContext::ModellinkbaseContext(ExpressionContext *ctx) { copyFrom(ctx); }


antlrcpp::Any EquationsParser::ModellinkbaseContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitModellinkbase(this);
  else
    return visitor->visitChildren(this);
}
//----------------- FunctionContext ------------------------------------------------------------------

tree::TerminalNode* EquationsParser::FunctionContext::LB() {
  return getToken(EquationsParser::LB, 0);
}

tree::TerminalNode* EquationsParser::FunctionContext::RB() {
  return getToken(EquationsParser::RB, 0);
}

tree::TerminalNode* EquationsParser::FunctionContext::VARIABLE() {
  return getToken(EquationsParser::VARIABLE, 0);
}

EquationsParser::ExpressionlistContext* EquationsParser::FunctionContext::expressionlist() {
  return getRuleContext<EquationsParser::ExpressionlistContext>(0);
}

EquationsParser::FunctionContext::FunctionContext(ExpressionContext *ctx) { copyFrom(ctx); }


antlrcpp::Any EquationsParser::FunctionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitFunction(this);
  else
    return visitor->visitChildren(this);
}
//----------------- VariableContext ------------------------------------------------------------------

tree::TerminalNode* EquationsParser::VariableContext::VARIABLE() {
  return getToken(EquationsParser::VARIABLE, 0);
}

EquationsParser::VariableContext::VariableContext(ExpressionContext *ctx) { copyFrom(ctx); }


antlrcpp::Any EquationsParser::VariableContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitVariable(this);
  else
    return visitor->visitChildren(this);
}
//----------------- PowContext ------------------------------------------------------------------

std::vector<EquationsParser::ExpressionContext *> EquationsParser::PowContext::expression() {
  return getRuleContexts<EquationsParser::ExpressionContext>();
}

EquationsParser::ExpressionContext* EquationsParser::PowContext::expression(size_t i) {
  return getRuleContext<EquationsParser::ExpressionContext>(i);
}

tree::TerminalNode* EquationsParser::PowContext::POW() {
  return getToken(EquationsParser::POW, 0);
}

EquationsParser::PowContext::PowContext(ExpressionContext *ctx) { copyFrom(ctx); }


antlrcpp::Any EquationsParser::PowContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitPow(this);
  else
    return visitor->visitChildren(this);
}
//----------------- RealconstContext ------------------------------------------------------------------

EquationsParser::ConstvalueContext* EquationsParser::RealconstContext::constvalue() {
  return getRuleContext<EquationsParser::ConstvalueContext>(0);
}

EquationsParser::RealconstContext::RealconstContext(ExpressionContext *ctx) { copyFrom(ctx); }


antlrcpp::Any EquationsParser::RealconstContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitRealconst(this);
  else
    return visitor->visitChildren(this);
}
//----------------- UnaryContext ------------------------------------------------------------------

EquationsParser::ExpressionContext* EquationsParser::UnaryContext::expression() {
  return getRuleContext<EquationsParser::ExpressionContext>(0);
}

tree::TerminalNode* EquationsParser::UnaryContext::PLUS() {
  return getToken(EquationsParser::PLUS, 0);
}

tree::TerminalNode* EquationsParser::UnaryContext::MINUS() {
  return getToken(EquationsParser::MINUS, 0);
}

EquationsParser::UnaryContext::UnaryContext(ExpressionContext *ctx) { copyFrom(ctx); }


antlrcpp::Any EquationsParser::UnaryContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitUnary(this);
  else
    return visitor->visitChildren(this);
}
//----------------- InfixContext ------------------------------------------------------------------

std::vector<EquationsParser::ExpressionContext *> EquationsParser::InfixContext::expression() {
  return getRuleContexts<EquationsParser::ExpressionContext>();
}

EquationsParser::ExpressionContext* EquationsParser::InfixContext::expression(size_t i) {
  return getRuleContext<EquationsParser::ExpressionContext>(i);
}

tree::TerminalNode* EquationsParser::InfixContext::MUL() {
  return getToken(EquationsParser::MUL, 0);
}

tree::TerminalNode* EquationsParser::InfixContext::DIV() {
  return getToken(EquationsParser::DIV, 0);
}

tree::TerminalNode* EquationsParser::InfixContext::PLUS() {
  return getToken(EquationsParser::PLUS, 0);
}

tree::TerminalNode* EquationsParser::InfixContext::MINUS() {
  return getToken(EquationsParser::MINUS, 0);
}

EquationsParser::InfixContext::InfixContext(ExpressionContext *ctx) { copyFrom(ctx); }


antlrcpp::Any EquationsParser::InfixContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitInfix(this);
  else
    return visitor->visitChildren(this);
}
//----------------- BracesContext ------------------------------------------------------------------

tree::TerminalNode* EquationsParser::BracesContext::LB() {
  return getToken(EquationsParser::LB, 0);
}

EquationsParser::ExpressionContext* EquationsParser::BracesContext::expression() {
  return getRuleContext<EquationsParser::ExpressionContext>(0);
}

tree::TerminalNode* EquationsParser::BracesContext::RB() {
  return getToken(EquationsParser::RB, 0);
}

EquationsParser::BracesContext::BracesContext(ExpressionContext *ctx) { copyFrom(ctx); }


antlrcpp::Any EquationsParser::BracesContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitBraces(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::ExpressionContext* EquationsParser::expression() {
   return expression(0);
}

EquationsParser::ExpressionContext* EquationsParser::expression(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  EquationsParser::ExpressionContext *_localctx = _tracker.createInstance<ExpressionContext>(_ctx, parentState);
  EquationsParser::ExpressionContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 36;
  enterRecursionRule(_localctx, 36, EquationsParser::RuleExpression, precedence);

    size_t _la = 0;

  auto onExit = finally([=] {
    unrollRecursionContexts(parentContext);
  });
  try {
    size_t alt;
    enterOuterAlt(_localctx, 1);
    setState(173);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 14, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<RealconstContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(157);
      constvalue();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<VariableContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(158);
      match(EquationsParser::VARIABLE);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<ModellinkbaseContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(159);
      modlinkbase();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<ModellinkContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(160);
      modlink();
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<BracesContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(161);
      match(EquationsParser::LB);
      setState(162);
      expression(0);
      setState(163);
      match(EquationsParser::RB);
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<UnaryContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(165);
      dynamic_cast<UnaryContext *>(_localctx)->op = _input->LT(1);
      _la = _input->LA(1);
      if (!(_la == EquationsParser::PLUS

      || _la == EquationsParser::MINUS)) {
        dynamic_cast<UnaryContext *>(_localctx)->op = _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(166);
      expression(6);
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<FunctionContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(167);
      dynamic_cast<FunctionContext *>(_localctx)->func = match(EquationsParser::VARIABLE);
      setState(168);
      match(EquationsParser::LB);
      setState(170);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & ((1ULL << EquationsParser::FLOAT)
        | (1ULL << EquationsParser::INTEGER)
        | (1ULL << EquationsParser::VARIABLE)
        | (1ULL << EquationsParser::BAR)
        | (1ULL << EquationsParser::LB)
        | (1ULL << EquationsParser::PLUS)
        | (1ULL << EquationsParser::MINUS))) != 0)) {
        setState(169);
        expressionlist();
      }
      setState(172);
      match(EquationsParser::RB);
      break;
    }

    }
    _ctx->stop = _input->LT(-1);
    setState(192);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 16, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(190);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<PowContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(175);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(176);
          match(EquationsParser::POW);
          setState(177);
          expression(7);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<InfixContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          newContext->left = previousContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(178);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(179);
          dynamic_cast<InfixContext *>(_localctx)->op = _input->LT(1);
          _la = _input->LA(1);
          if (!(_la == EquationsParser::MUL

          || _la == EquationsParser::DIV)) {
            dynamic_cast<InfixContext *>(_localctx)->op = _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(180);
          dynamic_cast<InfixContext *>(_localctx)->right = expression(6);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<InfixContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          newContext->left = previousContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(181);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(182);
          dynamic_cast<InfixContext *>(_localctx)->op = _input->LT(1);
          _la = _input->LA(1);
          if (!(_la == EquationsParser::PLUS

          || _la == EquationsParser::MINUS)) {
            dynamic_cast<InfixContext *>(_localctx)->op = _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(183);
          dynamic_cast<InfixContext *>(_localctx)->right = expression(5);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<HighlowContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          newContext->left = previousContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(184);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(185);
          dynamic_cast<HighlowContext *>(_localctx)->op = _input->LT(1);
          _la = _input->LA(1);
          if (!(_la == EquationsParser::HIGH

          || _la == EquationsParser::LOW)) {
            dynamic_cast<HighlowContext *>(_localctx)->op = _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(186);
          dynamic_cast<HighlowContext *>(_localctx)->right = expression(4);
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<AndorContext>(_tracker.createInstance<ExpressionContext>(parentContext, parentState));
          _localctx = newContext;
          newContext->left = previousContext;
          pushNewRecursionContext(newContext, startState, RuleExpression);
          setState(187);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(188);
          dynamic_cast<AndorContext *>(_localctx)->op = _input->LT(1);
          _la = _input->LA(1);
          if (!(_la == EquationsParser::AND

          || _la == EquationsParser::OR)) {
            dynamic_cast<AndorContext *>(_localctx)->op = _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(189);
          dynamic_cast<AndorContext *>(_localctx)->right = expression(3);
          break;
        }

        } 
      }
      setState(194);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 16, _ctx);
    }
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

bool EquationsParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 18: return expressionSempred(dynamic_cast<ExpressionContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool EquationsParser::expressionSempred(ExpressionContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 7);
    case 1: return precpred(_ctx, 5);
    case 2: return precpred(_ctx, 4);
    case 3: return precpred(_ctx, 3);
    case 4: return precpred(_ctx, 2);

  default:
    break;
  }
  return true;
}

// Static vars and initialization.
std::vector<dfa::DFA> EquationsParser::_decisionToDFA;
atn::PredictionContextCache EquationsParser::_sharedContextCache;

// We own the ATN which in turn owns the ATN states.
atn::ATN EquationsParser::_atn;
std::vector<uint16_t> EquationsParser::_serializedATN;

std::vector<std::string> EquationsParser::_ruleNames = {
  "input", "main", "init", "variables", "vardefines", "equationsys", "varlist", 
  "vardefineline", "vardefine", "constvardefine", "extvardefine", "equation", 
  "equationline", "expressionlist", "intlist", "constvalue", "modlink", 
  "modlinkbase", "expression"
};

std::vector<std::string> EquationsParser::_literalNames = {
  "", "", "", "", "", "", "'const'", "'external'", "", "", "", "'='", "'#'", 
  "'&'", "'|'", "'('", "')'", "'{'", "'}'", "'['", "']'", "'.'", "','", 
  "'+'", "'-'", "'*'", "'/'", "'^'", "'>'", "'<'"
};

std::vector<std::string> EquationsParser::_symbolicNames = {
  "", "FLOAT", "INTEGER", "MAIN", "INIT", "VARS", "CONST", "EXTERNAL", "VARIABLE", 
  "NEWLINE", "LINE_COMMENT", "EQUAL", "BAR", "AND", "OR", "LB", "RB", "LCB", 
  "RCB", "LSB", "RSB", "DOT", "COMMA", "PLUS", "MINUS", "MUL", "DIV", "POW", 
  "HIGH", "LOW", "WS"
};

dfa::Vocabulary EquationsParser::_vocabulary(_literalNames, _symbolicNames);

std::vector<std::string> EquationsParser::_tokenNames;

EquationsParser::Initializer::Initializer() {
	for (size_t i = 0; i < _symbolicNames.size(); ++i) {
		std::string name = _vocabulary.getLiteralName(i);
		if (name.empty()) {
			name = _vocabulary.getSymbolicName(i);
		}

		if (name.empty()) {
			_tokenNames.push_back("<INVALID>");
		} else {
      _tokenNames.push_back(name);
    }
	}

  _serializedATN = {
    0x3, 0x608b, 0xa72a, 0x8133, 0xb9ed, 0x417c, 0x3be7, 0x7786, 0x5964, 
    0x3, 0x20, 0xc6, 0x4, 0x2, 0x9, 0x2, 0x4, 0x3, 0x9, 0x3, 0x4, 0x4, 0x9, 
    0x4, 0x4, 0x5, 0x9, 0x5, 0x4, 0x6, 0x9, 0x6, 0x4, 0x7, 0x9, 0x7, 0x4, 
    0x8, 0x9, 0x8, 0x4, 0x9, 0x9, 0x9, 0x4, 0xa, 0x9, 0xa, 0x4, 0xb, 0x9, 
    0xb, 0x4, 0xc, 0x9, 0xc, 0x4, 0xd, 0x9, 0xd, 0x4, 0xe, 0x9, 0xe, 0x4, 
    0xf, 0x9, 0xf, 0x4, 0x10, 0x9, 0x10, 0x4, 0x11, 0x9, 0x11, 0x4, 0x12, 
    0x9, 0x12, 0x4, 0x13, 0x9, 0x13, 0x4, 0x14, 0x9, 0x14, 0x3, 0x2, 0x5, 
    0x2, 0x2a, 0xa, 0x2, 0x3, 0x2, 0x3, 0x2, 0x5, 0x2, 0x2e, 0xa, 0x2, 0x3, 
    0x2, 0x7, 0x2, 0x31, 0xa, 0x2, 0xc, 0x2, 0xe, 0x2, 0x34, 0xb, 0x2, 0x3, 
    0x2, 0x3, 0x2, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x4, 0x3, 
    0x4, 0x3, 0x5, 0x3, 0x5, 0x3, 0x5, 0x3, 0x6, 0x3, 0x6, 0x7, 0x6, 0x43, 
    0xa, 0x6, 0xc, 0x6, 0xe, 0x6, 0x46, 0xb, 0x6, 0x3, 0x6, 0x7, 0x6, 0x49, 
    0xa, 0x6, 0xc, 0x6, 0xe, 0x6, 0x4c, 0xb, 0x6, 0x3, 0x6, 0x3, 0x6, 0x3, 
    0x7, 0x3, 0x7, 0x7, 0x7, 0x52, 0xa, 0x7, 0xc, 0x7, 0xe, 0x7, 0x55, 0xb, 
    0x7, 0x3, 0x7, 0x7, 0x7, 0x58, 0xa, 0x7, 0xc, 0x7, 0xe, 0x7, 0x5b, 0xb, 
    0x7, 0x3, 0x7, 0x3, 0x7, 0x3, 0x8, 0x3, 0x8, 0x3, 0x8, 0x7, 0x8, 0x62, 
    0xa, 0x8, 0xc, 0x8, 0xe, 0x8, 0x65, 0xb, 0x8, 0x3, 0x9, 0x6, 0x9, 0x68, 
    0xa, 0x9, 0xd, 0x9, 0xe, 0x9, 0x69, 0x3, 0x9, 0x3, 0x9, 0x3, 0xa, 0x3, 
    0xa, 0x5, 0xa, 0x70, 0xa, 0xa, 0x3, 0xb, 0x3, 0xb, 0x3, 0xb, 0x3, 0xc, 
    0x3, 0xc, 0x3, 0xc, 0x3, 0xd, 0x3, 0xd, 0x3, 0xd, 0x3, 0xd, 0x3, 0xe, 
    0x6, 0xe, 0x7d, 0xa, 0xe, 0xd, 0xe, 0xe, 0xe, 0x7e, 0x3, 0xe, 0x3, 0xe, 
    0x3, 0xf, 0x3, 0xf, 0x3, 0xf, 0x7, 0xf, 0x86, 0xa, 0xf, 0xc, 0xf, 0xe, 
    0xf, 0x89, 0xb, 0xf, 0x3, 0x10, 0x3, 0x10, 0x3, 0x10, 0x7, 0x10, 0x8e, 
    0xa, 0x10, 0xc, 0x10, 0xe, 0x10, 0x91, 0xb, 0x10, 0x3, 0x11, 0x3, 0x11, 
    0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 0x12, 0x3, 
    0x12, 0x3, 0x13, 0x3, 0x13, 0x3, 0x13, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 
    0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 
    0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x5, 0x14, 0xad, 0xa, 
    0x14, 0x3, 0x14, 0x5, 0x14, 0xb0, 0xa, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 
    0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 
    0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x3, 0x14, 0x7, 
    0x14, 0xc1, 0xa, 0x14, 0xc, 0x14, 0xe, 0x14, 0xc4, 0xb, 0x14, 0x3, 0x14, 
    0x2, 0x3, 0x26, 0x15, 0x2, 0x4, 0x6, 0x8, 0xa, 0xc, 0xe, 0x10, 0x12, 
    0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e, 0x20, 0x22, 0x24, 0x26, 0x2, 0x7, 
    0x3, 0x2, 0x3, 0x4, 0x3, 0x2, 0x19, 0x1a, 0x3, 0x2, 0x1b, 0x1c, 0x3, 
    0x2, 0x1e, 0x1f, 0x3, 0x2, 0xf, 0x10, 0x2, 0xcb, 0x2, 0x29, 0x3, 0x2, 
    0x2, 0x2, 0x4, 0x37, 0x3, 0x2, 0x2, 0x2, 0x6, 0x3a, 0x3, 0x2, 0x2, 0x2, 
    0x8, 0x3d, 0x3, 0x2, 0x2, 0x2, 0xa, 0x40, 0x3, 0x2, 0x2, 0x2, 0xc, 0x4f, 
    0x3, 0x2, 0x2, 0x2, 0xe, 0x5e, 0x3, 0x2, 0x2, 0x2, 0x10, 0x67, 0x3, 
    0x2, 0x2, 0x2, 0x12, 0x6f, 0x3, 0x2, 0x2, 0x2, 0x14, 0x71, 0x3, 0x2, 
    0x2, 0x2, 0x16, 0x74, 0x3, 0x2, 0x2, 0x2, 0x18, 0x77, 0x3, 0x2, 0x2, 
    0x2, 0x1a, 0x7c, 0x3, 0x2, 0x2, 0x2, 0x1c, 0x82, 0x3, 0x2, 0x2, 0x2, 
    0x1e, 0x8a, 0x3, 0x2, 0x2, 0x2, 0x20, 0x92, 0x3, 0x2, 0x2, 0x2, 0x22, 
    0x94, 0x3, 0x2, 0x2, 0x2, 0x24, 0x9b, 0x3, 0x2, 0x2, 0x2, 0x26, 0xaf, 
    0x3, 0x2, 0x2, 0x2, 0x28, 0x2a, 0x5, 0x8, 0x5, 0x2, 0x29, 0x28, 0x3, 
    0x2, 0x2, 0x2, 0x29, 0x2a, 0x3, 0x2, 0x2, 0x2, 0x2a, 0x2b, 0x3, 0x2, 
    0x2, 0x2, 0x2b, 0x2d, 0x5, 0x4, 0x3, 0x2, 0x2c, 0x2e, 0x5, 0x6, 0x4, 
    0x2, 0x2d, 0x2c, 0x3, 0x2, 0x2, 0x2, 0x2d, 0x2e, 0x3, 0x2, 0x2, 0x2, 
    0x2e, 0x32, 0x3, 0x2, 0x2, 0x2, 0x2f, 0x31, 0x7, 0xb, 0x2, 0x2, 0x30, 
    0x2f, 0x3, 0x2, 0x2, 0x2, 0x31, 0x34, 0x3, 0x2, 0x2, 0x2, 0x32, 0x30, 
    0x3, 0x2, 0x2, 0x2, 0x32, 0x33, 0x3, 0x2, 0x2, 0x2, 0x33, 0x35, 0x3, 
    0x2, 0x2, 0x2, 0x34, 0x32, 0x3, 0x2, 0x2, 0x2, 0x35, 0x36, 0x7, 0x2, 
    0x2, 0x3, 0x36, 0x3, 0x3, 0x2, 0x2, 0x2, 0x37, 0x38, 0x7, 0x5, 0x2, 
    0x2, 0x38, 0x39, 0x5, 0xc, 0x7, 0x2, 0x39, 0x5, 0x3, 0x2, 0x2, 0x2, 
    0x3a, 0x3b, 0x7, 0x6, 0x2, 0x2, 0x3b, 0x3c, 0x5, 0xc, 0x7, 0x2, 0x3c, 
    0x7, 0x3, 0x2, 0x2, 0x2, 0x3d, 0x3e, 0x7, 0x7, 0x2, 0x2, 0x3e, 0x3f, 
    0x5, 0xa, 0x6, 0x2, 0x3f, 0x9, 0x3, 0x2, 0x2, 0x2, 0x40, 0x44, 0x7, 
    0x13, 0x2, 0x2, 0x41, 0x43, 0x5, 0x10, 0x9, 0x2, 0x42, 0x41, 0x3, 0x2, 
    0x2, 0x2, 0x43, 0x46, 0x3, 0x2, 0x2, 0x2, 0x44, 0x42, 0x3, 0x2, 0x2, 
    0x2, 0x44, 0x45, 0x3, 0x2, 0x2, 0x2, 0x45, 0x4a, 0x3, 0x2, 0x2, 0x2, 
    0x46, 0x44, 0x3, 0x2, 0x2, 0x2, 0x47, 0x49, 0x7, 0xb, 0x2, 0x2, 0x48, 
    0x47, 0x3, 0x2, 0x2, 0x2, 0x49, 0x4c, 0x3, 0x2, 0x2, 0x2, 0x4a, 0x48, 
    0x3, 0x2, 0x2, 0x2, 0x4a, 0x4b, 0x3, 0x2, 0x2, 0x2, 0x4b, 0x4d, 0x3, 
    0x2, 0x2, 0x2, 0x4c, 0x4a, 0x3, 0x2, 0x2, 0x2, 0x4d, 0x4e, 0x7, 0x14, 
    0x2, 0x2, 0x4e, 0xb, 0x3, 0x2, 0x2, 0x2, 0x4f, 0x53, 0x7, 0x13, 0x2, 
    0x2, 0x50, 0x52, 0x5, 0x1a, 0xe, 0x2, 0x51, 0x50, 0x3, 0x2, 0x2, 0x2, 
    0x52, 0x55, 0x3, 0x2, 0x2, 0x2, 0x53, 0x51, 0x3, 0x2, 0x2, 0x2, 0x53, 
    0x54, 0x3, 0x2, 0x2, 0x2, 0x54, 0x59, 0x3, 0x2, 0x2, 0x2, 0x55, 0x53, 
    0x3, 0x2, 0x2, 0x2, 0x56, 0x58, 0x7, 0xb, 0x2, 0x2, 0x57, 0x56, 0x3, 
    0x2, 0x2, 0x2, 0x58, 0x5b, 0x3, 0x2, 0x2, 0x2, 0x59, 0x57, 0x3, 0x2, 
    0x2, 0x2, 0x59, 0x5a, 0x3, 0x2, 0x2, 0x2, 0x5a, 0x5c, 0x3, 0x2, 0x2, 
    0x2, 0x5b, 0x59, 0x3, 0x2, 0x2, 0x2, 0x5c, 0x5d, 0x7, 0x14, 0x2, 0x2, 
    0x5d, 0xd, 0x3, 0x2, 0x2, 0x2, 0x5e, 0x63, 0x7, 0xa, 0x2, 0x2, 0x5f, 
    0x60, 0x7, 0x18, 0x2, 0x2, 0x60, 0x62, 0x7, 0xa, 0x2, 0x2, 0x61, 0x5f, 
    0x3, 0x2, 0x2, 0x2, 0x62, 0x65, 0x3, 0x2, 0x2, 0x2, 0x63, 0x61, 0x3, 
    0x2, 0x2, 0x2, 0x63, 0x64, 0x3, 0x2, 0x2, 0x2, 0x64, 0xf, 0x3, 0x2, 
    0x2, 0x2, 0x65, 0x63, 0x3, 0x2, 0x2, 0x2, 0x66, 0x68, 0x7, 0xb, 0x2, 
    0x2, 0x67, 0x66, 0x3, 0x2, 0x2, 0x2, 0x68, 0x69, 0x3, 0x2, 0x2, 0x2, 
    0x69, 0x67, 0x3, 0x2, 0x2, 0x2, 0x69, 0x6a, 0x3, 0x2, 0x2, 0x2, 0x6a, 
    0x6b, 0x3, 0x2, 0x2, 0x2, 0x6b, 0x6c, 0x5, 0x12, 0xa, 0x2, 0x6c, 0x11, 
    0x3, 0x2, 0x2, 0x2, 0x6d, 0x70, 0x5, 0x14, 0xb, 0x2, 0x6e, 0x70, 0x5, 
    0x16, 0xc, 0x2, 0x6f, 0x6d, 0x3, 0x2, 0x2, 0x2, 0x6f, 0x6e, 0x3, 0x2, 
    0x2, 0x2, 0x70, 0x13, 0x3, 0x2, 0x2, 0x2, 0x71, 0x72, 0x7, 0x8, 0x2, 
    0x2, 0x72, 0x73, 0x5, 0xe, 0x8, 0x2, 0x73, 0x15, 0x3, 0x2, 0x2, 0x2, 
    0x74, 0x75, 0x7, 0x9, 0x2, 0x2, 0x75, 0x76, 0x5, 0xe, 0x8, 0x2, 0x76, 
    0x17, 0x3, 0x2, 0x2, 0x2, 0x77, 0x78, 0x5, 0x26, 0x14, 0x2, 0x78, 0x79, 
    0x7, 0xd, 0x2, 0x2, 0x79, 0x7a, 0x5, 0x26, 0x14, 0x2, 0x7a, 0x19, 0x3, 
    0x2, 0x2, 0x2, 0x7b, 0x7d, 0x7, 0xb, 0x2, 0x2, 0x7c, 0x7b, 0x3, 0x2, 
    0x2, 0x2, 0x7d, 0x7e, 0x3, 0x2, 0x2, 0x2, 0x7e, 0x7c, 0x3, 0x2, 0x2, 
    0x2, 0x7e, 0x7f, 0x3, 0x2, 0x2, 0x2, 0x7f, 0x80, 0x3, 0x2, 0x2, 0x2, 
    0x80, 0x81, 0x5, 0x18, 0xd, 0x2, 0x81, 0x1b, 0x3, 0x2, 0x2, 0x2, 0x82, 
    0x87, 0x5, 0x26, 0x14, 0x2, 0x83, 0x84, 0x7, 0x18, 0x2, 0x2, 0x84, 0x86, 
    0x5, 0x26, 0x14, 0x2, 0x85, 0x83, 0x3, 0x2, 0x2, 0x2, 0x86, 0x89, 0x3, 
    0x2, 0x2, 0x2, 0x87, 0x85, 0x3, 0x2, 0x2, 0x2, 0x87, 0x88, 0x3, 0x2, 
    0x2, 0x2, 0x88, 0x1d, 0x3, 0x2, 0x2, 0x2, 0x89, 0x87, 0x3, 0x2, 0x2, 
    0x2, 0x8a, 0x8f, 0x7, 0x4, 0x2, 0x2, 0x8b, 0x8c, 0x7, 0x18, 0x2, 0x2, 
    0x8c, 0x8e, 0x7, 0x4, 0x2, 0x2, 0x8d, 0x8b, 0x3, 0x2, 0x2, 0x2, 0x8e, 
    0x91, 0x3, 0x2, 0x2, 0x2, 0x8f, 0x8d, 0x3, 0x2, 0x2, 0x2, 0x8f, 0x90, 
    0x3, 0x2, 0x2, 0x2, 0x90, 0x1f, 0x3, 0x2, 0x2, 0x2, 0x91, 0x8f, 0x3, 
    0x2, 0x2, 0x2, 0x92, 0x93, 0x9, 0x2, 0x2, 0x2, 0x93, 0x21, 0x3, 0x2, 
    0x2, 0x2, 0x94, 0x95, 0x7, 0xa, 0x2, 0x2, 0x95, 0x96, 0x7, 0x15, 0x2, 
    0x2, 0x96, 0x97, 0x5, 0x1e, 0x10, 0x2, 0x97, 0x98, 0x7, 0x16, 0x2, 0x2, 
    0x98, 0x99, 0x7, 0x17, 0x2, 0x2, 0x99, 0x9a, 0x7, 0xa, 0x2, 0x2, 0x9a, 
    0x23, 0x3, 0x2, 0x2, 0x2, 0x9b, 0x9c, 0x7, 0xe, 0x2, 0x2, 0x9c, 0x9d, 
    0x5, 0x22, 0x12, 0x2, 0x9d, 0x25, 0x3, 0x2, 0x2, 0x2, 0x9e, 0x9f, 0x8, 
    0x14, 0x1, 0x2, 0x9f, 0xb0, 0x5, 0x20, 0x11, 0x2, 0xa0, 0xb0, 0x7, 0xa, 
    0x2, 0x2, 0xa1, 0xb0, 0x5, 0x24, 0x13, 0x2, 0xa2, 0xb0, 0x5, 0x22, 0x12, 
    0x2, 0xa3, 0xa4, 0x7, 0x11, 0x2, 0x2, 0xa4, 0xa5, 0x5, 0x26, 0x14, 0x2, 
    0xa5, 0xa6, 0x7, 0x12, 0x2, 0x2, 0xa6, 0xb0, 0x3, 0x2, 0x2, 0x2, 0xa7, 
    0xa8, 0x9, 0x3, 0x2, 0x2, 0xa8, 0xb0, 0x5, 0x26, 0x14, 0x8, 0xa9, 0xaa, 
    0x7, 0xa, 0x2, 0x2, 0xaa, 0xac, 0x7, 0x11, 0x2, 0x2, 0xab, 0xad, 0x5, 
    0x1c, 0xf, 0x2, 0xac, 0xab, 0x3, 0x2, 0x2, 0x2, 0xac, 0xad, 0x3, 0x2, 
    0x2, 0x2, 0xad, 0xae, 0x3, 0x2, 0x2, 0x2, 0xae, 0xb0, 0x7, 0x12, 0x2, 
    0x2, 0xaf, 0x9e, 0x3, 0x2, 0x2, 0x2, 0xaf, 0xa0, 0x3, 0x2, 0x2, 0x2, 
    0xaf, 0xa1, 0x3, 0x2, 0x2, 0x2, 0xaf, 0xa2, 0x3, 0x2, 0x2, 0x2, 0xaf, 
    0xa3, 0x3, 0x2, 0x2, 0x2, 0xaf, 0xa7, 0x3, 0x2, 0x2, 0x2, 0xaf, 0xa9, 
    0x3, 0x2, 0x2, 0x2, 0xb0, 0xc2, 0x3, 0x2, 0x2, 0x2, 0xb1, 0xb2, 0xc, 
    0x9, 0x2, 0x2, 0xb2, 0xb3, 0x7, 0x1d, 0x2, 0x2, 0xb3, 0xc1, 0x5, 0x26, 
    0x14, 0x9, 0xb4, 0xb5, 0xc, 0x7, 0x2, 0x2, 0xb5, 0xb6, 0x9, 0x4, 0x2, 
    0x2, 0xb6, 0xc1, 0x5, 0x26, 0x14, 0x8, 0xb7, 0xb8, 0xc, 0x6, 0x2, 0x2, 
    0xb8, 0xb9, 0x9, 0x3, 0x2, 0x2, 0xb9, 0xc1, 0x5, 0x26, 0x14, 0x7, 0xba, 
    0xbb, 0xc, 0x5, 0x2, 0x2, 0xbb, 0xbc, 0x9, 0x5, 0x2, 0x2, 0xbc, 0xc1, 
    0x5, 0x26, 0x14, 0x6, 0xbd, 0xbe, 0xc, 0x4, 0x2, 0x2, 0xbe, 0xbf, 0x9, 
    0x6, 0x2, 0x2, 0xbf, 0xc1, 0x5, 0x26, 0x14, 0x5, 0xc0, 0xb1, 0x3, 0x2, 
    0x2, 0x2, 0xc0, 0xb4, 0x3, 0x2, 0x2, 0x2, 0xc0, 0xb7, 0x3, 0x2, 0x2, 
    0x2, 0xc0, 0xba, 0x3, 0x2, 0x2, 0x2, 0xc0, 0xbd, 0x3, 0x2, 0x2, 0x2, 
    0xc1, 0xc4, 0x3, 0x2, 0x2, 0x2, 0xc2, 0xc0, 0x3, 0x2, 0x2, 0x2, 0xc2, 
    0xc3, 0x3, 0x2, 0x2, 0x2, 0xc3, 0x27, 0x3, 0x2, 0x2, 0x2, 0xc4, 0xc2, 
    0x3, 0x2, 0x2, 0x2, 0x13, 0x29, 0x2d, 0x32, 0x44, 0x4a, 0x53, 0x59, 
    0x63, 0x69, 0x6f, 0x7e, 0x87, 0x8f, 0xac, 0xaf, 0xc0, 0xc2, 
  };

  atn::ATNDeserializer deserializer;
  _atn = deserializer.deserialize(_serializedATN);

  size_t count = _atn.getNumberOfDecisions();
  _decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    _decisionToDFA.emplace_back(_atn.getDecisionState(i), i);
  }
}

EquationsParser::Initializer EquationsParser::_init;
