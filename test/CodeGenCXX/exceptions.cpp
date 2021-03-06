// RUN: %clang_cc1 %s -triple=x86_64-apple-darwin10 -emit-llvm -o - -fexceptions | FileCheck %s

typedef typeof(sizeof(0)) size_t;

// This just shouldn't crash.
namespace test0 {
  struct allocator {
    allocator();
    allocator(const allocator&);
    ~allocator();
  };

  void f();
  void g(bool b, bool c) {
    if (b) {
      if (!c)
        throw allocator();

      return;
    }
    f();
  }
}

namespace test1 {
  struct A { A(int); A(int, int); ~A(); void *p; };

  A *a() {
    // CHECK:    define [[A:%.*]]* @_ZN5test11aEv()
    // CHECK:      [[NEW:%.*]] = call noalias i8* @_Znwm(i64 8)
    // CHECK-NEXT: [[CAST:%.*]] = bitcast i8* [[NEW]] to [[A]]*
    // CHECK-NEXT: invoke void @_ZN5test11AC1Ei([[A]]* [[CAST]], i32 5)
    // CHECK:      ret [[A]]* [[CAST]]
    // CHECK:      call void @_ZdlPv(i8* [[NEW]])
    return new A(5);
  }

  A *b() {
    // CHECK:    define [[A:%.*]]* @_ZN5test11bEv()
    // CHECK:      [[NEW:%.*]] = call noalias i8* @_Znwm(i64 8)
    // CHECK-NEXT: [[CAST:%.*]] = bitcast i8* [[NEW]] to [[A]]*
    // CHECK-NEXT: [[FOO:%.*]] = invoke i32 @_ZN5test13fooEv()
    // CHECK:      invoke void @_ZN5test11AC1Ei([[A]]* [[CAST]], i32 [[FOO]])
    // CHECK:      ret [[A]]* [[CAST]]
    // CHECK:      call void @_ZdlPv(i8* [[NEW]])
    extern int foo();
    return new A(foo());
  }

  struct B { B(); ~B(); operator int(); int x; };
  B makeB();

  A *c() {
    // CHECK:    define [[A:%.*]]* @_ZN5test11cEv()
    // CHECK:      [[ACTIVE:%.*]] = alloca i1
    // CHECK-NEXT: store i1 true, i1* [[ACTIVE]] 
    // CHECK-NEXT: [[NEW:%.*]] = call noalias i8* @_Znwm(i64 8)
    // CHECK-NEXT: [[CAST:%.*]] = bitcast i8* [[NEW]] to [[A]]*
    // CHECK-NEXT: invoke void @_ZN5test11BC1Ev([[B:%.*]]* [[T0:%.*]])
    // CHECK:      [[T1:%.*]] = getelementptr inbounds [[B]]* [[T0]], i32 0, i32 0
    // CHECK-NEXT: [[T2:%.*]] = load i32* [[T1]], align 4
    // CHECK-NEXT: invoke void @_ZN5test11AC1Ei([[A]]* [[CAST]], i32 [[T2]])
    // CHECK:      store i1 false, i1* [[ACTIVE]]
    // CHECK-NEXT: invoke void @_ZN5test11BD1Ev([[B]]* [[T0]])
    // CHECK:      ret [[A]]* [[CAST]]
    // CHECK:      [[ISACTIVE:%.*]] = load i1* [[ACTIVE]]
    // CHECK-NEXT: br i1 [[ISACTIVE]]
    // CHECK:      call void @_ZdlPv(i8* [[NEW]])
    return new A(B().x);
  }

  A *d() {
    // CHECK:    define [[A:%.*]]* @_ZN5test11dEv()
    // CHECK:      [[ACTIVE:%.*]] = alloca i1
    // CHECK-NEXT: store i1 true, i1* [[ACTIVE]] 
    // CHECK-NEXT: [[NEW:%.*]] = call noalias i8* @_Znwm(i64 8)
    // CHECK-NEXT: [[CAST:%.*]] = bitcast i8* [[NEW]] to [[A]]*
    // CHECK-NEXT: invoke void @_ZN5test11BC1Ev([[B:%.*]]* [[T0:%.*]])
    // CHECK:      [[T1:%.*]] = invoke i32 @_ZN5test11BcviEv([[B]]* [[T0]])
    // CHECK:      invoke void @_ZN5test11AC1Ei([[A]]* [[CAST]], i32 [[T1]])
    // CHECK:      store i1 false, i1* [[ACTIVE]]
    // CHECK-NEXT: invoke void @_ZN5test11BD1Ev([[B]]* [[T0]])
    // CHECK:      ret [[A]]* [[CAST]]
    // CHECK:      [[ISACTIVE:%.*]] = load i1* [[ACTIVE]]
    // CHECK-NEXT: br i1 [[ISACTIVE]]
    // CHECK:      call void @_ZdlPv(i8* [[NEW]])
    return new A(B());
  }

