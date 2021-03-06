//===--- Stmt.cpp - Statement AST Node Implementation ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the Stmt class and statement subclasses.
//
//===----------------------------------------------------------------------===//

#include "clang/AST/Stmt.h"
#include "clang/AST/ExprCXX.h"
#include "clang/AST/ExprObjC.h"
#include "clang/AST/StmtCXX.h"
#include "clang/AST/StmtObjC.h"
#include "clang/AST/Type.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTDiagnostic.h"
#include "clang/Basic/TargetInfo.h"
#include <cstdio>
using namespace clang;

static struct StmtClassNameTable {
  const char *Name;
  unsigned Counter;
  unsigned Size;
} StmtClassInfo[Stmt::lastStmtConstant+1];

static StmtClassNameTable &getStmtInfoTableEntry(Stmt::StmtClass E) {
  static bool Initialized = false;
  if (Initialized)
    return StmtClassInfo[E];

  // Intialize the table on the first use.
  Initialized = true;
#define ABSTRACT_STMT(STMT)
#define STMT(CLASS, PARENT) \
  StmtClassInfo[(unsigned)Stmt::CLASS##Class].Name = #CLASS;    \
  StmtClassInfo[(unsigned)Stmt::CLASS##Class].Size = sizeof(CLASS);
#include "clang/AST/StmtNodes.inc"

  return StmtClassInfo[E];
}

const char *Stmt::getStmtClassName() const {
  return getStmtInfoTableEntry((StmtClass) StmtBits.sClass).Name;
}

void Stmt::PrintStats() {
  // Ensure the table is primed.
  getStmtInfoTableEntry(Stmt::NullStmtClass);

  unsigned sum = 0;
  fprintf(stderr, "*** Stmt/Expr Stats:\n");
  for (int i = 0; i != Stmt::lastStmtConstant+1; i++) {
    if (StmtClassInfo[i].Name == 0) continue;
    sum += StmtClassInfo[i].Counter;
  }
  fprintf(stderr, "  %d stmts/exprs total.\n", sum);
  sum = 0;
  for (int i = 0; i != Stmt::lastStmtConstant+1; i++) {
    if (StmtClassInfo[i].Name == 0) continue;
    if (StmtClassInfo[i].Counter == 0) continue;
    fprintf(stderr, "    %d %s, %d each (%d bytes)\n",
            StmtClassInfo[i].Counter, StmtClassInfo[i].Name,
            StmtClassInfo[i].Size,
            StmtClassInfo[i].Counter*StmtClassInfo[i].Size);
    sum += StmtClassInfo[i].Counter*StmtClassInfo[i].Size;
  }
  fprintf(stderr, "Total bytes = %d\n", sum);
}

void Stmt::addStmtClass(StmtClass s) {
  ++getStmtInfoTableEntry(s).Counter;
}

static bool StatSwitch = false;

bool Stmt::CollectingStats(bool Enable) {
  if (Enable) StatSwitch = true;
  return StatSwitch;
}

void CompoundStmt::setStmts(ASTContext &C, Stmt **Stmts, unsigned NumStmts) {
  if (this->Body)
    C.Deallocate(Body);
  this->CompoundStmtBits.NumStmts = NumStmts;

  Body = new (C) Stmt*[NumStmts];
  memcpy(Body, Stmts, sizeof(Stmt *) * NumStmts);
}

const char *LabelStmt::getName() const {
  return getID()->getNameStart();
}

// This is defined here to avoid polluting Stmt.h with importing Expr.h
SourceRange ReturnStmt::getSourceRange() const {
  if (RetExpr)
    return SourceRange(RetLoc, RetExpr->getLocEnd());
  else
    return SourceRange(RetLoc);
}

bool Stmt::hasImplicitControlFlow() const {
  switch (StmtBits.sClass) {
    default:
      return false;

    case CallExprClass:
    case ConditionalOperatorClass:
    case ChooseExprClass:
    case StmtExprClass:
    case DeclStmtClass:
      return true;

    case Stmt::BinaryOperatorClass: {
      const BinaryOperator* B = cast<BinaryOperator>(this);
      if (B->isLogicalOp() || B->getOpcode() == BO_Comma)
        return true;
      else
        return false;
    }
  }
}

Expr *AsmStmt::getOutputExpr(unsigned i) {
  return cast<Expr>(Exprs[i]);
}

/// getOutputConstraint - Return the constraint string for the specified
/// output operand.  All output constraints are known to be non-empty (either
/// '=' or '+').
llvm::StringRef AsmStmt::getOutputConstraint(unsigned i) const {
  return getOutputConstraintLiteral(i)->getString();
}

/// getNumPlusOperands - Return the number of output operands that have a "+"
/// constraint.
unsigned AsmStmt::getNumPlusOperands() const {
  unsigned Res = 0;
  for (unsigned i = 0, e = getNumOutputs(); i != e; ++i)
    if (isOutputPlusConstraint(i))
      ++Res;
  return Res;
}

Expr *AsmStmt::getInputExpr(unsigned i) {
  return cast<Expr>(Exprs[i + NumOutputs]);
}

/// getInputConstraint - Return the specified input constraint.  Unlike output
/// constraints, these can be empty.
llvm::StringRef AsmStmt::getInputConstraint(unsigned i) const {
  return getInputConstraintLiteral(i)->getString();
}


void AsmStmt::setOutputsAndInputsAndClobbers(ASTContext &C,
                                             IdentifierInfo **Names,
                                             StringLiteral **Constraints,
                                             Stmt **Exprs,
                                             unsigned NumOutputs,
                                             unsigned NumInputs,                                      
                                             StringLiteral **Clobbers,
                                             unsigned NumClobbers) {
  this->NumOutputs = NumOutputs;
  this->NumInputs = NumInputs;
  this->NumClobbers = NumClobbers;

  unsigned NumExprs = NumOutputs + NumInputs;
  
  C.Deallocate(this->Names);
  this->Names = new (C) IdentifierInfo*[NumExprs];
  std::copy(Names, Names + NumExprs, this->Names);
  
  C.Deallocate(this->Exprs);
  this->Exprs = new (C) Stmt*[NumExprs];
  std::copy(Exprs, Exprs + NumExprs, this->Exprs);
  
  C.Deallocate(this->Constraints);
  this->Constraints = new (C) StringLiteral*[NumExprs];
  std::copy(Constraints, Constraints + NumExprs, this->Constraints);
  
  C.Deallocate(this->Clobbers);
  this->Clobbers = new (C) StringLiteral*[NumClobbers];
  std::copy(Clobbers, Clobbers + NumClobbers, this->Clobbers);
}

/// getNamedOperand - Given a symbolic operand reference like %[foo],
/// translate this into a numeric value needed to reference the same operand.
/// This returns -1 if the operand name is invalid.
int AsmStmt::getNamedOperand(llvm::StringRef SymbolicName) const {
  unsigned NumPlusOperands = 0;

  // Check if this is an output operand.
  for (unsigned i = 0, e = getNumOutputs(); i != e; ++i) {
    if (getOutputName(i) == SymbolicName)
      return i;
  }

  for (unsigned i = 0, e = getNumInputs(); i != e; ++i)
    if (getInputName(i) == SymbolicName)
      return getNumOutputs() + NumPlusOperands + i;

  // Not found.
  return -1;
}

/// AnalyzeAsmString - Analyze the asm string of the current asm, decomposing
/// it into pieces.  If the asm string is erroneous, emit errors and return
/// true, otherwise return false.
unsigned AsmStmt::AnalyzeAsmString(llvm::SmallVectorImpl<AsmStringPiece>&Pieces,
                                   ASTContext &C, unsigned &DiagOffs) const {
  llvm::StringRef Str = getAsmString()->getString();
  const char *StrStart = Str.begin();
  const char *StrEnd = Str.end();
  const char *CurPtr = StrStart;

  // "Simple" inline asms have no constraints or operands, just convert the asm
  // string to escape $'s.
  if (isSimple()) {
    std::string Result;
    for (; CurPtr != StrEnd; ++CurPtr) {
      switch (*CurPtr) {
      case '$':
        Result += "$$";
        break;
      default:
        Result += *CurPtr;
        break;
      }
    }
    Pieces.push_back(AsmStringPiece(Result));
    return 0;
  }

  // CurStringPiece - The current string that we are building up as we scan the
  // asm string.
  std::string CurStringPiece;

  bool HasVariants = !C.Target.hasNoAsmVariants();
  
  while (1) {
    // Done with the string?
    if (CurPtr == StrEnd) {
      if (!CurStringPiece.empty())
        Pieces.push_back(AsmStringPiece(CurStringPiece));
      return 0;
    }

    char CurChar = *CurPtr++;
    switch (CurChar) {
    case '$': CurStringPiece += "$$"; continue;
    case '{': CurStringPiece += (HasVariants ? "$(" : "{"); continue;
    case '|': CurStringPiece += (HasVariants ? "$|" : "|"); continue;
    case '}': CurStringPiece += (HasVariants ? "$)" : "}"); continue;
    case '%':
      break;
    default:
      CurStringPiece += CurChar;
      continue;
    }
    
    // Escaped "%" character in asm string.
    if (CurPtr == StrEnd) {
      // % at end of string is invalid (no escape).
      DiagOffs = CurPtr-StrStart-1;
      return diag::err_asm_invalid_escape;
    }

    char EscapedChar = *CurPtr++;
    if (EscapedChar == '%') {  // %% -> %
      // Escaped percentage sign.
      CurStringPiece += '%';
      continue;
    }

    if (EscapedChar == '=') {  // %= -> Generate an unique ID.
      CurStringPiece += "${:uid}";
      continue;
    }

    // Otherwise, we have an operand.  If we have accumulated a string so far,
    // add it to the Pieces list.
    if (!CurStringPiece.empty()) {
      Pieces.push_back(AsmStringPiece(CurStringPiece));
      CurStringPiece.clear();
    }

    // Handle %x4 and %x[foo] by capturing x as the modifier character.
    char Modifier = '\0';
    if (isalpha(EscapedChar)) {
      Modifier = EscapedChar;
      EscapedChar = *CurPtr++;
    }

    if (isdigit(EscapedChar)) {
      // %n - Assembler operand n
      unsigned N = 0;

      --CurPtr;
      while (CurPtr != StrEnd && isdigit(*CurPtr))
        N = N*10 + ((*CurPtr++)-'0');

      unsigned NumOperands =
        getNumOutputs() + getNumPlusOperands() + getNumInputs();
      if (N >= NumOperands) {
        DiagOffs = CurPtr-StrStart-1;
        return diag::err_asm_invalid_operand_number;
      }

      Pieces.push_back(AsmStringPiece(N, Modifier));
      continue;
    }

    // Handle %[foo], a symbolic operand reference.
    if (EscapedChar == '[') {
      DiagOffs = CurPtr-StrStart-1;

      // Find the ']'.
      const char *NameEnd = (const char*)memchr(CurPtr, ']', StrEnd-CurPtr);
      if (NameEnd == 0)
        return diag::err_asm_unterminated_symbolic_operand_name;
      if (NameEnd == CurPtr)
        return diag::err_asm_empty_symbolic_operand_name;

      llvm::StringRef SymbolicName(CurPtr, NameEnd - CurPtr);

      int N = getNamedOperand(SymbolicName);
      if (N == -1) {
        // Verify that an operand with that name exists.
        DiagOffs = CurPtr-StrStart;
        return diag::err_asm_unknown_symbolic_operand_name;
      }
      Pieces.push_back(AsmStringPiece(N, Modifier));

      CurPtr = NameEnd+1;
      continue;
    }

    DiagOffs = CurPtr-StrStart-1;
    return diag::err_asm_invalid_escape;
  }
}

QualType CXXCatchStmt::getCaughtType() const {
  if (ExceptionDecl)
    return ExceptionDecl->getType();
  return QualType();
}

//===----------------------------------------------------------------------===//
// Constructors
//===----------------------------------------------------------------------===//

AsmStmt::AsmStmt(ASTContext &C, SourceLocation asmloc, bool issimple, 
                 bool isvolatile, bool msasm, 
                 unsigned numoutputs, unsigned numinputs,
                 IdentifierInfo **names, StringLiteral **constraints,
                 Expr **exprs, StringLiteral *asmstr, unsigned numclobbers,
                 StringLiteral **clobbers, SourceLocation rparenloc)
  : Stmt(AsmStmtClass), AsmLoc(asmloc), RParenLoc(rparenloc), AsmStr(asmstr)
  , IsSimple(issimple), IsVolatile(isvolatile), MSAsm(msasm)
  , NumOutputs(numoutputs), NumInputs(numinputs), NumClobbers(numclobbers) {

  unsigned NumExprs = NumOutputs +NumInputs;
    
  Names = new (C) IdentifierInfo*[NumExprs];
  std::copy(names, names + NumExprs, Names);

  Exprs = new (C) Stmt*[NumExprs];
  std::copy(exprs, exprs + NumExprs, Exprs);

  Constraints = new (C) StringLiteral*[NumExprs];
  std::copy(constraints, constraints + NumExprs, Constraints);

  Clobbers = new (C) StringLiteral*[NumClobbers];
  std::copy(clobbers, clobbers + NumClobbers, Clobbers);
}

ObjCForCollectionStmt::ObjCForCollectionStmt(Stmt *Elem, Expr *Collect,
                                             Stmt *Body,  SourceLocation FCL,
                                             SourceLocation RPL)
: Stmt(ObjCForCollectionStmtClass) {
  SubExprs[ELEM] = Elem;
  SubExprs[COLLECTION] = reinterpret_cast<Stmt*>(Collect);
  SubExprs[BODY] = Body;
  ForLoc = FCL;
  RParenLoc = RPL;
}

ObjCAtTryStmt::ObjCAtTryStmt(SourceLocation atTryLoc, Stmt *atTryStmt,
                             Stmt **CatchStmts, unsigned NumCatchStmts,
                             Stmt *atFinallyStmt)
  : Stmt(ObjCAtTryStmtClass), AtTryLoc(atTryLoc),
    NumCatchStmts(NumCatchStmts), HasFinally(atFinallyStmt != 0)
{
  Stmt **Stmts = getStmts();
  Stmts[0] = atTryStmt;
  for (unsigned I = 0; I != NumCatchStmts; ++I)
    Stmts[I + 1] = CatchStmts[I];
  
  if (HasFinally)
    Stmts[NumCatchStmts + 1] = atFinallyStmt;
}

ObjCAtTryStmt *ObjCAtTryStmt::Create(ASTContext &Context, 
                                     SourceLocation atTryLoc, 
                                     Stmt *atTryStmt,
                                     Stmt **CatchStmts, 
                                     unsigned NumCatchStmts,
                                     Stmt *atFinallyStmt) {
  unsigned Size = sizeof(ObjCAtTryStmt) + 
    (1 + NumCatchStmts + (atFinallyStmt != 0)) * sizeof(Stmt *);
  void *Mem = Context.Allocate(Size, llvm::alignOf<ObjCAtTryStmt>());
  return new (Mem) ObjCAtTryStmt(atTryLoc, atTryStmt, CatchStmts, NumCatchStmts,
                                 atFinallyStmt);
}

ObjCAtTryStmt *ObjCAtTryStmt::CreateEmpty(ASTContext &Context, 
                                                 unsigned NumCatchStmts,
                                                 bool HasFinally) {
  unsigned Size = sizeof(ObjCAtTryStmt) + 
    (1 + NumCatchStmts + HasFinally) * sizeof(Stmt *);
  void *Mem = Context.Allocate(Size, llvm::alignOf<ObjCAtTryStmt>());
  return new (Mem) ObjCAtTryStmt(EmptyShell(), NumCatchStmts, HasFinally);  
}

SourceRange ObjCAtTryStmt::getSourceRange() const {
  SourceLocation EndLoc;
  if (HasFinally)
    EndLoc = getFinallyStmt()->getLocEnd();
  else if (NumCatchStmts)
    EndLoc = getCatchStmt(NumCatchStmts - 1)->getLocEnd();
  else
    EndLoc = getTryBody()->getLocEnd();
  
  return SourceRange(AtTryLoc, EndLoc);
}

CXXTryStmt *CXXTryStmt::Create(ASTContext &C, SourceLocation tryLoc,
                               Stmt *tryBlock, Stmt **handlers, 
                               unsigned numHandlers) {
  std::size_t Size = sizeof(CXXTryStmt);
  Size += ((numHandlers + 1) * sizeof(Stmt));

  void *Mem = C.Allocate(Size, llvm::alignOf<CXXTryStmt>());
  return new (Mem) CXXTryStmt(tryLoc, tryBlock, handlers, numHandlers);
}

CXXTryStmt *CXXTryStmt::Create(ASTContext &C, EmptyShell Empty,
                               unsigned numHandlers) {
  std::size_t Size = sizeof(CXXTryStmt);
  Size += ((numHandlers + 1) * sizeof(Stmt));

  void *Mem = C.Allocate(Size, llvm::alignOf<CXXTryStmt>());
  return new (Mem) CXXTryStmt(Empty, numHandlers);
}

CXXTryStmt::CXXTryStmt(SourceLocation tryLoc, Stmt *tryBlock,
                       Stmt **handlers, unsigned numHandlers)
  : Stmt(CXXTryStmtClass), TryLoc(tryLoc), NumHandlers(numHandlers) {
  Stmt **Stmts = reinterpret_cast<Stmt **>(this + 1);
  Stmts[0] = tryBlock;
  std::copy(handlers, handlers + NumHandlers, Stmts + 1);
}

IfStmt::IfStmt(ASTContext &C, SourceLocation IL, VarDecl *var, Expr *cond, 
               Stmt *then, SourceLocation EL, Stmt *elsev)
  : Stmt(IfStmtClass), IfLoc(IL), ElseLoc(EL)
{
  setConditionVariable(C, var);
  SubExprs[COND] = reinterpret_cast<Stmt*>(cond);
  SubExprs[THEN] = then;
  SubExprs[ELSE] = elsev;  
}

VarDecl *IfStmt::getConditionVariable() const {
  if (!SubExprs[VAR])
    return 0;
  
  DeclStmt *DS = cast<DeclStmt>(SubExprs[VAR]);
  return cast<VarDecl>(DS->getSingleDecl());
}

void IfStmt::setConditionVariable(ASTContext &C, VarDecl *V) {
  if (!V) {
    SubExprs[VAR] = 0;
    return;
  }
  
  SubExprs[VAR] = new (C) DeclStmt(DeclGroupRef(V), 
                                   V->getSourceRange().getBegin(),
                                   V->getSourceRange().getEnd());
}

ForStmt::ForStmt(ASTContext &C, Stmt *Init, Expr *Cond, VarDecl *condVar, 
                 Expr *Inc, Stmt *Body, SourceLocation FL, SourceLocation LP, 
                 SourceLocation RP)
  : Stmt(ForStmtClass), ForLoc(FL), LParenLoc(LP), RParenLoc(RP) 
{
  SubExprs[INIT] = Init;
  setConditionVariable(C, condVar);
  SubExprs[COND] = reinterpret_cast<Stmt*>(Cond);
  SubExprs[INC] = reinterpret_cast<Stmt*>(Inc);
  SubExprs[BODY] = Body;
}

VarDecl *ForStmt::getConditionVariable() const {
  if (!SubExprs[CONDVAR])
    return 0;
  
  DeclStmt *DS = cast<DeclStmt>(SubExprs[CONDVAR]);
  return cast<VarDecl>(DS->getSingleDecl());
}

void ForStmt::setConditionVariable(ASTContext &C, VarDecl *V) {
  if (!V) {
    SubExprs[CONDVAR] = 0;
    return;
  }
  
  SubExprs[CONDVAR] = new (C) DeclStmt(DeclGroupRef(V), 
                                       V->getSourceRange().getBegin(),
                                       V->getSourceRange().getEnd());
}

SwitchStmt::SwitchStmt(ASTContext &C, VarDecl *Var, Expr *cond) 
  : Stmt(SwitchStmtClass), FirstCase(0), AllEnumCasesCovered(0) 
{
  setConditionVariable(C, Var);
  SubExprs[COND] = reinterpret_cast<Stmt*>(cond);
  SubExprs[BODY] = NULL;
}

VarDecl *SwitchStmt::getConditionVariable() const {
  if (!SubExprs[VAR])
    return 0;
  
  DeclStmt *DS = cast<DeclStmt>(SubExprs[VAR]);
  return cast<VarDecl>(DS->getSingleDecl());
}

void SwitchStmt::setConditionVariable(ASTContext &C, VarDecl *V) {
  if (!V) {
    SubExprs[VAR] = 0;
    return;
  }
  
  SubExprs[VAR] = new (C) DeclStmt(DeclGroupRef(V), 
                                   V->getSourceRange().getBegin(),
                                   V->getSourceRange().getEnd());
}

WhileStmt::WhileStmt(ASTContext &C, VarDecl *Var, Expr *cond, Stmt *body, 
                     SourceLocation WL)
: Stmt(WhileStmtClass)
{
  setConditionVariable(C, Var);
  SubExprs[COND] = reinterpret_cast<Stmt*>(cond);
  SubExprs[BODY] = body;
  WhileLoc = WL;
}

VarDecl *WhileStmt::getConditionVariable() const {
  if (!SubExprs[VAR])
    return 0;
  
  DeclStmt *DS = cast<DeclStmt>(SubExprs[VAR]);
  return cast<VarDecl>(DS->getSingleDecl());
}

void WhileStmt::setConditionVariable(ASTContext &C, VarDecl *V) {
  if (!V) {
    SubExprs[VAR] = 0;
    return;
  }
  
  SubExprs[VAR] = new (C) DeclStmt(DeclGroupRef(V), 
                                   V->getSourceRange().getBegin(),
                                   V->getSourceRange().getEnd());
}

//===----------------------------------------------------------------------===//
//  Child Iterators for iterating over subexpressions/substatements
//===----------------------------------------------------------------------===//

// DeclStmt
Stmt::child_iterator DeclStmt::child_begin() {
  return StmtIterator(DG.begin(), DG.end());
}

Stmt::child_iterator DeclStmt::child_end() {
  return StmtIterator(DG.end(), DG.end());
}

// NullStmt
Stmt::child_iterator NullStmt::child_begin() { return child_iterator(); }
Stmt::child_iterator NullStmt::child_end() { return child_iterator(); }

// CompoundStmt
Stmt::child_iterator CompoundStmt::child_begin() { return &Body[0]; }
Stmt::child_iterator CompoundStmt::child_end() {
  return &Body[0]+CompoundStmtBits.NumStmts;
}

// CaseStmt
Stmt::child_iterator CaseStmt::child_begin() { return &SubExprs[0]; }
Stmt::child_iterator CaseStmt::child_end() { return &SubExprs[END_EXPR]; }

// DefaultStmt
Stmt::child_iterator DefaultStmt::child_begin() { return &SubStmt; }
Stmt::child_iterator DefaultStmt::child_end() { return &SubStmt+1; }

// LabelStmt
Stmt::child_iterator LabelStmt::child_begin() { return &SubStmt; }
Stmt::child_iterator LabelStmt::child_end() { return &SubStmt+1; }

// IfStmt
Stmt::child_iterator IfStmt::child_begin() {
  return &SubExprs[0];
}
Stmt::child_iterator IfStmt::child_end() {
  return &SubExprs[0]+END_EXPR;
}

// SwitchStmt
Stmt::child_iterator SwitchStmt::child_begin() {
  return &SubExprs[0];
}
Stmt::child_iterator SwitchStmt::child_end() {
  return &SubExprs[0]+END_EXPR;
}

// WhileStmt
Stmt::child_iterator WhileStmt::child_begin() {
  return &SubExprs[0];
}
Stmt::child_iterator WhileStmt::child_end() {
  return &SubExprs[0]+END_EXPR;
}

// DoStmt
Stmt::child_iterator DoStmt::child_begin() { return &SubExprs[0]; }
Stmt::child_iterator DoStmt::child_end() { return &SubExprs[0]+END_EXPR; }

// ForStmt
Stmt::child_iterator ForStmt::child_begin() {
  return &SubExprs[0];
}
Stmt::child_iterator ForStmt::child_end() {
  return &SubExprs[0]+END_EXPR;
}

// ObjCForCollectionStmt
Stmt::child_iterator ObjCForCollectionStmt::child_begin() {
  return &SubExprs[0];
}
Stmt::child_iterator ObjCForCollectionStmt::child_end() {
  return &SubExprs[0]+END_EXPR;
}

// GotoStmt
Stmt::child_iterator GotoStmt::child_begin() { return child_iterator(); }
Stmt::child_iterator GotoStmt::child_end() { return child_iterator(); }

// IndirectGotoStmt
LabelStmt *IndirectGotoStmt::getConstantTarget() {
  if (AddrLabelExpr *E =
        dyn_cast<AddrLabelExpr>(getTarget()->IgnoreParenImpCasts()))
    return E->getLabel();
  return 0;
}

Stmt::child_iterator IndirectGotoStmt::child_begin() { return &Target; }
Stmt::child_iterator IndirectGotoStmt::child_end() { return &Target+1; }

// ContinueStmt
Stmt::child_iterator ContinueStmt::child_begin() { return child_iterator(); }
Stmt::child_iterator ContinueStmt::child_end() { return child_iterator(); }

// BreakStmt
Stmt::child_iterator BreakStmt::child_begin() { return child_iterator(); }
Stmt::child_iterator BreakStmt::child_end() { return child_iterator(); }

// ReturnStmt
const Expr* ReturnStmt::getRetValue() const {
  return cast_or_null<Expr>(RetExpr);
}
Expr* ReturnStmt::getRetValue() {
  return cast_or_null<Expr>(RetExpr);
}

Stmt::child_iterator ReturnStmt::child_begin() {
  return &RetExpr;
}
Stmt::child_iterator ReturnStmt::child_end() {
  return RetExpr ? &RetExpr+1 : &RetExpr;
}

// AsmStmt
Stmt::child_iterator AsmStmt::child_begin() {
  return NumOutputs + NumInputs == 0 ? 0 : &Exprs[0];
}
Stmt::child_iterator AsmStmt::child_end() {
  return NumOutputs + NumInputs == 0 ? 0 : &Exprs[0] + NumOutputs + NumInputs;
}

// ObjCAtCatchStmt
Stmt::child_iterator ObjCAtCatchStmt::child_begin() { return &Body; }
Stmt::child_iterator ObjCAtCatchStmt::child_end() { return &Body + 1; }

// ObjCAtFinallyStmt
Stmt::child_iterator ObjCAtFinallyStmt::child_begin() { return &AtFinallyStmt; }
Stmt::child_iterator ObjCAtFinallyStmt::child_end() { return &AtFinallyStmt+1; }

// ObjCAtTryStmt
Stmt::child_iterator ObjCAtTryStmt::child_begin() { return getStmts(); }

Stmt::child_iterator ObjCAtTryStmt::child_end() {
  return getStmts() + 1 + NumCatchStmts + HasFinally;
}

// ObjCAtThrowStmt
Stmt::child_iterator ObjCAtThrowStmt::child_begin() {
  return &Throw;
}

Stmt::child_iterator ObjCAtThrowStmt::child_end() {
  return &Throw+1;
}

// ObjCAtSynchronizedStmt
Stmt::child_iterator ObjCAtSynchronizedStmt::child_begin() {
  return &SubStmts[0];
}

Stmt::child_iterator ObjCAtSynchronizedStmt::child_end() {
  return &SubStmts[0]+END_EXPR;
}

// CXXCatchStmt
Stmt::child_iterator CXXCatchStmt::child_begin() {
  return &HandlerBlock;
}

Stmt::child_iterator CXXCatchStmt::child_end() {
  return &HandlerBlock + 1;
}

// CXXTryStmt
Stmt::child_iterator CXXTryStmt::child_begin() {
  return reinterpret_cast<Stmt **>(this + 1);
}

Stmt::child_iterator CXXTryStmt::child_end() {
  return reinterpret_cast<Stmt **>(this + 1) + NumHandlers + 1;
}
