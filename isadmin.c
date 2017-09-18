// See MSDN CheckTokenMembership function example
#include <stdio.h>
#include <windows.h>

#ifdef __TINYC__
DECLSPEC_IMPORT WINBOOL APIENTRY CheckTokenMembership (HANDLE , PSID , PBOOL );
//BOOL IsUserAnAdmin(void);
#endif

#pragma comment(lib, "advapi32.lib")

int main(void) {
/*++ 
Routine Description: This routine returns TRUE if the caller's
process is a member of the Administrators local group. Caller is NOT
expected to be impersonating anyone and is expected to be able to
open its own process and process token. 
Arguments: None. 
Return Value: 
   TRUE - Caller has Administrators local group. 
   FALSE - Caller does not have Administrators local group. --
*/ 
BOOL b;
SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
PSID AdministratorsGroup; 
b = AllocateAndInitializeSid(
    &NtAuthority,
    2,
    SECURITY_BUILTIN_DOMAIN_RID,
    DOMAIN_ALIAS_RID_ADMINS,
    0, 0, 0, 0, 0, 0,
    &AdministratorsGroup); 
if(b) 
{
    if (!CheckTokenMembership( NULL, AdministratorsGroup, &b)) 
    {
         b = FALSE;
    } 
    FreeSid(AdministratorsGroup); 
}
printf("%d\n", b);
if(b) return 0;
else return 1;
}


