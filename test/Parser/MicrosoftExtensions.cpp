// RUN: %clang_cc1 %s -fsyntax-only -Wno-unused-value -Wmicrosoft -verify -fms-extensions

/* Microsoft attribute tests */
[repeatable][source_annotation_attribute( Parameter|ReturnValue )]
struct SA_Post{ SA_Post(); int attr; };

[returnvalue:SA_Post( attr=1)] 
int foo1([SA_Post(attr=1)] void *param);

namespace {
  [returnvalue:SA_Post(attr=1)] 
  int foo2([SA_Post(attr=1)] void *param);
}

class T {
  [returnvalue:SA_Post(attr=1)] 
  int foo3([SA_Post(attr=1)] void *param);
};

extern "C" {
  [returnvalue:SA_Post(attr=1)] 
  int foo5([SA_Post(attr=1)] void *param);
}


void uuidof_test1()
{  
  __uuidof(0);  // expected-error {{you need to include <guiddef.h> before using the '__uuidof' operator}}
}

typedef struct _GUID
{
    unsigned long  Data1;
    unsigned short Data2;
    unsigned short Data3;
    unsigned char  Data4[8];
} GUID;

struct __declspec(uuid(L"00000000-0000-0000-1234-000000000047")) uuid_attr_bad1 { };// expected-error {{'uuid' attribute requires parameter 1 to be a string}}
struct __declspec(uuid(3)) uuid_attr_bad2 { };// expected-error {{'uuid' attribute requires parameter 1 to be a string}}
struct __declspec(uuid("0000000-0000-0000-1234-0000500000047")) uuid_attr_bad3 { };// expected-error {{uuid attribute contains a malformed GUID}}
struct __declspec(uuid("0000000-0000-0000-Z234-000000000047")) uuid_attr_bad4 { };// expected-error {{uuid attribute contains a malformed GUID}}
struct __declspec(uuid("000000000000-0000-1234-000000000047")) uuid_attr_bad5 { };// expected-error {{uuid attribute contains a malformed GUID}}



struct __declspec(uuid("000000A0-0000-0000-C000-000000000046"))
struct_with_uuid { };
struct struct_without_uuid { };


int uuid_sema_test()
{
   struct_with_uuid var_with_uuid[1];
   struct_without_uuid var_without_uuid[1];

   __uuidof(struct_with_uuid);
   __uuidof(struct_without_uuid); // expected-error {{cannot call operator __uuidof on a type with no GUID}}
   __uuidof(struct_with_uuid*);
   __uuidof(struct_without_uuid*); // expected-error {{cannot call operator __uuidof on a type with no GUID}}

   __uuidof(var_with_uuid);
   __uuidof(var_without_uuid);// expected-error {{cannot call operator __uuidof on a type with no GUID}}
   __uuidof(var_with_uuid[1]);
   __uuidof(var_without_uuid[1]);// expected-error {{cannot call operator __uuidof on a type with no GUID}}
   __uuidof(&var_with_uuid[1]);
   __uuidof(&var_without_uuid[1]);// expected-error {{cannot call operator __uuidof on a type with no GUID}}

   __uuidof(0);
   __uuidof(1);// expected-error {{cannot call operator __uuidof on a type with no GUID}}
}
