#pragma once

#include "antlr4-runtime.h"
#include "Generated/EquationsLexer.h"
#include "Generated/EquationsParser.h"
#include "Generated/EquationsBaseVisitor.h"
#include "ASTNodes.h"


class AntlrASTErrorListener : public antlr4::ANTLRErrorListener
{
    CASTTreeBase* pTree;
    std::string SourceText;
public:
    AntlrASTErrorListener(CASTTreeBase* pASTTree, std::string_view Source) : pTree(pASTTree), SourceText(Source)
    {

    }

    void reportAmbiguity(antlr4::Parser* recognizer, const antlr4::dfa::DFA& dfa, size_t startIndex, size_t stopIndex, bool exact,
        const antlrcpp::BitSet& ambigAlts, antlr4::atn::ATNConfigSet* configs)
    {

    }
    virtual void syntaxError(antlr4::Recognizer* recognizer, antlr4::Token* offendingSymbol, size_t line,
        size_t charPositionInLine, const std::string& msg, std::exception_ptr e)
    {
        // иногда offendingSymbol == nullptr
        // поэтому для описания ошибки используем
        // просто текст исходника SourceText
        pTree->Error(ParserError(fmt::format(u8"синтаксическая ошибка {}", msg),
            SourceText,
            offendingSymbol ? offendingSymbol->getText() : "",
            line, charPositionInLine));
        /*
        auto stk = static_cast<antlr4::Parser*>(recognizer)->getRuleInvocationStack();
        for (auto& sti : stk)
        {
            std::cout << u8"rule stack: " << sti << std::endl;
        }
        */
    }

    virtual void reportContextSensitivity(antlr4::Parser* recognizer, const antlr4::dfa::DFA& dfa, size_t startIndex, size_t stopIndex,
        size_t prediction, antlr4::atn::ATNConfigSet* configs)
    {

    }

    virtual void reportAttemptingFullContext(antlr4::Parser* recognizer, const antlr4::dfa::DFA& dfa, size_t startIndex, size_t stopIndex,
        const antlrcpp::BitSet& conflictingAlts, antlr4::atn::ATNConfigSet* configs)
    {

    }

    static std::string GetSourceLineFromSource(std::string_view source, size_t line);
    static std::string ParserError(std::string_view What, std::string_view FullSourceText, std::string_view Offending, size_t line, size_t pos);
};
