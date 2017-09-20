//See http://www.codeproject.com/KB/cs/compresswithwinshellapics.aspx

#include <stdio.h>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <shldisp.h>

#define FOF_NO_UI (FOF_SILENT | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_NOCONFIRMMKDIR)

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "shlwapi.lib")

int main (int argc, char **argv) {
wchar_t fin[30000];
wchar_t odir[30000];
HRESULT hr;
IShellDispatch *pISD;
Folder *pToFolder = NULL;
VARIANT vDir, vFile, vOpt;
wchar_t** wargv = CommandLineToArgvW (GetCommandLineW(), &argc);

if (argc != 3) {
  printf("Usage: %S infile emptyDir/non-existentDir\n", wargv[0]);
  return 1;
}
if (PathFileExistsW(wargv[1])) {
  if (PathIsDirectoryW(wargv[1]))
    return 1;
  else 
    if (GetFullPathNameW(wargv[1], 30000, fin, NULL) < 4) return 1;
} else return 1;
if (PathFileExistsW(wargv[2])) {
  if (PathIsDirectoryW(wargv[2])) {
    if (PathIsDirectoryEmpty (wargv[2])) {
      if (GetFullPathNameW(wargv[2], 30000, odir, NULL) < 4) return 1;
    } else
      return 1;
  }
} else {
  if (GetFullPathNameW(wargv[2], 30000, odir, NULL) < 4) return 1;
  if (SHCreateDirectoryExW(NULL, odir, NULL) != ERROR_SUCCESS) return 1; //deprecation or just die?
}

CoInitialize(NULL);

hr = CoCreateInstance(CLSID_Shell, NULL, CLSCTX_INPROC_SERVER, IID_IShellDispatch, (void **)&pISD);

if (SUCCEEDED(hr))
{
  VariantInit(&vDir);
  vDir.vt = VT_BSTR;
  vDir.bstrVal = odir;
  // Destination is our zip file
  hr = pISD->NameSpace(vDir, &pToFolder);
  if (SUCCEEDED(hr))
  {
      Folder *pFromFolder = NULL;
      VariantInit(&vFile);
      vFile.vt = VT_BSTR;
      vFile.bstrVal = fin;

      pISD->NameSpace(vFile, &pFromFolder);
      FolderItems *fi = NULL;
      pFromFolder->Items(&fi);

      VariantInit(&vOpt);
      vOpt.vt = VT_I4;
      vOpt.lVal = FOF_NO_UI; // Do not display a progress dialog box

      // Creating a new Variant with pointer to FolderItems to be copied
      VARIANT newV;
      VariantInit(&newV);

      newV.vt = VT_DISPATCH;
      newV.pdispVal = fi;

      hr = pToFolder->CopyHere(newV, vOpt);
      Sleep(500);
      pFromFolder->Release();
      pToFolder->Release();
    }
  pISD->Release();
}
CoUninitialize();
if (SUCCEEDED(hr))
  return 0;
return 1;
}