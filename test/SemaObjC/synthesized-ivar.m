// RUN: %clang_cc1 -fsyntax-only -fobjc-nonfragile-abi2 -verify %s
@interface I
{
}
@property int IP;
@end

@implementation I
@synthesize IP;
- (int) Meth {
   return IP;
}
@end

// rdar://7823675
int f0(I *a) { return a->IP; } // expected-error {{instance variable 'IP' is private}}

// rdar://8769582

@interface I1 {
 int protected_ivar;
}
@property int PROP_INMAIN;
@end

@interface I1() {
 int private_ivar;
}
@property int PROP_INCLASSEXT;
@end

@implementation I1
- (int) Meth {
   PROP_INMAIN = 1;
   PROP_INCLASSEXT = 2;
   protected_ivar = 1;	// OK
   return private_ivar; // OK
}
@end


@interface DER : I1
@end

@implementation DER
- (int) Meth {
   protected_ivar = 1;	// OK
   PROP_INMAIN = 1; // expected-error {{instance variable 'PROP_INMAIN' is private}}
   PROP_INCLASSEXT = 2; // expected-error {{instance variable 'PROP_INCLASSEXT' is private}}
   return private_ivar; // expected-error {{instance variable 'private_ivar' is private}}
}
@end

