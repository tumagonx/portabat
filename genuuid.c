#include <stdio.h>
#include <windows.h>
#ifdef __TINYC__
#define RPC_ENTRY __stdcall
#define RPCRTAPI __declspec(dllimport)
typedef unsigned char *RPC_CSTR;
typedef long RPC_STATUS;
RPCRTAPI RPC_STATUS RPC_ENTRY UuidCreate(UUID *Uuid);
RPCRTAPI RPC_STATUS RPC_ENTRY UuidToStringA(UUID *Uuid,RPC_CSTR *StringUuid);
RPCRTAPI RPC_STATUS RPC_ENTRY RpcStringFreeA(RPC_CSTR *String);
#endif
int main(void)
{
  UUID Uuid;
  char *str;
  UuidCreate(&Uuid);
  UuidToStringA(&Uuid, (RPC_CSTR*)&str);
  printf(str);
  RpcStringFreeA((RPC_CSTR*)&str);
  return 0;
}