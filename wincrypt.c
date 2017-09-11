/* by hasherezade */

#include <stdio.h>
#include <Windows.h>
#include <shellapi.h>

#define BLOCK_LEN 128

#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "shell32.lib")



//params: <input file> <output file> <key> <is decrypt mode>
int main( int argc, char *argv[])
{
    wchar_t** wargv = CommandLineToArgvW (GetCommandLineW(), &argc);
    wchar_t *filename = L"plain.txt"; //input file
    wchar_t *filename2 = L"out1.enc"; //output file

    wchar_t default_key[] = L"3igcZhRdWq96m3GUmTAiv9";
    wchar_t *key_str = default_key;
    
    BOOL isDecrypt = FALSE;
    if (argc >= 3) {
        filename = wargv[1];
        filename2 = wargv[2];
        if (argc >= 4) {
            key_str = wargv[3];
        }
        if (argc >=5 && wargv[4][0] > '0') {
            isDecrypt = TRUE;
        }
    }
    if (isDecrypt) {
        printf("DECRYPTING\n");
    } else {
        printf("ENCRYPTING\n");
    }

    size_t len = lstrlenW(key_str);

    printf("Key: %S\n", key_str);
    printf("Key len: %#x\n", len);
    printf("Input File: %S\n", filename);
    printf("Output File: %S\n", filename2);
    printf("----\n");

    DWORD dwStatus = 0;
    BOOL bResult = FALSE;
    wchar_t info[] = L"Microsoft Enhanced RSA and AES Cryptographic Provider";
    HCRYPTPROV hProv;
    if (!CryptAcquireContextW(&hProv, NULL, info, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)){
        dwStatus = GetLastError();
        printf("CryptAcquireContext failed: %x\n", dwStatus);
        CryptReleaseContext(hProv, 0);
        system("pause");
		return (-1);
         
        return dwStatus;
    }
    HCRYPTHASH hHash;
    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)){
        dwStatus = GetLastError();
        printf("CryptCreateHash failed: %x\n", dwStatus);
        CryptReleaseContext(hProv, 0);
        system("pause");
		return (-1);
    }

    if (!CryptHashData(hHash, (BYTE*)key_str, len, 0)) {
        DWORD err = GetLastError();
        printf ("CryptHashData Failed : %#x\n", err);
        system("pause");
        return (-1);
    }
    printf ("[+] CryptHashData Success\n");

    HCRYPTKEY hKey;
    if (!CryptDeriveKey(hProv, CALG_AES_128, hHash, 0,&hKey)){
        dwStatus = GetLastError();
        printf("CryptDeriveKey failed: %x\n", dwStatus);
        CryptReleaseContext(hProv, 0);
        system("pause");
        return (-1);
    }
    printf ("[+] CryptDeriveKey Success\n");
    
    const size_t chunk_size = BLOCK_LEN;
    BYTE chunk[128];
    DWORD read = 0;
    DWORD written = 0;

    HANDLE hInpFile = CreateFileW(filename, GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
    HANDLE hOutFile = CreateFileW(filename2, GENERIC_WRITE, 0,  NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);          

    if (hInpFile == NULL) {
        printf("Cannot open input file!\n");
        system("pause");
        return (-1);
    }
    if (hOutFile == NULL) {
        printf("Cannot open output file!\n");
        system("pause");
        return (-1);
    }
    while (bResult = ReadFile(hInpFile, chunk, chunk_size, &read, NULL)) {
        if (0 == read){
            break;
        }
        DWORD ciphertextLen = BLOCK_LEN;
        //printf("read: %d\n", read);
        if (isDecrypt) {
            if (!CryptDecrypt(hKey, NULL, FALSE, 0,chunk, &ciphertextLen)) {
                printf("failed!\n");
                break;
            }
        } else {
            if (!CryptEncrypt(hKey, NULL, FALSE, 0,chunk, &ciphertextLen, read)) {
                printf("failed!\n");
                break;
            }
        }
        if (!WriteFile(hOutFile, chunk, ciphertextLen, &written, NULL)) {
            printf("writing failed!\n");
            break;
        }
        memset(chunk, 0, chunk_size);
    }

    CryptReleaseContext(hProv, 0);
    CryptDestroyKey(hKey);
    CryptDestroyHash(hHash);

    CloseHandle(hInpFile);
    CloseHandle(hOutFile);
    printf ("[+] CryptEncrypt OK!\n");
    system("pause");
    return 0;
}
