
// Generated from D:\source\repos\DFW2\UMC\UMC\Equations.g4 by ANTLR 4.13.0

#pragma once


#include "antlr4-runtime.h"
#include "EquationsVisitor.h"


/**
 * This class provides an empty implementation of EquationsVisitor, which can be
 * extended to create a visitor which only needs to handle a subset of the available methods.
 */
class  EquationsBaseVisitor : public EquationsVisitor {
public:

  virtual std::any visitInput(EquationsParser::InputContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitMain(EquationsParser::MainContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInit(EquationsParser::InitContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVariables(EquationsParser::VariablesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVardefines(EquationsParser::VardefinesContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEquationsys(EquationsParser::EquationsysContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVarlist(EquationsParser::VarlistContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVardefineline(EquationsParser::VardefinelineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVardefine(EquationsParser::VardefineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstvardefine(EquationsParser::ConstvardefineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExtvardefine(EquationsParser::ExtvardefineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEquation(EquationsParser::EquationContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitEquationline(EquationsParser::EquationlineContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitExpressionlist(EquationsParser::ExpressionlistContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitIntlist(EquationsParser::IntlistContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitConstvalue(EquationsParser::ConstvalueContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitModlink(EquationsParser::ModlinkContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitModlinkbase(EquationsParser::ModlinkbaseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitAndor(EquationsParser::AndorContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitModellink(EquationsParser::ModellinkContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitHighlow(EquationsParser::HighlowContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitModellinkbase(EquationsParser::ModellinkbaseContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitFunction(EquationsParser::FunctionContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitVariable(EquationsParser::VariableContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitPow(EquationsParser::PowContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitRealconst(EquationsParser::RealconstContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitUnary(EquationsParser::UnaryContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitInfix(EquationsParser::InfixContext *ctx) override {
    return visitChildren(ctx);
  }

  virtual std::any visitBraces(EquationsParser::BracesContext *ctx) override {
    return visitChildren(ctx);
  }


};

