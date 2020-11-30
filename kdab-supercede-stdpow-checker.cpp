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

/**
 * Read:
 * Find call expressions
 * - whose callee is defined in standard namespace and has the name "pow" (We label its declaration "callee").
 * - whose first argument is any expression. We call that expression "base"
 * - whose second argument is any expression. We call that expression "exponent"
 * - that did not enter the source through an `#include` directive
 * We denote this call expression as "funcCall".
 */
auto stdPowMatcher =
    callExpr(
        callee(
            functionDecl(
                hasName("pow"),
                isInStdNamespace()
            ).bind("callee")
        ),
        hasArgument(0, expr().bind("base")),
        hasArgument(1, expr().bind("exponent")),
        isExpansionInMainFile()
    ).bind("funcCall");

/**
 * StdPowChecker implements the main analysis code. It implements a MatchCallback, which follows
 * the listener pattern to define code to be run for every occurance of a certain AST pattern in
 * the analyzed code.
 */
class StdPowChecker : public MatchFinder::MatchCallback {
public :
    StdPowChecker() = default;

    void run(const MatchFinder::MatchResult &result) override
    {
        ASTContext *context = result.Context;

        // Extract those AST nodes out of the result which we have named before using `bind`
        const CallExpr *callExpr = result.Nodes.getNodeAs<CallExpr>("funcCall");
        const FunctionDecl *callee = result.Nodes.getNodeAs<FunctionDecl>("callee");
        const Expr *base = result.Nodes.getNodeAs<Expr>("base");
        const Expr *exponent = result.Nodes.getNodeAs<Expr>("exponent");

        if (exponent && exponent->isCXX11ConstantExpr(*context) && exponent->getType()->isIntegerType()) {

            // Find the region within the original source file, where this call expression is written.
            // Also reconstruct the source code of the base and exponent expressions as strings,
            // exactly (including whitespaces) as they were written in the code.
            // We will use these as building blocks to construct the new, refactored code.
            auto &sm = context->getSourceManager();
            auto &lo = context->getLangOpts();
            auto baseRng = Lexer::getAsCharRange(base->getSourceRange(), sm, lo);
            auto expRng = Lexer::getAsCharRange(exponent->getSourceRange(), sm, lo);
            auto callRng = Lexer::getAsCharRange(callExpr->getSourceRange(), sm, lo);

            auto baseStr = Lexer::getSourceText(baseRng, sm, lo);
            auto expStr = Lexer::getSourceText(callRng, sm, lo);



            auto &diagEngine = context->getDiagnostics();
            // Create a diagnostic message from scratch. A more complete solution would fill the
            // diagnostic messages into a Diagnostic*Kinds.td and use the `tblgen` command of llvm
            // to generate unique ID, severity and the English translation + format string.
            unsigned ID = diagEngine.getDiagnosticIDs()->getCustomDiagID(
                DiagnosticIDs::Warning,
                "std::pow is called with integer constant expression. Use utils::pow instead.");

            // Create a diagnostic message at the location of the exponent with the message created
            // above. Then, provide a FixItHint, which implements the automated refactoring we want
            // to do.
            //
            // A more sophisticated refactoring tool would make use of clang::tooling::Replacement group of
            // classes, instead of simple FixItHints.
            //
            // llvm::Twine is just a simple helper to efficiently concatenate strings.
            diagEngine.Report(exponent->getBeginLoc(), ID)
                << FixItHint::CreateReplacement(callRng,
                                                (llvm::Twine("utils::pow<") + expStr + ">(" + baseStr + ")").str()
                                               );

            // TODO: Use clang::tooling::HeaderIncludes to automatically include <utils.h>.
        }
    }
};

/**
 * SupercedeStdPowAction implements a FrontendAction that will be executed once for every source
 * file. We initialize our analysis here by requesting a MatchFinder to run our AST matcher for
 * the readily parsed source file.
 *
 * This class derives from FixItAction in order to automate the application of our fix-it hints.
 */
class SupercedeStdPowAction : public FixItAction {
public:
    SupercedeStdPowAction()
    {
        m_finder.addMatcher(stdPowMatcher, &m_stdPowChecker);
    }

    std::unique_ptr<ASTConsumer>
    CreateASTConsumer(CompilerInstance &, StringRef) override {
        // The CompilerInstance argument is an important and valuable object, which gives us access
        // to e.g. the preprocessor instance. We don't need it in this example, though.

        // the ASTConsumer we return here will be fed with the AST of the analyzed source files.
        return m_finder.newASTConsumer();
    }

public:
    MatchFinder m_finder;
    StdPowChecker m_stdPowChecker;
};

int main(int argc, const char **argv) {
  CommonOptionsParser optionsParser {
      argc, argv,
      llvm::cl::GeneralCategory // Replace this by your own OptionCategory instance when adding own options
  };
  ClangTool tool(optionsParser.getCompilations(),
                 optionsParser.getSourcePathList());

  return tool.run(newFrontendActionFactory<SupercedeStdPowAction>().get());
}
