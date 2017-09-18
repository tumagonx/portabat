#include <stdio.h>
#include <windows.h>
#include <shlwapi.h>
#include <rpcsal.h>
#include <imapi2.h>
#include <imapi2error.h>
#include <imapi2fs.h>
#include <imapi2fserror.h>

//Based what found on net under the name of iampi2.cpp. See also imapi2sample.cpp in WSDK

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")

  // supported UDF (latest is 2.60) :
  // 258 = 1.02 Most consumer devices
  // 336 = 1.50 Win2K
  // 512 = 2.00
  // 513 = 2.01 XP
  // 592 = 2.50 Vista
  // MediaType (from cd to bluray)
  // Bluray = UDF 2.50
  // DVD = UDF 1.02
  // Except for CD (ISO+Joliet+UDF), FS always UDF only
  // under XP HDDVD always failed
  // under XP if media not choosen, default is CD
  // media selection decide max iso size
  // Thus important media type for iso data image is CD, DVDDL and BD

int main(int argc, char ** argv) {
  HRESULT hr = 0;
  CoInitialize(NULL);
  IFileSystemImage* image = NULL;
  IFileSystemImageResult* result = NULL;
  IFsiDirectoryItem* root = NULL;
  IStream* bootfile = NULL;
  wchar_t* bootfilename;
  IStream* isostream = NULL;
  IBootOptions *bootopt = NULL;
  EmulationType	emul = EmulationNone;
  int isbootable = 0;
  STATSTG stg;
  DWORD size;
  DWORD written = 0;
  HANDLE hFile;
  BOOL bErrorFlag = FALSE;
  LONG numFiles = 0;
  LONG numDirs = 0;
  LONG numBlocks = 0;
  unsigned int chunk = 536870912; //I dont know what to pick but somehow >1GB dont work
  
  if (argc < 4) {
    printf ("Usage:\n%s inrootdirectory outfile volname [bootfile emulation]\n",argv[0]);
    printf ("emulation: None|12MFloppy|144MFloppy|288MFloppy|Harddisk\n");
    return 1;
  }
  wchar_t** wargv = CommandLineToArgvW (GetCommandLineW(), &argc);
  if (argc > 4) {
    if (argc != 6) {
      printf("Both boot parameters must be specified\n");
      return 1;
    }
    bootfilename = wargv[4];
    if (wcsicmp(wargv[5], L"Harddisk") == 0)
      emul = EmulationHardDisk;
    if (wcsicmp(wargv[5], L"288MFloppy") == 0)
      emul = Emulation288MFloppy;
    if (wcsicmp(wargv[5], L"144MFloppy") == 0)
      emul = Emulation144MFloppy;
    if (wcsicmp(wargv[5], L"12MFloppy") == 0)
      emul = Emulation12MFloppy;
    isbootable = 1;
  }
  
  hr = CoCreateInstance(CLSID_MsftFileSystemImage, NULL, CLSCTX_ALL, __uuidof(IFileSystemImage), (void**)&image);
  if (SUCCEEDED(hr)) hr = image->ChooseImageDefaultsForMediaType(IMAPI_MEDIA_TYPE_BDR); //First, assign biggest media
  else { printf ("Error: CoCreateInstance failed\n"); exit(1); }
  hr = image->put_VolumeName(wargv[3]);
  if (SUCCEEDED(hr)) {
    printf ("Collecting data...\nVolume name : %S\n", wargv[3]);
    hr = image->get_Root(&root);
  } else exit(1);
  if (isbootable) {
    if (SUCCEEDED(hr)) hr = CoCreateInstance(CLSID_BootOptions, NULL, CLSCTX_ALL, __uuidof(IBootOptions), (void**)&bootopt);
    if (SUCCEEDED(hr)) hr = SHCreateStreamOnFileW(bootfilename, STGM_READ | STGM_SHARE_DENY_WRITE, &bootfile);
    else { printf ("Error: CoCreateInstance failed\n"); exit(1); }
    if (SUCCEEDED(hr)) hr = bootopt->put_Emulation(emul);
    else { printf ("Error: Getting boot file %s failed\n", bootfilename); exit(1); }
    if (SUCCEEDED(hr)) hr = bootopt->put_PlatformId(PlatformX86);
    if (SUCCEEDED(hr)) hr = bootopt->AssignBootImage(bootfile);
    if (SUCCEEDED(hr)) hr = image->put_BootImageOptions (bootopt);
    else { printf ("Error: Assigning boot file failed\n"); exit(1); }
  }
  if (SUCCEEDED(hr)) hr = root->AddTree(wargv[1], false);
  if (FAILED(hr)) { printf ("Error: Adding content %S failed\n", wargv[1]); exit(1); }
  // find smallest (more compatible) media to fit
  hr = image->ChooseImageDefaultsForMediaType(IMAPI_MEDIA_TYPE_CDR);
  if (FAILED(hr)) {
    hr = image->ChooseImageDefaultsForMediaType(IMAPI_MEDIA_TYPE_DVDDASHR);
    if (FAILED(hr)) {
      hr = image->ChooseImageDefaultsForMediaType(IMAPI_MEDIA_TYPE_BDR); // Back to BD
      printf("MediaType: Bluray (UDF 2.5)\n");
    }
    else printf("MediaType: DVD (UDF)\n");
  }
  else printf("MediaType: CD (ISO9660+Joliet+UDF)\n");
  if (SUCCEEDED(hr)) hr = image->CreateResultImage(&result);
  else { printf ("Error: Adding content %S failed\n", wargv[1]); exit(1); }
  if (SUCCEEDED(hr)) hr = result->get_ImageStream(&isostream);

  hFile = CreateFileW(wargv[2], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); 
  if (hFile == INVALID_HANDLE_VALUE) {
    printf ("Error: opening %S\n",wargv[2]);
    exit(1);
  }

  isostream->Stat(&stg, 1);
  if (stg.cbSize.QuadPart < (ULONGLONG)chunk)
    chunk = stg.cbSize.QuadPart;

  char* data = new char[chunk];
  do {
  hr = isostream->Read(data, chunk, &size);
  bErrorFlag = WriteFile(hFile, data, size, &written, NULL);
  } while (SUCCEEDED(hr) && (TRUE == bErrorFlag) && (size > 0));
  
  if (FALSE == bErrorFlag) {
    printf ("Error: writing %S\n",wargv[3]);
    CloseHandle(hFile);
    DeleteFileW(wargv[3]);
    exit(1);
  }
  
  result->get_TotalBlocks(&numBlocks);
  image->get_FileCount(&numFiles);
  image->get_DirectoryCount(&numDirs);
  printf ("Total: %d Block(s), %d Folder(s) and %d File(s)\nFinished.\n", numBlocks, numDirs, numFiles);  
  CloseHandle(hFile);

  delete data;
  isostream->Release();
  root->Release();
  result->Release();
  image->Release();
    
  CoUninitialize();
  return 0;
}