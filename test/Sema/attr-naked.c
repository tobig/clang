// RUN: %clang_cc1 %s -verify -fsyntax-only

int a __attribute__((naked)); // expected-warning {{'naked' attribute only applies to function types}}

void t1() __attribute__((naked));

void t2() __attribute__((naked(2))); // expected-error {{attribute requires 0 argument(s)}}

