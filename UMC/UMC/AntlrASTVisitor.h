#pragma once

#include "AntlrASTErrorListener.h"

class AntlrASTVisitor : public EquationsBaseVisitor
{
public:
    CASTTreeBase* pTree;
    AntlrASTVisitor(CASTTreeBase* pASTTree) : pTree(pASTTree)
    {
    }
    std::map<antlr4::tree::ParseTree*, CASTNodeBase*> Parents;

    std::string ParserError(antlr4::ParserRuleContext* ctx, std::string_view error, std::string_view offending)
    {
        return AntlrASTErrorListener::ParserError(error,
            ctx->start->getInputStream()->toString(),
            offending,
            ctx->start->getLine(),
            ctx->start->getCharPositionInLine());
    }

    virtual antlrcpp::Any visitInput(EquationsParser::InputContext* ctx) override {

        Parents.insert(std::make_pair(ctx, pTree->CreateNode<CASTRoot>(nullptr)));
        return visitChildren(ctx);
    }

    CASTNodeBase* GetParent(antlr4::tree::ParseTree* Tree)
    {
        auto Parent = Tree->parent;
        while (Parent)
        {
            auto Pit = Parents.find(Parent);
            if (Pit != Parents.end())
                return Pit->second;
            else
                Parent = Parent->parent;
        }
        return nullptr;
    }

