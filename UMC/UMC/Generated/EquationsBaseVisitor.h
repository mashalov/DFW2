
// Generated from C:\Users\masha\source\repos\DFW2\UMC\UMC\Equations.g4 by ANTLR 4.9.2

#pragma once


#include "antlr4-runtime.h"
#include "EquationsVisitor.h"


/**
 * This class provides an empty implementation of EquationsVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  EquationsBaseVisitor : public EquationsVisitor {
public:

  virtual antlrcpp::Any visitInput(EquationsParser::InputContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitMain(EquationsParser::MainContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitInit(EquationsParser::InitContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitVariables(EquationsParser::VariablesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitVardefines(EquationsParser::VardefinesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitEquationsys(EquationsParser::EquationsysContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitVarlist(EquationsParser::VarlistContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitVardefineline(EquationsParser::VardefinelineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitVardefine(EquationsParser::VardefineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitConstvardefine(EquationsParser::ConstvardefineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitExtvardefine(EquationsParser::ExtvardefineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitEquation(EquationsParser::EquationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitEquationline(EquationsParser::EquationlineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitExpressionlist(EquationsParser::ExpressionlistContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitIntlist(EquationsParser::IntlistContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitConstvalue(EquationsParser::ConstvalueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitModlink(EquationsParser::ModlinkContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitModlinkbase(EquationsParser::ModlinkbaseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitAndor(EquationsParser::AndorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitModellink(EquationsParser::ModellinkContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitHighlow(EquationsParser::HighlowContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitModellinkbase(EquationsParser::ModellinkbaseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitFunction(EquationsParser::FunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitVariable(EquationsParser::VariableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitPow(EquationsParser::PowContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitRealconst(EquationsParser::RealconstContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitUnary(EquationsParser::UnaryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitInfix(EquationsParser::InfixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual antlrcpp::Any visitBraces(EquationsParser::BracesContext *ctx) override {
    return visitChildren(ctx);
  }


};

