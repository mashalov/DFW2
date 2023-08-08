
// Generated from /home/eugene/projects/DFW2/UMC/UMC/Equations.g4 by ANTLR 4.13.0

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
    virtual std::any visitInput(EquationsParser::InputContext *context) = 0;

    virtual std::any visitMain(EquationsParser::MainContext *context) = 0;

    virtual std::any visitInit(EquationsParser::InitContext *context) = 0;

    virtual std::any visitVariables(EquationsParser::VariablesContext *context) = 0;

    virtual std::any visitVardefines(EquationsParser::VardefinesContext *context) = 0;

    virtual std::any visitEquationsys(EquationsParser::EquationsysContext *context) = 0;

    virtual std::any visitVarlist(EquationsParser::VarlistContext *context) = 0;

    virtual std::any visitVardefineline(EquationsParser::VardefinelineContext *context) = 0;

    virtual std::any visitVardefine(EquationsParser::VardefineContext *context) = 0;

    virtual std::any visitConstvardefine(EquationsParser::ConstvardefineContext *context) = 0;

    virtual std::any visitExtvardefine(EquationsParser::ExtvardefineContext *context) = 0;

    virtual std::any visitEquation(EquationsParser::EquationContext *context) = 0;

    virtual std::any visitEquationline(EquationsParser::EquationlineContext *context) = 0;

    virtual std::any visitExpressionlist(EquationsParser::ExpressionlistContext *context) = 0;

    virtual std::any visitIntlist(EquationsParser::IntlistContext *context) = 0;

    virtual std::any visitConstvalue(EquationsParser::ConstvalueContext *context) = 0;

    virtual std::any visitModlink(EquationsParser::ModlinkContext *context) = 0;

    virtual std::any visitModlinkbase(EquationsParser::ModlinkbaseContext *context) = 0;

    virtual std::any visitAndor(EquationsParser::AndorContext *context) = 0;

    virtual std::any visitModellink(EquationsParser::ModellinkContext *context) = 0;

    virtual std::any visitHighlow(EquationsParser::HighlowContext *context) = 0;

    virtual std::any visitModellinkbase(EquationsParser::ModellinkbaseContext *context) = 0;

    virtual std::any visitFunction(EquationsParser::FunctionContext *context) = 0;

    virtual std::any visitVariable(EquationsParser::VariableContext *context) = 0;

    virtual std::any visitPow(EquationsParser::PowContext *context) = 0;

    virtual std::any visitRealconst(EquationsParser::RealconstContext *context) = 0;

    virtual std::any visitUnary(EquationsParser::UnaryContext *context) = 0;

    virtual std::any visitInfix(EquationsParser::InfixContext *context) = 0;

    virtual std::any visitBraces(EquationsParser::BracesContext *context) = 0;


};