    virtual antlrcpp::Any visitEquationsys(EquationsParser::EquationsysContext* ctx) override
    {
        CASTNodeBase* Parent = GetParent(ctx);
        Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTEquationSystem>()));
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitMain(EquationsParser::MainContext* ctx) override {

        CASTNodeBase* Parent = GetParent(ctx);
        Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTMain>()));
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitInit(EquationsParser::InitContext* ctx) override
    {
        CASTNodeBase* Parent = GetParent(ctx);
        Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTInit>()));
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitEquation(EquationsParser::EquationContext* ctx) override
    {
        CASTNodeBase* Parent = GetParent(ctx);
        auto pEquation = Parent->CreateChild<CASTEquation>();
        pEquation->SetSourceEquation(ctx->getText(), ctx->start->getLine());
        Parents.insert(std::make_pair(ctx, pEquation));
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitModlink(EquationsParser::ModlinkContext* ctx) override
    {
        CASTNodeBase* Parent = GetParent(ctx);
        // создаем экземпляр переменной для ссылки на модель и получаем ее имя
        CASTVariable* var = pTree->CreateVariableFromModelLink(ctx->getText(), Parent);
        Parents.insert(std::make_pair(ctx, var));
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitModellinkbase(EquationsParser::ModellinkbaseContext* ctx) override
    {
        CASTNodeBase* Parent = GetParent(ctx);
        Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTModLinkBase>()));
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitVariable(EquationsParser::VariableContext* ctx) override
    {
        CASTNodeBase* Parent = GetParent(ctx);

        std::string varName(ctx->getText());
        if (varName == "t")
        {
            auto fntime = Parent->CreateChild<CASTfnTime>();
            Parents.insert(std::make_pair(ctx, fntime));
        }
        else
        {
            auto VarInfo = pTree->CheckVariableInfo(varName);
            auto var = Parent->CreateChild<CASTVariable>(ctx->getText());
            var->Info().FromSource = true;  // переменную из исходника в любом случае отмечаем как исходник
            if (!VarInfo)
            {
                // если переменная _впервые_ создается в Init-секции - отмечаем ее как Init
                // по идее она не должна входить в систему уравнений как переменная
                // так как ее уравнение должно быть в Main

                if (var->GetParentOfType(ASTNodeType::Init))
                    var->Info().InitSection = true;
            }
            Parents.insert(std::make_pair(ctx, var));
        }
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitHighlow(EquationsParser::HighlowContext* ctx) override
    {
        CASTNodeBase* Parent = GetParent(ctx);
        switch (ctx->op->getType())
        {
        case EquationsParser::HIGH:
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTHigher>(pTree->GetHostBlocks().size())));
            break;
        case EquationsParser::LOW:
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTLower>(pTree->GetHostBlocks().size())));
            break;
        default:
            EXCEPTIONMSG("Unknown high/low operator");
        }
        return visitChildren(ctx);
    }


    virtual antlrcpp::Any visitAndor(EquationsParser::AndorContext* ctx) override
    {
        CASTNodeBase* Parent = GetParent(ctx);
        switch (ctx->op->getType())
        {
        case EquationsParser::OR:
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnOr>()));
            break;
        case EquationsParser::AND:
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnAnd>()));
            break;
        default:
            EXCEPTIONMSG("Unknown and/or operator");
        }
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitInfix(EquationsParser::InfixContext* ctx) override
    {
        CASTNodeBase* Parent = GetParent(ctx);
        switch (ctx->op->getType())
        {
        case EquationsParser::PLUS:
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTSum>()));
            break;
        case EquationsParser::MINUS:
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTMinus>()));
            break;
        case EquationsParser::MUL:
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTMul>()));
            break;
        case EquationsParser::DIV:
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTDiv>()));
            break;
        default:
            EXCEPTIONMSG("Unknown infix operator");
        }
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitUnary(EquationsParser::UnaryContext* ctx) override
    {
        CASTNodeBase* Parent = GetParent(ctx);
        Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTUminus>()));
        return visitChildren(ctx);
    }


    virtual antlrcpp::Any visitRealconst(EquationsParser::RealconstContext* ctx) override
    {
        CASTNodeBase* Parent = GetParent(ctx);
        Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTNumeric>(ctx->getText())));
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitFunction(EquationsParser::FunctionContext* ctx) override
    {
        CASTNodeBase* Parent = GetParent(ctx);
        if (ctx->VARIABLE()->getText() == "pow")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTPow>()));
        else if (ctx->VARIABLE()->getText() == "sin")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnSin>()));
        else if (ctx->VARIABLE()->getText() == "cos")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnCos>()));
        else if (ctx->VARIABLE()->getText() == "relaydelay")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnRelayDelay>(pTree->GetHostBlocks().size())));
        else if (ctx->VARIABLE()->getText() == "relaymindelay")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnRelayMinDelay> (pTree->GetHostBlocks().size())));
        else if (ctx->VARIABLE()->getText() == "relay")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnRelay>(pTree->GetHostBlocks().size())));
        else if (ctx->VARIABLE()->getText() == "relayd" || ctx->VARIABLE()->getText() == "relaymin")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnRelayMin>(pTree->GetHostBlocks().size())));
        else if (ctx->VARIABLE()->getText() == "abs")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnAbs>(pTree->GetHostBlocks().size())));
        else if (ctx->VARIABLE()->getText() == "laglim")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnLagLim>(pTree->GetHostBlocks().size())));
        else if (ctx->VARIABLE()->getText() == "lag")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnLag>(pTree->GetHostBlocks().size())));
        else if (ctx->VARIABLE()->getText() == "derlag")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnDerLag>(pTree->GetHostBlocks().size())));
        else if (ctx->VARIABLE()->getText() == "starter" || ctx->VARIABLE()->getText() == "action")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnProxyVariable>()));
        else if (ctx->VARIABLE()->getText() == "shrink")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnShrink>(pTree->GetHostBlocks().size())));
        else if (ctx->VARIABLE()->getText() == "expand")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnExpand>(pTree->GetHostBlocks().size())));
        else if (ctx->VARIABLE()->getText() == "deadband")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnDeadBand>(pTree->GetHostBlocks().size())));
        else if (ctx->VARIABLE()->getText() == "limit")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnLimit>(pTree->GetHostBlocks().size())));
        else if (ctx->VARIABLE()->getText() == "exp")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnExp>()));
        else if (ctx->VARIABLE()->getText() == "sqrt")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnSqrt>()));
        else if (ctx->VARIABLE()->getText() == "or")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnOr>()));
        /*
        else if (ctx->VARIABLE()->getText() == "and")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnAnd>()));
        else if (ctx->VARIABLE()->getText() == "not")
            Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTfnNot>()));
            */
        else Parent->Error(ParserError(ctx, "неизвестная функция", ctx->VARIABLE()->getText()), true);
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitPow(EquationsParser::PowContext* ctx) override
    {
        CASTNodeBase* Parent = GetParent(ctx);
        Parents.insert(std::make_pair(ctx, Parent->CreateChild<CASTPow>()));
        return visitChildren(ctx);
    }

    enum class VariableType
    {
        Const,
        External
    };

    void DeclareVariables(antlr4::ParserRuleContext* ctx, const std::vector<antlr4::tree::TerminalNode*>& Variables, VariableType Type)
    {
        for (auto& v : Variables)
        {
            std::string varName(v->getText());

            if (varName == "t")
            {
                pTree->Error(ParserError(ctx, fmt::format("Имя переменной зарезервировано :", varName), varName));
                continue;
            }

            VariableInfo* pVarInfo = pTree->CheckVariableInfo(varName);
            if (pVarInfo)
            {
                const std::string strConst = "const";
                const std::string strExt = "external";

                const std::string strError = "переменная {}, ранее объявленная как {}, переопределяется как {}";
                const std::string strWarning = "переменная {} повторно объявляется как {}";

                const std::string& strDest = pVarInfo->Constant ? strConst : strExt;
                const std::string& strSource = Type == VariableType::Const ? strConst : strExt;

                if (strDest != strSource)
                    pTree->Error(ParserError(ctx, fmt::format(strError, varName, strDest, strSource), varName));
                else
                    pTree->Warning(ParserError(ctx, fmt::format(strWarning, varName, strDest), varName));
            }
            else
            {
                VariableInfo& VarInfo = pTree->GetVariableInfo(varName);
                VarInfo.FromSource = true;
                switch (Type)
                {
                case VariableType::Const:
                    VarInfo.Constant = VarInfo.NamedConstant = true;
                    break;
                case VariableType::External:
                    VarInfo.External = true;
                    break;
                }
            }
        }
    }

    virtual antlrcpp::Any visitConstvardefine(EquationsParser::ConstvardefineContext* ctx) override
    {
        DeclareVariables(ctx, ctx->varlist()->VARIABLE(), VariableType::Const);
        return visitChildren(ctx);
    }

    virtual antlrcpp::Any visitExtvardefine(EquationsParser::ExtvardefineContext* ctx) override
    {
        DeclareVariables(ctx, ctx->varlist()->VARIABLE(), VariableType::External);
        return visitChildren(ctx);
    }

    ~AntlrASTVisitor()
    {
    }
};

