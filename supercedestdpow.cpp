#include "clang/Frontend/FrontendActions.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Tooling/Refactoring.h"
#include "llvm/Support/CommandLine.h"

#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"

#include "clang/Basic/Diagnostic.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/FixItRewriter.h"
#include "clang/Rewrite/Frontend/FrontendActions.h"
#include "clang/Tooling/Refactoring/AtomicChange.h"


using namespace clang;
using namespace clang::ast_matchers;
using namespace clang::tooling;
using namespace llvm;

auto stdPowMatcher =
    callExpr(
        callee(
            functionDecl(
                hasName("pow"),
                isInStdNamespace()
            ).bind("callee")
        ),
        hasArgument(0, expr().bind("base")),
        hasArgument(1, expr().bind("exponent"))
    ).bind("funcCall");

class StdPowChecker : public MatchFinder::MatchCallback {
public :
    StdPowChecker() = default;

    void run(const MatchFinder::MatchResult &result) override
    {
        ASTContext *context = result.Context;
        const CallExpr *callExpr = result.Nodes.getNodeAs<CallExpr>("funcCall");
        const FunctionDecl *callee = result.Nodes.getNodeAs<FunctionDecl>("callee");
        const Expr *base = result.Nodes.getNodeAs<Expr>("base");
        const Expr *exponent = result.Nodes.getNodeAs<Expr>("exponent");


        if (!callExpr || !context || !context->getSourceManager().isWrittenInMainFile(callExpr->getBeginLoc()))
            return;

        if (exponent && exponent->isCXX11ConstantExpr(*context) && exponent->getType()->isIntegerType()) {
            auto &sm = context->getSourceManager();
            auto &lo = context->getLangOpts();
            auto baseRng = Lexer::getAsCharRange(base->getSourceRange(), sm, lo);
            auto expRng = Lexer::getAsCharRange(exponent->getSourceRange(), sm, lo);
            auto callRng = Lexer::getAsCharRange(callExpr->getSourceRange(), sm, lo);

            auto baseStr = Lexer::getSourceText(baseRng, sm, lo);
            auto expStr = Lexer::getSourceText(callRng, sm, lo);

            auto &diagEngine = context->getDiagnostics();
            unsigned ID = diagEngine.getDiagnosticIDs()->getCustomDiagID(
                DiagnosticIDs::Warning,
                "std::pow is called with integer constant expression. Use utils::pow instead.");
            diagEngine.Report(exponent->getBeginLoc(), ID)
                << FixItHint::CreateReplacement(callRng,
                                                (llvm::Twine("utils::pow<") + expStr + ">(" + baseStr + ")").str()
                                               );
        }
    }
};


class SupercedeStdPowAction : public FixItAction {
public:
    SupercedeStdPowAction()
    {
        m_finder.addMatcher(stdPowMatcher, &m_stdPowChecker);
    }

    std::unique_ptr<ASTConsumer>
    CreateASTConsumer(CompilerInstance &, StringRef) override {
        return m_finder.newASTConsumer();
    }

public:
    MatchFinder m_finder;
    StdPowChecker m_stdPowChecker;
};

int main(int argc, const char **argv) {
  CommonOptionsParser optionsParser {
      argc, argv,
      llvm::cl::GeneralCategory
  };
  ClangTool tool(optionsParser.getCompilations(),
                 optionsParser.getSourcePathList());

  return tool.run(newFrontendActionFactory<SupercedeStdPowAction>().get());
}
