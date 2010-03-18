//===--- PreprocessingRecord.h - Record of Preprocessing --------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This file defines the PreprocessingRecord class, which maintains a record
//  of what occurred during preprocessing.
//
//===----------------------------------------------------------------------===//
#ifndef LLVM_CLANG_LEX_PREPROCESSINGRECORD_H
#define LLVM_CLANG_LEX_PREPROCESSINGRECORD_H

#include "clang/Lex/PPCallbacks.h"
#include "clang/Basic/SourceLocation.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/PointerUnion.h"
#include "llvm/Support/Allocator.h"
#include <vector>

namespace clang {
  class IdentifierInfo;
  class PreprocessingRecord;
}

/// \brief Allocates memory within a Clang preprocessing record.
void* operator new(size_t bytes, clang::PreprocessingRecord& PR,
                   unsigned alignment = 8) throw();

/// \brief Frees memory allocated in a Clang preprocessing record.
void operator delete(void* ptr, clang::PreprocessingRecord& PR,
                     unsigned) throw();

namespace clang {
  class MacroDefinition;

  /// \brief Base class that describes a preprocessed entity, which may be a
  /// preprocessor directive or macro instantiation.
  class PreprocessedEntity {
  public:
    /// \brief The kind of preprocessed entity an object describes.
    enum EntityKind {
      /// \brief A macro instantiation.
      MacroInstantiationKind,
      
      /// \brief A preprocessing directive whose kind is not specified.
      ///
      /// This kind will be used for any preprocessing directive that does not
      /// have a more specific kind within the \c DirectiveKind enumeration.
      PreprocessingDirectiveKind,
      
      /// \brief A macro definition.
      MacroDefinitionKind,
      
      FirstPreprocessingDirective = PreprocessingDirectiveKind,
      LastPreprocessingDirective = MacroDefinitionKind
    };

  private:
    /// \brief The kind of preprocessed entity that this object describes.
    EntityKind Kind;
    
    /// \brief The source range that covers this preprocessed entity.
    SourceRange Range;
    
  protected:
    PreprocessedEntity(EntityKind Kind, SourceRange Range)
      : Kind(Kind), Range(Range) { }
    
  public:
    /// \brief Retrieve the kind of preprocessed entity stored in this object.
    EntityKind getKind() const { return Kind; }
    
    /// \brief Retrieve the source range that covers this entire preprocessed 
    /// entity.
    SourceRange getSourceRange() const { return Range; }
    
    // Implement isa/cast/dyncast/etc.
    static bool classof(const PreprocessedEntity *) { return true; }

    // Only allow allocation of preprocessed entities using the allocator 
    // in PreprocessingRecord or by doing a placement new.
    void* operator new(size_t bytes, PreprocessingRecord& PR,
                       unsigned alignment = 8) throw() {
      return ::operator new(bytes, PR, alignment);
    }
    
    void* operator new(size_t bytes, void* mem) throw() {
      return mem;
    }
    
    void operator delete(void* ptr, PreprocessingRecord& PR, 
                         unsigned alignment) throw() {
      return ::operator delete(ptr, PR, alignment);
    }
    
    void operator delete(void*, std::size_t) throw() { }
    void operator delete(void*, void*) throw() { }
    
  private:
    // Make vanilla 'new' and 'delete' illegal for preprocessed entities.
    void* operator new(size_t bytes) throw();
    void operator delete(void* data) throw();
  };
  
  /// \brief Records the location of a macro instantiation.
  class MacroInstantiation : public PreprocessedEntity {
    /// \brief The name of the macro being instantiation.
    IdentifierInfo *Name;
    
    /// \brief The definition of this macro.
    MacroDefinition *Definition;
    
  public:
    MacroInstantiation(IdentifierInfo *Name, SourceRange Range,
                       MacroDefinition *Definition)
      : PreprocessedEntity(MacroInstantiationKind, Range), Name(Name), 
        Definition(Definition) { }
    
    /// \brief The name of the macro being instantiated.
    IdentifierInfo *getName() const { return Name; }
    
    /// \brief The definition of the macro being instantiated.
    MacroDefinition *getDefinition() const { return Definition; }

