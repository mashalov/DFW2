
// Generated from D:\source\repos\DFW2\UMC\UMC\Equations.g4 by ANTLR 4.13.0


#include "EquationsVisitor.h"

#include "EquationsParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct EquationsParserStaticData final {
  EquationsParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  EquationsParserStaticData(const EquationsParserStaticData&) = delete;
  EquationsParserStaticData(EquationsParserStaticData&&) = delete;
  EquationsParserStaticData& operator=(const EquationsParserStaticData&) = delete;
  EquationsParserStaticData& operator=(EquationsParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag equationsParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
EquationsParserStaticData *equationsParserStaticData = nullptr;

void equationsParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (equationsParserStaticData != nullptr) {
    return;
  }
#else
  assert(equationsParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<EquationsParserStaticData>(
    std::vector<std::string>{
      "input", "main", "init", "variables", "vardefines", "equationsys", 
      "varlist", "vardefineline", "vardefine", "constvardefine", "extvardefine", 
      "equation", "equationline", "expressionlist", "intlist", "constvalue", 
      "modlink", "modlinkbase", "expression"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "'const'", "'external'", "", "", "", "'='", 
      "'#'", "'&'", "'|'", "'('", "')'", "'{'", "'}'", "'['", "']'", "'.'", 
      "','", "'+'", "'-'", "'*'", "'/'", "'^'", "'>'", "'<'"
    },
    std::vector<std::string>{
      "", "FLOAT", "INTEGER", "MAIN", "INIT", "VARS", "CONST", "EXTERNAL", 
      "VARIABLE", "NEWLINE", "LINE_COMMENT", "EQUAL", "BAR", "AND", "OR", 
      "LB", "RB", "LCB", "RCB", "LSB", "RSB", "DOT", "COMMA", "PLUS", "MINUS", 
      "MUL", "DIV", "POW", "HIGH", "LOW", "WS"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,30,196,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,1,0,3,0,40,8,0,1,0,1,0,3,0,
  	44,8,0,1,0,5,0,47,8,0,10,0,12,0,50,9,0,1,0,1,0,1,1,1,1,1,1,1,2,1,2,1,
  	2,1,3,1,3,1,3,1,4,1,4,5,4,65,8,4,10,4,12,4,68,9,4,1,4,5,4,71,8,4,10,4,
  	12,4,74,9,4,1,4,1,4,1,5,1,5,5,5,80,8,5,10,5,12,5,83,9,5,1,5,5,5,86,8,
  	5,10,5,12,5,89,9,5,1,5,1,5,1,6,1,6,1,6,5,6,96,8,6,10,6,12,6,99,9,6,1,
  	7,4,7,102,8,7,11,7,12,7,103,1,7,1,7,1,8,1,8,3,8,110,8,8,1,9,1,9,1,9,1,
  	10,1,10,1,10,1,11,1,11,1,11,1,11,1,12,4,12,123,8,12,11,12,12,12,124,1,
  	12,1,12,1,13,1,13,1,13,5,13,132,8,13,10,13,12,13,135,9,13,1,14,1,14,1,
  	14,5,14,140,8,14,10,14,12,14,143,9,14,1,15,1,15,1,16,1,16,1,16,1,16,1,
  	16,1,16,1,16,1,17,1,17,1,17,1,18,1,18,1,18,1,18,1,18,1,18,1,18,1,18,1,
  	18,1,18,1,18,1,18,1,18,1,18,3,18,171,8,18,1,18,3,18,174,8,18,1,18,1,18,
  	1,18,1,18,1,18,1,18,1,18,1,18,1,18,1,18,1,18,1,18,1,18,1,18,1,18,5,18,
  	191,8,18,10,18,12,18,194,9,18,1,18,0,1,36,19,0,2,4,6,8,10,12,14,16,18,
  	20,22,24,26,28,30,32,34,36,0,5,1,0,1,2,1,0,23,24,1,0,25,26,1,0,28,29,
  	1,0,13,14,201,0,39,1,0,0,0,2,53,1,0,0,0,4,56,1,0,0,0,6,59,1,0,0,0,8,62,
  	1,0,0,0,10,77,1,0,0,0,12,92,1,0,0,0,14,101,1,0,0,0,16,109,1,0,0,0,18,
  	111,1,0,0,0,20,114,1,0,0,0,22,117,1,0,0,0,24,122,1,0,0,0,26,128,1,0,0,
  	0,28,136,1,0,0,0,30,144,1,0,0,0,32,146,1,0,0,0,34,153,1,0,0,0,36,173,
  	1,0,0,0,38,40,3,6,3,0,39,38,1,0,0,0,39,40,1,0,0,0,40,41,1,0,0,0,41,43,
  	3,2,1,0,42,44,3,4,2,0,43,42,1,0,0,0,43,44,1,0,0,0,44,48,1,0,0,0,45,47,
  	5,9,0,0,46,45,1,0,0,0,47,50,1,0,0,0,48,46,1,0,0,0,48,49,1,0,0,0,49,51,
  	1,0,0,0,50,48,1,0,0,0,51,52,5,0,0,1,52,1,1,0,0,0,53,54,5,3,0,0,54,55,
  	3,10,5,0,55,3,1,0,0,0,56,57,5,4,0,0,57,58,3,10,5,0,58,5,1,0,0,0,59,60,
  	5,5,0,0,60,61,3,8,4,0,61,7,1,0,0,0,62,66,5,17,0,0,63,65,3,14,7,0,64,63,
  	1,0,0,0,65,68,1,0,0,0,66,64,1,0,0,0,66,67,1,0,0,0,67,72,1,0,0,0,68,66,
  	1,0,0,0,69,71,5,9,0,0,70,69,1,0,0,0,71,74,1,0,0,0,72,70,1,0,0,0,72,73,
  	1,0,0,0,73,75,1,0,0,0,74,72,1,0,0,0,75,76,5,18,0,0,76,9,1,0,0,0,77,81,
  	5,17,0,0,78,80,3,24,12,0,79,78,1,0,0,0,80,83,1,0,0,0,81,79,1,0,0,0,81,
  	82,1,0,0,0,82,87,1,0,0,0,83,81,1,0,0,0,84,86,5,9,0,0,85,84,1,0,0,0,86,
  	89,1,0,0,0,87,85,1,0,0,0,87,88,1,0,0,0,88,90,1,0,0,0,89,87,1,0,0,0,90,
  	91,5,18,0,0,91,11,1,0,0,0,92,97,5,8,0,0,93,94,5,22,0,0,94,96,5,8,0,0,
  	95,93,1,0,0,0,96,99,1,0,0,0,97,95,1,0,0,0,97,98,1,0,0,0,98,13,1,0,0,0,
  	99,97,1,0,0,0,100,102,5,9,0,0,101,100,1,0,0,0,102,103,1,0,0,0,103,101,
  	1,0,0,0,103,104,1,0,0,0,104,105,1,0,0,0,105,106,3,16,8,0,106,15,1,0,0,
  	0,107,110,3,18,9,0,108,110,3,20,10,0,109,107,1,0,0,0,109,108,1,0,0,0,
  	110,17,1,0,0,0,111,112,5,6,0,0,112,113,3,12,6,0,113,19,1,0,0,0,114,115,
  	5,7,0,0,115,116,3,12,6,0,116,21,1,0,0,0,117,118,3,36,18,0,118,119,5,11,
  	0,0,119,120,3,36,18,0,120,23,1,0,0,0,121,123,5,9,0,0,122,121,1,0,0,0,
  	123,124,1,0,0,0,124,122,1,0,0,0,124,125,1,0,0,0,125,126,1,0,0,0,126,127,
  	3,22,11,0,127,25,1,0,0,0,128,133,3,36,18,0,129,130,5,22,0,0,130,132,3,
  	36,18,0,131,129,1,0,0,0,132,135,1,0,0,0,133,131,1,0,0,0,133,134,1,0,0,
  	0,134,27,1,0,0,0,135,133,1,0,0,0,136,141,5,2,0,0,137,138,5,22,0,0,138,
  	140,5,2,0,0,139,137,1,0,0,0,140,143,1,0,0,0,141,139,1,0,0,0,141,142,1,
  	0,0,0,142,29,1,0,0,0,143,141,1,0,0,0,144,145,7,0,0,0,145,31,1,0,0,0,146,
  	147,5,8,0,0,147,148,5,19,0,0,148,149,3,28,14,0,149,150,5,20,0,0,150,151,
  	5,21,0,0,151,152,5,8,0,0,152,33,1,0,0,0,153,154,5,12,0,0,154,155,3,32,
  	16,0,155,35,1,0,0,0,156,157,6,18,-1,0,157,174,3,30,15,0,158,174,5,8,0,
  	0,159,174,3,34,17,0,160,174,3,32,16,0,161,162,5,15,0,0,162,163,3,36,18,
  	0,163,164,5,16,0,0,164,174,1,0,0,0,165,166,7,1,0,0,166,174,3,36,18,6,
  	167,168,5,8,0,0,168,170,5,15,0,0,169,171,3,26,13,0,170,169,1,0,0,0,170,
  	171,1,0,0,0,171,172,1,0,0,0,172,174,5,16,0,0,173,156,1,0,0,0,173,158,
  	1,0,0,0,173,159,1,0,0,0,173,160,1,0,0,0,173,161,1,0,0,0,173,165,1,0,0,
  	0,173,167,1,0,0,0,174,192,1,0,0,0,175,176,10,7,0,0,176,177,5,27,0,0,177,
  	191,3,36,18,7,178,179,10,5,0,0,179,180,7,2,0,0,180,191,3,36,18,6,181,
  	182,10,4,0,0,182,183,7,1,0,0,183,191,3,36,18,5,184,185,10,3,0,0,185,186,
  	7,3,0,0,186,191,3,36,18,4,187,188,10,2,0,0,188,189,7,4,0,0,189,191,3,
  	36,18,3,190,175,1,0,0,0,190,178,1,0,0,0,190,181,1,0,0,0,190,184,1,0,0,
  	0,190,187,1,0,0,0,191,194,1,0,0,0,192,190,1,0,0,0,192,193,1,0,0,0,193,
  	37,1,0,0,0,194,192,1,0,0,0,17,39,43,48,66,72,81,87,97,103,109,124,133,
  	141,170,173,190,192
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  equationsParserStaticData = staticData.release();
}

}

EquationsParser::EquationsParser(TokenStream *input) : EquationsParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

EquationsParser::EquationsParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  EquationsParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *equationsParserStaticData->atn, equationsParserStaticData->decisionToDFA, equationsParserStaticData->sharedContextCache, options);
}

