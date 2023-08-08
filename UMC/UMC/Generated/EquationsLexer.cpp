
// Generated from /home/eugene/projects/DFW2/UMC/UMC/Equations.g4 by ANTLR 4.13.0


#include "EquationsLexer.h"


using namespace antlr4;



using namespace antlr4;

namespace {

struct EquationsLexerStaticData final {
  EquationsLexerStaticData(std::vector<std::string> ruleNames,
                          std::vector<std::string> channelNames,
                          std::vector<std::string> modeNames,
                          std::vector<std::string> literalNames,
                          std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), channelNames(std::move(channelNames)),
        modeNames(std::move(modeNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  EquationsLexerStaticData(const EquationsLexerStaticData&) = delete;
  EquationsLexerStaticData(EquationsLexerStaticData&&) = delete;
  EquationsLexerStaticData& operator=(const EquationsLexerStaticData&) = delete;
  EquationsLexerStaticData& operator=(EquationsLexerStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> channelNames;
  const std::vector<std::string> modeNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag equationslexerLexerOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
EquationsLexerStaticData *equationslexerLexerStaticData = nullptr;

void equationslexerLexerInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (equationslexerLexerStaticData != nullptr) {
    return;
  }
#else
  assert(equationslexerLexerStaticData == nullptr);
#endif
  auto staticData = std::make_unique<EquationsLexerStaticData>(
    std::vector<std::string>{
      "FLOAT", "INTEGER", "DIGITS", "Exponent", "MAIN", "INIT", "VARS", 
      "CONST", "EXTERNAL", "VARIABLE", "NEWLINE", "LINE_COMMENT", "EQUAL", 
      "BAR", "AND", "OR", "LB", "RB", "LCB", "RCB", "LSB", "RSB", "DOT", 
      "COMMA", "PLUS", "MINUS", "MUL", "DIV", "POW", "HIGH", "LOW", "WS"
    },
    std::vector<std::string>{
      "DEFAULT_TOKEN_CHANNEL", "HIDDEN"
    },
    std::vector<std::string>{
      "DEFAULT_MODE"
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
  	4,0,30,239,6,-1,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,
  	6,2,7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,
  	7,14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,
  	7,21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,
  	7,28,2,29,7,29,2,30,7,30,2,31,7,31,1,0,1,0,1,0,3,0,69,8,0,1,0,1,0,1,0,
  	1,0,1,0,1,0,1,0,1,0,3,0,79,8,0,3,0,81,8,0,1,0,3,0,84,8,0,3,0,86,8,0,1,
  	1,1,1,1,2,4,2,91,8,2,11,2,12,2,92,1,3,1,3,3,3,97,8,3,1,3,1,3,1,4,5,4,
  	102,8,4,10,4,12,4,105,9,4,1,4,1,4,1,4,1,4,1,4,1,4,5,4,113,8,4,10,4,12,
  	4,116,9,4,1,5,5,5,119,8,5,10,5,12,5,122,9,5,1,5,1,5,1,5,1,5,1,5,1,5,5,
  	5,130,8,5,10,5,12,5,133,9,5,1,6,5,6,136,8,6,10,6,12,6,139,9,6,1,6,1,6,
  	1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,1,6,5,6,152,8,6,10,6,12,6,155,9,6,1,7,
  	1,7,1,7,1,7,1,7,1,7,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,9,1,9,5,9,174,
  	8,9,10,9,12,9,177,9,9,1,10,3,10,180,8,10,1,10,1,10,1,11,1,11,1,11,1,11,
  	5,11,188,8,11,10,11,12,11,191,9,11,1,11,1,11,1,12,1,12,1,13,1,13,1,14,
  	1,14,1,15,1,15,1,16,1,16,1,17,1,17,1,18,1,18,1,19,1,19,1,20,1,20,1,21,
  	1,21,1,22,1,22,1,23,1,23,1,24,1,24,1,25,1,25,1,26,1,26,1,27,1,27,1,28,
  	1,28,1,29,1,29,1,30,1,30,1,31,4,31,234,8,31,11,31,12,31,235,1,31,1,31,
  	0,0,32,1,1,3,2,5,0,7,0,9,3,11,4,13,5,15,6,17,7,19,8,21,9,23,10,25,11,
  	27,12,29,13,31,14,33,15,35,16,37,17,39,18,41,19,43,20,45,21,47,22,49,
  	23,51,24,53,25,55,26,57,27,59,28,61,29,63,30,1,0,7,1,0,48,57,2,0,69,69,
  	101,101,2,0,43,43,45,45,3,0,65,90,95,95,97,122,4,0,48,57,65,90,95,95,
  	97,122,2,0,10,10,13,13,2,0,9,9,32,32,254,0,1,1,0,0,0,0,3,1,0,0,0,0,9,
  	1,0,0,0,0,11,1,0,0,0,0,13,1,0,0,0,0,15,1,0,0,0,0,17,1,0,0,0,0,19,1,0,
  	0,0,0,21,1,0,0,0,0,23,1,0,0,0,0,25,1,0,0,0,0,27,1,0,0,0,0,29,1,0,0,0,
  	0,31,1,0,0,0,0,33,1,0,0,0,0,35,1,0,0,0,0,37,1,0,0,0,0,39,1,0,0,0,0,41,
  	1,0,0,0,0,43,1,0,0,0,0,45,1,0,0,0,0,47,1,0,0,0,0,49,1,0,0,0,0,51,1,0,
  	0,0,0,53,1,0,0,0,0,55,1,0,0,0,0,57,1,0,0,0,0,59,1,0,0,0,0,61,1,0,0,0,
  	0,63,1,0,0,0,1,85,1,0,0,0,3,87,1,0,0,0,5,90,1,0,0,0,7,94,1,0,0,0,9,103,
  	1,0,0,0,11,120,1,0,0,0,13,137,1,0,0,0,15,156,1,0,0,0,17,162,1,0,0,0,19,
  	171,1,0,0,0,21,179,1,0,0,0,23,183,1,0,0,0,25,194,1,0,0,0,27,196,1,0,0,
  	0,29,198,1,0,0,0,31,200,1,0,0,0,33,202,1,0,0,0,35,204,1,0,0,0,37,206,
  	1,0,0,0,39,208,1,0,0,0,41,210,1,0,0,0,43,212,1,0,0,0,45,214,1,0,0,0,47,
  	216,1,0,0,0,49,218,1,0,0,0,51,220,1,0,0,0,53,222,1,0,0,0,55,224,1,0,0,
  	0,57,226,1,0,0,0,59,228,1,0,0,0,61,230,1,0,0,0,63,233,1,0,0,0,65,66,3,
  	45,22,0,66,68,3,5,2,0,67,69,3,7,3,0,68,67,1,0,0,0,68,69,1,0,0,0,69,86,
  	1,0,0,0,70,71,3,5,2,0,71,72,3,45,22,0,72,73,3,7,3,0,73,86,1,0,0,0,74,
  	83,3,5,2,0,75,80,3,45,22,0,76,78,3,5,2,0,77,79,3,7,3,0,78,77,1,0,0,0,
  	78,79,1,0,0,0,79,81,1,0,0,0,80,76,1,0,0,0,80,81,1,0,0,0,81,84,1,0,0,0,
  	82,84,3,7,3,0,83,75,1,0,0,0,83,82,1,0,0,0,84,86,1,0,0,0,85,65,1,0,0,0,
  	85,70,1,0,0,0,85,74,1,0,0,0,86,2,1,0,0,0,87,88,3,5,2,0,88,4,1,0,0,0,89,
  	91,7,0,0,0,90,89,1,0,0,0,91,92,1,0,0,0,92,90,1,0,0,0,92,93,1,0,0,0,93,
  	6,1,0,0,0,94,96,7,1,0,0,95,97,7,2,0,0,96,95,1,0,0,0,96,97,1,0,0,0,97,
  	98,1,0,0,0,98,99,3,5,2,0,99,8,1,0,0,0,100,102,3,21,10,0,101,100,1,0,0,
  	0,102,105,1,0,0,0,103,101,1,0,0,0,103,104,1,0,0,0,104,106,1,0,0,0,105,
  	103,1,0,0,0,106,107,5,109,0,0,107,108,5,97,0,0,108,109,5,105,0,0,109,
  	110,5,110,0,0,110,114,1,0,0,0,111,113,3,21,10,0,112,111,1,0,0,0,113,116,
  	1,0,0,0,114,112,1,0,0,0,114,115,1,0,0,0,115,10,1,0,0,0,116,114,1,0,0,
  	0,117,119,3,21,10,0,118,117,1,0,0,0,119,122,1,0,0,0,120,118,1,0,0,0,120,
  	121,1,0,0,0,121,123,1,0,0,0,122,120,1,0,0,0,123,124,5,105,0,0,124,125,
  	5,110,0,0,125,126,5,105,0,0,126,127,5,116,0,0,127,131,1,0,0,0,128,130,
  	3,21,10,0,129,128,1,0,0,0,130,133,1,0,0,0,131,129,1,0,0,0,131,132,1,0,
  	0,0,132,12,1,0,0,0,133,131,1,0,0,0,134,136,3,21,10,0,135,134,1,0,0,0,
  	136,139,1,0,0,0,137,135,1,0,0,0,137,138,1,0,0,0,138,140,1,0,0,0,139,137,
  	1,0,0,0,140,141,5,118,0,0,141,142,5,97,0,0,142,143,5,114,0,0,143,144,
  	5,105,0,0,144,145,5,97,0,0,145,146,5,98,0,0,146,147,5,108,0,0,147,148,
  	5,101,0,0,148,149,5,115,0,0,149,153,1,0,0,0,150,152,3,21,10,0,151,150,
  	1,0,0,0,152,155,1,0,0,0,153,151,1,0,0,0,153,154,1,0,0,0,154,14,1,0,0,
  	0,155,153,1,0,0,0,156,157,5,99,0,0,157,158,5,111,0,0,158,159,5,110,0,
  	0,159,160,5,115,0,0,160,161,5,116,0,0,161,16,1,0,0,0,162,163,5,101,0,
  	0,163,164,5,120,0,0,164,165,5,116,0,0,165,166,5,101,0,0,166,167,5,114,
  	0,0,167,168,5,110,0,0,168,169,5,97,0,0,169,170,5,108,0,0,170,18,1,0,0,
  	0,171,175,7,3,0,0,172,174,7,4,0,0,173,172,1,0,0,0,174,177,1,0,0,0,175,
  	173,1,0,0,0,175,176,1,0,0,0,176,20,1,0,0,0,177,175,1,0,0,0,178,180,5,
  	13,0,0,179,178,1,0,0,0,179,180,1,0,0,0,180,181,1,0,0,0,181,182,5,10,0,
  	0,182,22,1,0,0,0,183,184,5,47,0,0,184,185,5,47,0,0,185,189,1,0,0,0,186,
  	188,8,5,0,0,187,186,1,0,0,0,188,191,1,0,0,0,189,187,1,0,0,0,189,190,1,
  	0,0,0,190,192,1,0,0,0,191,189,1,0,0,0,192,193,6,11,0,0,193,24,1,0,0,0,
  	194,195,5,61,0,0,195,26,1,0,0,0,196,197,5,35,0,0,197,28,1,0,0,0,198,199,
  	5,38,0,0,199,30,1,0,0,0,200,201,5,124,0,0,201,32,1,0,0,0,202,203,5,40,
  	0,0,203,34,1,0,0,0,204,205,5,41,0,0,205,36,1,0,0,0,206,207,5,123,0,0,
  	207,38,1,0,0,0,208,209,5,125,0,0,209,40,1,0,0,0,210,211,5,91,0,0,211,
  	42,1,0,0,0,212,213,5,93,0,0,213,44,1,0,0,0,214,215,5,46,0,0,215,46,1,
  	0,0,0,216,217,5,44,0,0,217,48,1,0,0,0,218,219,5,43,0,0,219,50,1,0,0,0,
  	220,221,5,45,0,0,221,52,1,0,0,0,222,223,5,42,0,0,223,54,1,0,0,0,224,225,
  	5,47,0,0,225,56,1,0,0,0,226,227,5,94,0,0,227,58,1,0,0,0,228,229,5,62,
  	0,0,229,60,1,0,0,0,230,231,5,60,0,0,231,62,1,0,0,0,232,234,7,6,0,0,233,
  	232,1,0,0,0,234,235,1,0,0,0,235,233,1,0,0,0,235,236,1,0,0,0,236,237,1,
  	0,0,0,237,238,6,31,0,0,238,64,1,0,0,0,18,0,68,78,80,83,85,92,96,103,114,
  	120,131,137,153,175,179,189,235,1,6,0,0
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  equationslexerLexerStaticData = staticData.release();
}

}

EquationsLexer::EquationsLexer(CharStream *input) : Lexer(input) {
  EquationsLexer::initialize();
  _interpreter = new atn::LexerATNSimulator(this, *equationslexerLexerStaticData->atn, equationslexerLexerStaticData->decisionToDFA, equationslexerLexerStaticData->sharedContextCache);
}

EquationsLexer::~EquationsLexer() {
  delete _interpreter;
}

std::string EquationsLexer::getGrammarFileName() const {
  return "Equations.g4";
}

const std::vector<std::string>& EquationsLexer::getRuleNames() const {
  return equationslexerLexerStaticData->ruleNames;
}

const std::vector<std::string>& EquationsLexer::getChannelNames() const {
  return equationslexerLexerStaticData->channelNames;
}

const std::vector<std::string>& EquationsLexer::getModeNames() const {
  return equationslexerLexerStaticData->modeNames;
}

const dfa::Vocabulary& EquationsLexer::getVocabulary() const {
  return equationslexerLexerStaticData->vocabulary;
}

antlr4::atn::SerializedATNView EquationsLexer::getSerializedATN() const {
  return equationslexerLexerStaticData->serializedATN;
}

const atn::ATN& EquationsLexer::getATN() const {
  return *equationslexerLexerStaticData->atn;
}




void EquationsLexer::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  equationslexerLexerInitialize();
#else
  ::antlr4::internal::call_once(equationslexerLexerOnceFlag, equationslexerLexerInitialize);
#endif
}