  A *e() {
    // CHECK:    define [[A:%.*]]* @_ZN5test11eEv()
    // CHECK:      [[ACTIVE:%.*]] = alloca i1
    // CHECK-NEXT: store i1 true, i1* [[ACTIVE]] 
    // CHECK-NEXT: [[NEW:%.*]] = call noalias i8* @_Znwm(i64 8)
    // CHECK-NEXT: [[CAST:%.*]] = bitcast i8* [[NEW]] to [[A]]*
    // CHECK-NEXT: invoke void @_ZN5test11BC1Ev([[B:%.*]]* [[T0:%.*]])
    // CHECK:      [[T1:%.*]] = invoke i32 @_ZN5test11BcviEv([[B]]* [[T0]])
    // CHECK:      invoke void @_ZN5test11BC1Ev([[B]]* [[T2:%.*]])
    // CHECK:      [[T3:%.*]] = invoke i32 @_ZN5test11BcviEv([[B]]* [[T2]])
    // CHECK:      invoke void @_ZN5test11AC1Eii([[A]]* [[CAST]], i32 [[T1]], i32 [[T3]])
    // CHECK:      store i1 false, i1* [[ACTIVE]]
    // CHECK-NEXT: invoke void @_ZN5test11BD1Ev([[B]]* [[T2]])
    // CHECK:      invoke void @_ZN5test11BD1Ev([[B]]* [[T0]])
    // CHECK:      ret [[A]]* [[CAST]]
    // CHECK:      [[ISACTIVE:%.*]] = load i1* [[ACTIVE]]
    // CHECK-NEXT: br i1 [[ISACTIVE]]
    // CHECK:      call void @_ZdlPv(i8* [[NEW]])
    return new A(B(), B());
  }
  A *f() {
    return new A(makeB().x);
  }
  A *g() {
    return new A(makeB());
  }
  A *h() {
    return new A(makeB(), makeB());
  }

  A *i() {
    // CHECK:    define [[A:%.*]]* @_ZN5test11iEv()
    // CHECK:      [[X:%.*]] = alloca [[A]]*, align 8
    // CHECK:      [[ACTIVE:%.*]] = alloca i1
    // CHECK:      store i1 true, i1* [[ACTIVE]] 
    // CHECK-NEXT: [[NEW:%.*]] = call noalias i8* @_Znwm(i64 8)
    // CHECK-NEXT: [[CAST:%.*]] = bitcast i8* [[NEW]] to [[A]]*
    // CHECK-NEXT: invoke void @_ZN5test15makeBEv([[B:%.*]]* sret [[T0:%.*]])
    // CHECK:      [[T1:%.*]] = invoke i32 @_ZN5test11BcviEv([[B]]* [[T0]])
    // CHECK:      invoke void @_ZN5test11AC1Ei([[A]]* [[CAST]], i32 [[T1]])
    // CHECK:      store i1 false, i1* [[ACTIVE]]
    // CHECK-NEXT: store [[A]]* [[CAST]], [[A]]** [[X]], align 8
    // CHECK:      invoke void @_ZN5test15makeBEv([[B:%.*]]* sret [[T2:%.*]])
    // CHECK:      [[RET:%.*]] = load [[A]]** [[X]], align 8
    // CHECK:      invoke void @_ZN5test11BD1Ev([[B]]* [[T2]])
    // CHECK:      invoke void @_ZN5test11BD1Ev([[B]]* [[T0]])
    // CHECK:      ret [[A]]* [[RET]]
    // CHECK:      [[ISACTIVE:%.*]] = load i1* [[ACTIVE]]
    // CHECK-NEXT: br i1 [[ISACTIVE]]
    // CHECK:      call void @_ZdlPv(i8* [[NEW]])
    A *x;
    return (x = new A(makeB()), makeB(), x);
  }
}

namespace test2 {
  struct A {
    A(int); A(int, int); ~A();
    void *p;
    void *operator new(size_t);
    void operator delete(void*, size_t);
  };

  A *a() {
    // CHECK:    define [[A:%.*]]* @_ZN5test21aEv()
    // CHECK:      [[NEW:%.*]] = call i8* @_ZN5test21AnwEm(i64 8)
    // CHECK-NEXT: [[CAST:%.*]] = bitcast i8* [[NEW]] to [[A]]*
    // CHECK-NEXT: invoke void @_ZN5test21AC1Ei([[A]]* [[CAST]], i32 5)
    // CHECK:      ret [[A]]* [[CAST]]
    // CHECK:      invoke void @_ZN5test21AdlEPvm(i8* [[NEW]], i64 8)
    // CHECK:      call void @_ZSt9terminatev()
    return new A(5);
  }
}

namespace test3 {
  struct A {
    A(int); A(int, int); A(const A&); ~A();
    void *p;
    void *operator new(size_t, void*, double);
    void operator delete(void*, void*, double);
  };

  void *foo();
  double bar();
  A makeA(), *makeAPtr();

