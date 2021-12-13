#pragma once

enum class ASTNodeType
{
	Root,
	EquationSystem,
	Main,
	Init,
	Equation,
	Numeric,
	Variable,
	Sum,
	Mul,
	Uminus,
	Pow,
	ModLinkBase,
	FnSin,
	FnCos,
	FnRelay,
	FnRelayMin,
	FnAbs,
	FnProxyVariable,
	FnHigher,
	FnLower,
	FnLag,
	FnLagLim,
	FnDerLag,
	FnExpand,
	FnShrink,
	FnDeadBand,
	FnLimit,
	FnExp,
	FnSqrt,
	FnOr,
	FnAnd,
	FnNot,
	FnTime,
	Any,
	Jacobian,
	JacobiElement,
	ConstArgs,
	InitExtVars
};

template <typename Enumeration>
constexpr typename std::underlying_type<Enumeration>::type as_integer(Enumeration const value)
{
	return static_cast<typename std::underlying_type<Enumeration>::type>(value);
}