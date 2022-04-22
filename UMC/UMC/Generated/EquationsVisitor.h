
// Generated from /home/bug/projects/DFW2/DFW2/UMC/UMC/Equations.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"
#include "EquationsParser.h"



/**
 * This class defines an abstract visitor for a parse tree
 * produced by EquationsParser.
 */
class  EquationsVisitor : public antlr4::tree::AbstractParseTreeVisitor {
public:

  /**
   * Visit parse trees produced by EquationsParser.
   */
    virtual antlrcpp::Any visitInput(EquationsParser::InputContext *context) = 0;

    virtual antlrcpp::Any visitMain(EquationsParser::MainContext *context) = 0;

    virtual antlrcpp::Any visitInit(EquationsParser::InitContext *context) = 0;

    virtual antlrcpp::Any visitVariables(EquationsParser::VariablesContext *context) = 0;

    virtual antlrcpp::Any visitVardefines(EquationsParser::VardefinesContext *context) = 0;

    virtual antlrcpp::Any visitEquationsys(EquationsParser::EquationsysContext *context) = 0;

    virtual antlrcpp::Any visitVarlist(EquationsParser::VarlistContext *context) = 0;

    virtual antlrcpp::Any visitVardefineline(EquationsParser::VardefinelineContext *context) = 0;

    virtual antlrcpp::Any visitVardefine(EquationsParser::VardefineContext *context) = 0;

    virtual antlrcpp::Any visitConstvardefine(EquationsParser::ConstvardefineContext *context) = 0;

    virtual antlrcpp::Any visitExtvardefine(EquationsParser::ExtvardefineContext *context) = 0;

    virtual antlrcpp::Any visitEquation(EquationsParser::EquationContext *context) = 0;

    virtual antlrcpp::Any visitEquationline(EquationsParser::EquationlineContext *context) = 0;

    virtual antlrcpp::Any visitExpressionlist(EquationsParser::ExpressionlistContext *context) = 0;

    virtual antlrcpp::Any visitIntlist(EquationsParser::IntlistContext *context) = 0;

    virtual antlrcpp::Any visitConstvalue(EquationsParser::ConstvalueContext *context) = 0;

    virtual antlrcpp::Any visitModlink(EquationsParser::ModlinkContext *context) = 0;

    virtual antlrcpp::Any visitModlinkbase(EquationsParser::ModlinkbaseContext *context) = 0;

    virtual antlrcpp::Any visitAndor(EquationsParser::AndorContext *context) = 0;

    virtual antlrcpp::Any visitModellink(EquationsParser::ModellinkContext *context) = 0;

    virtual antlrcpp::Any visitHighlow(EquationsParser::HighlowContext *context) = 0;

    virtual antlrcpp::Any visitModellinkbase(EquationsParser::ModellinkbaseContext *context) = 0;

    virtual antlrcpp::Any visitFunction(EquationsParser::FunctionContext *context) = 0;

    virtual antlrcpp::Any visitVariable(EquationsParser::VariableContext *context) = 0;

    virtual antlrcpp::Any visitPow(EquationsParser::PowContext *context) = 0;

    virtual antlrcpp::Any visitRealconst(EquationsParser::RealconstContext *context) = 0;

    virtual antlrcpp::Any visitUnary(EquationsParser::UnaryContext *context) = 0;

    virtual antlrcpp::Any visitInfix(EquationsParser::InfixContext *context) = 0;

    virtual antlrcpp::Any visitBraces(EquationsParser::BracesContext *context) = 0;


};