EquationsParser::~EquationsParser() {
  delete _interpreter;
}

const atn::ATN& EquationsParser::getATN() const {
  return *equationsParserStaticData->atn;
}

std::string EquationsParser::getGrammarFileName() const {
  return "Equations.g4";
}

const std::vector<std::string>& EquationsParser::getRuleNames() const {
  return equationsParserStaticData->ruleNames;
}

const dfa::Vocabulary& EquationsParser::getVocabulary() const {
  return equationsParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView EquationsParser::getSerializedATN() const {
  return equationsParserStaticData->serializedATN;
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


std::any EquationsParser::InputContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitInput(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::InputContext* EquationsParser::input() {
  InputContext *_localctx = _tracker.createInstance<InputContext>(_ctx, getState());
  enterRule(_localctx, 0, EquationsParser::RuleInput);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::MainContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitMain(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::MainContext* EquationsParser::main() {
  MainContext *_localctx = _tracker.createInstance<MainContext>(_ctx, getState());
  enterRule(_localctx, 2, EquationsParser::RuleMain);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::InitContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitInit(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::InitContext* EquationsParser::init() {
  InitContext *_localctx = _tracker.createInstance<InitContext>(_ctx, getState());
  enterRule(_localctx, 4, EquationsParser::RuleInit);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::VariablesContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitVariables(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::VariablesContext* EquationsParser::variables() {
  VariablesContext *_localctx = _tracker.createInstance<VariablesContext>(_ctx, getState());
  enterRule(_localctx, 6, EquationsParser::RuleVariables);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::VardefinesContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitVardefines(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::VardefinesContext* EquationsParser::vardefines() {
  VardefinesContext *_localctx = _tracker.createInstance<VardefinesContext>(_ctx, getState());
  enterRule(_localctx, 8, EquationsParser::RuleVardefines);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::EquationsysContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitEquationsys(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::EquationsysContext* EquationsParser::equationsys() {
  EquationsysContext *_localctx = _tracker.createInstance<EquationsysContext>(_ctx, getState());
  enterRule(_localctx, 10, EquationsParser::RuleEquationsys);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::VarlistContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitVarlist(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::VarlistContext* EquationsParser::varlist() {
  VarlistContext *_localctx = _tracker.createInstance<VarlistContext>(_ctx, getState());
  enterRule(_localctx, 12, EquationsParser::RuleVarlist);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::VardefinelineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitVardefineline(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::VardefinelineContext* EquationsParser::vardefineline() {
  VardefinelineContext *_localctx = _tracker.createInstance<VardefinelineContext>(_ctx, getState());
  enterRule(_localctx, 14, EquationsParser::RuleVardefineline);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::VardefineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitVardefine(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::VardefineContext* EquationsParser::vardefine() {
  VardefineContext *_localctx = _tracker.createInstance<VardefineContext>(_ctx, getState());
  enterRule(_localctx, 16, EquationsParser::RuleVardefine);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::ConstvardefineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitConstvardefine(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::ConstvardefineContext* EquationsParser::constvardefine() {
  ConstvardefineContext *_localctx = _tracker.createInstance<ConstvardefineContext>(_ctx, getState());
  enterRule(_localctx, 18, EquationsParser::RuleConstvardefine);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::ExtvardefineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitExtvardefine(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::ExtvardefineContext* EquationsParser::extvardefine() {
  ExtvardefineContext *_localctx = _tracker.createInstance<ExtvardefineContext>(_ctx, getState());
  enterRule(_localctx, 20, EquationsParser::RuleExtvardefine);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::EquationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitEquation(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::EquationContext* EquationsParser::equation() {
  EquationContext *_localctx = _tracker.createInstance<EquationContext>(_ctx, getState());
  enterRule(_localctx, 22, EquationsParser::RuleEquation);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(117);
    antlrcpp::downCast<EquationContext *>(_localctx)->left = expression(0);
    setState(118);
    match(EquationsParser::EQUAL);
    setState(119);
    antlrcpp::downCast<EquationContext *>(_localctx)->right = expression(0);
   
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


std::any EquationsParser::EquationlineContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitEquationline(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::EquationlineContext* EquationsParser::equationline() {
  EquationlineContext *_localctx = _tracker.createInstance<EquationlineContext>(_ctx, getState());
  enterRule(_localctx, 24, EquationsParser::RuleEquationline);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::ExpressionlistContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitExpressionlist(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::ExpressionlistContext* EquationsParser::expressionlist() {
  ExpressionlistContext *_localctx = _tracker.createInstance<ExpressionlistContext>(_ctx, getState());
  enterRule(_localctx, 26, EquationsParser::RuleExpressionlist);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::IntlistContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitIntlist(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::IntlistContext* EquationsParser::intlist() {
  IntlistContext *_localctx = _tracker.createInstance<IntlistContext>(_ctx, getState());
  enterRule(_localctx, 28, EquationsParser::RuleIntlist);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::ConstvalueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitConstvalue(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::ConstvalueContext* EquationsParser::constvalue() {
  ConstvalueContext *_localctx = _tracker.createInstance<ConstvalueContext>(_ctx, getState());
  enterRule(_localctx, 30, EquationsParser::RuleConstvalue);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::ModlinkContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitModlink(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::ModlinkContext* EquationsParser::modlink() {
  ModlinkContext *_localctx = _tracker.createInstance<ModlinkContext>(_ctx, getState());
  enterRule(_localctx, 32, EquationsParser::RuleModlink);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::ModlinkbaseContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<EquationsVisitor*>(visitor))
    return parserVisitor->visitModlinkbase(this);
  else
    return visitor->visitChildren(this);
}

EquationsParser::ModlinkbaseContext* EquationsParser::modlinkbase() {
  ModlinkbaseContext *_localctx = _tracker.createInstance<ModlinkbaseContext>(_ctx, getState());
  enterRule(_localctx, 34, EquationsParser::RuleModlinkbase);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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


std::any EquationsParser::AndorContext::accept(tree::ParseTreeVisitor *visitor) {
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


std::any EquationsParser::ModellinkContext::accept(tree::ParseTreeVisitor *visitor) {
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


std::any EquationsParser::HighlowContext::accept(tree::ParseTreeVisitor *visitor) {
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


std::any EquationsParser::ModellinkbaseContext::accept(tree::ParseTreeVisitor *visitor) {
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


std::any EquationsParser::FunctionContext::accept(tree::ParseTreeVisitor *visitor) {
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


std::any EquationsParser::VariableContext::accept(tree::ParseTreeVisitor *visitor) {
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


std::any EquationsParser::PowContext::accept(tree::ParseTreeVisitor *visitor) {
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


std::any EquationsParser::RealconstContext::accept(tree::ParseTreeVisitor *visitor) {
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


std::any EquationsParser::UnaryContext::accept(tree::ParseTreeVisitor *visitor) {
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


std::any EquationsParser::InfixContext::accept(tree::ParseTreeVisitor *visitor) {
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


std::any EquationsParser::BracesContext::accept(tree::ParseTreeVisitor *visitor) {
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

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
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
      antlrcpp::downCast<UnaryContext *>(_localctx)->op = _input->LT(1);
      _la = _input->LA(1);
      if (!(_la == EquationsParser::PLUS

      || _la == EquationsParser::MINUS)) {
        antlrcpp::downCast<UnaryContext *>(_localctx)->op = _errHandler->recoverInline(this);
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
      antlrcpp::downCast<FunctionContext *>(_localctx)->func = match(EquationsParser::VARIABLE);
      setState(168);
      match(EquationsParser::LB);
      setState(170);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 25202950) != 0)) {
        setState(169);
        expressionlist();
      }
      setState(172);
      match(EquationsParser::RB);
      break;
    }

    default:
      break;
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
          antlrcpp::downCast<InfixContext *>(_localctx)->op = _input->LT(1);
          _la = _input->LA(1);
          if (!(_la == EquationsParser::MUL

          || _la == EquationsParser::DIV)) {
            antlrcpp::downCast<InfixContext *>(_localctx)->op = _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(180);
          antlrcpp::downCast<InfixContext *>(_localctx)->right = expression(6);
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
          antlrcpp::downCast<InfixContext *>(_localctx)->op = _input->LT(1);
          _la = _input->LA(1);
          if (!(_la == EquationsParser::PLUS

          || _la == EquationsParser::MINUS)) {
            antlrcpp::downCast<InfixContext *>(_localctx)->op = _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(183);
          antlrcpp::downCast<InfixContext *>(_localctx)->right = expression(5);
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
          antlrcpp::downCast<HighlowContext *>(_localctx)->op = _input->LT(1);
          _la = _input->LA(1);
          if (!(_la == EquationsParser::HIGH

          || _la == EquationsParser::LOW)) {
            antlrcpp::downCast<HighlowContext *>(_localctx)->op = _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(186);
          antlrcpp::downCast<HighlowContext *>(_localctx)->right = expression(4);
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
          antlrcpp::downCast<AndorContext *>(_localctx)->op = _input->LT(1);
          _la = _input->LA(1);
          if (!(_la == EquationsParser::AND

          || _la == EquationsParser::OR)) {
            antlrcpp::downCast<AndorContext *>(_localctx)->op = _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(189);
          antlrcpp::downCast<AndorContext *>(_localctx)->right = expression(3);
          break;
        }

        default:
          break;
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
    case 18: return expressionSempred(antlrcpp::downCast<ExpressionContext *>(context), predicateIndex);

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

void EquationsParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  equationsParserInitialize();
#else
  ::antlr4::internal::call_once(equationsParserOnceFlag, equationsParserInitialize);
#endif
}
