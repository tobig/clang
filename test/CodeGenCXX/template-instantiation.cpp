// RUN: %clang_cc1 %s -triple=x86_64-apple-darwin10 -emit-llvm -o - | FileCheck %s

// CHECK-NOT: @_ZTVN5test118stdio_sync_filebufIwEE = constant
// CHECK: @_ZTVN5test018stdio_sync_filebufIwEE = constant

// CHECK: define linkonce_odr void @_ZN5test21CIiEC1Ev(
// CHECK: define linkonce_odr void @_ZN5test21CIiE6foobarIdEEvT_(
// CHECK: define available_externally void @_ZN5test21CIiE6zedbarEd(

namespace test0 {
  struct  basic_streambuf   {
    virtual       ~basic_streambuf();
  };
  template<typename _CharT >
  struct stdio_sync_filebuf : public basic_streambuf {
    virtual void      xsgetn();
  };

  // This specialization should cause the vtable to be emitted, even with
  // the following extern template declaration.
  template<> void stdio_sync_filebuf<wchar_t>::xsgetn()  {
  }
  extern template class stdio_sync_filebuf<wchar_t>;
}

namespace test1 {
  struct  basic_streambuf   {
    virtual       ~basic_streambuf();
  };
  template<typename _CharT >
  struct stdio_sync_filebuf : public basic_streambuf {
    virtual void      xsgetn();
  };

  // Just a declaration should not force the vtable to be emitted.
  template<> void stdio_sync_filebuf<wchar_t>::xsgetn();
}

namespace test2 {
  template<typename T1>
  class C {
    virtual ~C();
    void zedbar(double) {
    }
    template<typename T2>
    void foobar(T2 foo) {
    }
  };
  extern template class C<int>;
  void g() {
    // The extern template declaration should not prevent us from producing
    // the implicit constructor (test at the top).
    C<int> a;

    // or foobar(test at the top).
    a.foobar(0.0);

    // But it should prevent zebbar
    // (test at the top).
    a.zedbar(0.0);
  }
}