  A *a() {
    // CHECK:    define [[A:%.*]]* @_ZN5test31aEv()
    // CHECK:      [[FOO:%.*]] = call i8* @_ZN5test33fooEv()
    // CHECK:      [[BAR:%.*]] = call double @_ZN5test33barEv()
    // CHECK:      [[NEW:%.*]] = call i8* @_ZN5test31AnwEmPvd(i64 8, i8* [[FOO]], double [[BAR]])
    // CHECK-NEXT: [[CAST:%.*]] = bitcast i8* [[NEW]] to [[A]]*
    // CHECK-NEXT: invoke void @_ZN5test31AC1Ei([[A]]* [[CAST]], i32 5)
    // CHECK:      ret [[A]]* [[CAST]]
    // CHECK:      invoke void @_ZN5test31AdlEPvS1_d(i8* [[NEW]], i8* [[FOO]], double [[BAR]])
    // CHECK:      call void @_ZSt9terminatev()
    return new(foo(),bar()) A(5);
  }

  // rdar://problem/8439196
  A *b(bool cond) {

    // CHECK:    define [[A:%.*]]* @_ZN5test31bEb(i1 zeroext
    // CHECK:      [[SAVED0:%.*]] = alloca i8*
    // CHECK-NEXT: [[SAVED1:%.*]] = alloca i8*
    // CHECK-NEXT: [[CLEANUPACTIVE:%.*]] = alloca i1
    // CHECK-NEXT: [[TMP:%.*]] = alloca [[A]], align 8
    // CHECK:      [[TMPACTIVE:%.*]] = alloca i1
    // CHECK-NEXT: store i1 false, i1* [[TMPACTIVE]]
    // CHECK-NEXT: store i1 false, i1* [[CLEANUPACTIVE]]

    // CHECK:      [[COND:%.*]] = trunc i8 {{.*}} to i1
    // CHECK-NEXT: br i1 [[COND]]
    return (cond ?

    // CHECK:      [[FOO:%.*]] = call i8* @_ZN5test33fooEv()
    // CHECK-NEXT: [[NEW:%.*]] = call i8* @_ZN5test31AnwEmPvd(i64 8, i8* [[FOO]], double [[CONST:.*]])
    // CHECK-NEXT: store i8* [[NEW]], i8** [[SAVED0]]
    // CHECK-NEXT: store i8* [[FOO]], i8** [[SAVED1]]
    // CHECK-NEXT: store i1 true, i1* [[CLEANUPACTIVE]]
    // CHECK-NEXT: [[CAST:%.*]] = bitcast i8* [[NEW]] to [[A]]*
    // CHECK-NEXT: invoke void @_ZN5test35makeAEv([[A]]* sret [[TMP]])
    // CHECK:      store i1 true, i1* [[TMPACTIVE]]
    // CHECK-NEXT: invoke void @_ZN5test31AC1ERKS0_([[A]]* [[CAST]], [[A]]* [[TMP]])
    // CHECK:      store i1 false, i1* [[CLEANUPACTIVE]]
    // CHECK-NEXT: br label
    //   -> cond.end
            new(foo(),10.0) A(makeA()) :

    // CHECK:      [[MAKE:%.*]] = invoke [[A]]* @_ZN5test38makeAPtrEv()
    // CHECK:      br label
    //   -> cond.end
            makeAPtr());

    // cond.end:
    // CHECK:      [[RESULT:%.*]] = phi [[A]]* {{.*}}[[CAST]]{{.*}}[[MAKE]]
    // CHECK-NEXT: [[ISACTIVE:%.*]] = load i1* [[TMPACTIVE]]
    // CHECK-NEXT: br i1 [[ISACTIVE]]
    // CHECK:      invoke void @_ZN5test31AD1Ev
    // CHECK:      ret [[A]]* [[RESULT]]

    // in the EH path:
    // CHECK:      [[ISACTIVE:%.*]] = load i1* [[CLEANUPACTIVE]]
    // CHECK-NEXT: br i1 [[ISACTIVE]]
    // CHECK:      [[V0:%.*]] = load i8** [[SAVED0]]
    // CHECK-NEXT: [[V1:%.*]] = load i8** [[SAVED1]]
    // CHECK-NEXT: invoke void @_ZN5test31AdlEPvS1_d(i8* [[V0]], i8* [[V1]], double [[CONST]])
  }
}

namespace test4 {
  struct A {
    A(int); A(int, int); ~A();
    void *p;
    void *operator new(size_t, void*, void*);
    void operator delete(void*, size_t, void*, void*); // not a match
  };

  A *a() {
    // CHECK:    define [[A:%.*]]* @_ZN5test41aEv()
    // CHECK:      [[FOO:%.*]] = call i8* @_ZN5test43fooEv()
    // CHECK-NEXT: [[BAR:%.*]] = call i8* @_ZN5test43barEv()
    // CHECK-NEXT: [[NEW:%.*]] = call i8* @_ZN5test41AnwEmPvS1_(i64 8, i8* [[FOO]], i8* [[BAR]])
    // CHECK-NEXT: [[CAST:%.*]] = bitcast i8* [[NEW]] to [[A]]*
    // CHECK-NEXT: call void @_ZN5test41AC1Ei([[A]]* [[CAST]], i32 5)
    // CHECK-NEXT: ret [[A]]* [[CAST]]
    extern void *foo(), *bar();

    return new(foo(),bar()) A(5);
  }
}