    // Implement isa/cast/dyncast/etc.
    static bool classof(const PreprocessedEntity *PE) {
      return PE->getKind() == MacroInstantiationKind;
    }
    static bool classof(const MacroInstantiation *) { return true; }

  };
  
  /// \brief Records the presence of a preprocessor directive.
  class PreprocessingDirective : public PreprocessedEntity {
  public:
    PreprocessingDirective(EntityKind Kind, SourceRange Range) 
      : PreprocessedEntity(Kind, Range) { }
    
    // Implement isa/cast/dyncast/etc.
    static bool classof(const PreprocessedEntity *PD) { 
      return PD->getKind() >= FirstPreprocessingDirective &&
             PD->getKind() <= LastPreprocessingDirective;
    }
    static bool classof(const PreprocessingDirective *) { return true; }    
  };
  
  /// \brief Record the location of a macro definition.
  class MacroDefinition : public PreprocessingDirective {
    /// \brief The name of the macro being defined.
    const IdentifierInfo *Name;
    
    /// \brief The location of the macro name in the macro definition.
    SourceLocation Location;

  public:
    explicit MacroDefinition(const IdentifierInfo *Name, SourceLocation Location,
                             SourceRange Range)
      : PreprocessingDirective(MacroDefinitionKind, Range), Name(Name), 
        Location(Location) { }
    
    /// \brief Retrieve the name of the macro being defined.
    const IdentifierInfo *getName() const { return Name; }
    
    /// \brief Retrieve the location of the macro name in the definition.
    SourceLocation getLocation() const { return Location; }
    
    // Implement isa/cast/dyncast/etc.
    static bool classof(const PreprocessedEntity *PE) {
      return PE->getKind() == MacroDefinitionKind;
    }
    static bool classof(const MacroDefinition *) { return true; }
  };
    
  /// \brief A record of the steps taken while preprocessing a source file,
  /// including the various preprocessing directives processed, macros 
  /// instantiated, etc.
  class PreprocessingRecord {
    /// \brief Allocator used to store preprocessing objects.
    llvm::BumpPtrAllocator BumpAlloc;

    /// \brief The set of preprocessed entities in this record, in order they
    /// were seen.
    std::vector<PreprocessedEntity *> PreprocessedEntities;
    
  public:
    /// \brief Allocate memory in the preprocessing record.
    void *Allocate(unsigned Size, unsigned Align = 8) {
      return BumpAlloc.Allocate(Size, Align);
    }
    
    /// \brief Deallocate memory in the preprocessing record.
    void Deallocate(void *Ptr) { }
    
    // Iteration over the preprocessed entities.
    typedef std::vector<PreprocessedEntity *>::iterator iterator;
    typedef std::vector<PreprocessedEntity *>::const_iterator const_iterator;
    iterator begin() { return PreprocessedEntities.begin(); }
    iterator end() { return PreprocessedEntities.end(); }
    const_iterator begin() const { return PreprocessedEntities.begin(); }
    const_iterator end() const { return PreprocessedEntities.end(); }
    
    /// \brief Add a new preprocessed entity to this record.
    void addPreprocessedEntity(PreprocessedEntity *Entity);
  };
  
  /// \brief Preprocessor callback action used to populate a preprocessing
  /// record.
  class PopulatePreprocessingRecord : public PPCallbacks {
    /// \brief The preprocessing record this action will populate.
    PreprocessingRecord &Record;
    
    /// \brief Mapping from MacroInfo structures to their definitions.
    llvm::DenseMap<const MacroInfo *, MacroDefinition *> MacroDefinitions;

  public:
    explicit PopulatePreprocessingRecord(PreprocessingRecord &Record)
      : Record(Record) { }
    
    virtual void MacroExpands(const Token &Id, const MacroInfo* MI);
    virtual void MacroDefined(const IdentifierInfo *II, const MacroInfo *MI);
  };
  
} // end namespace clang

inline void* operator new(size_t bytes, clang::PreprocessingRecord& PR,
                          unsigned alignment) throw() {
  return PR.Allocate(bytes, alignment);
}

inline void operator delete(void* ptr, clang::PreprocessingRecord& PR,
                            unsigned) throw() {
  PR.Deallocate(ptr);
}

#endif // LLVM_CLANG_LEX_PREPROCESSINGRECORD_H