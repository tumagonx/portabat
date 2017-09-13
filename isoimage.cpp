#include <stdio.h>
#include <windows.h>
#include <shlwapi.h>
#include <rpcsal.h>
#include <imapi2.h>
#include <imapi2error.h>
#include <imapi2fs.h>
#include <imapi2fserror.h>

//Found on net under the name of iampi2.cpp See also imapi2sample.cpp in WDK

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "shlwapi.lib")

int main(int argc, char ** argv) {
  HRESULT hres = 0;
  CoInitialize(NULL);
  IFileSystemImage* image = NULL;
  IFileSystemImageResult* result = NULL;
  IFsiDirectoryItem* root = NULL;
  IStream* bootfile = NULL;
  wchar_t* bootfilename;
  IStream* r_i = NULL;
  IBootOptions *bootopt = NULL;
  EmulationType	emul = EmulationNone;
  int isbootable = 0;
  STATSTG stg;
  FILE* f;
  
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
    if (PathFileExistsW(wargv[4]) && !PathIsDirectoryW(wargv[4])) {
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
  }

  hres = CoCreateInstance(CLSID_MsftFileSystemImage, NULL, CLSCTX_ALL, __uuidof(IFileSystemImage), (void**)&image);
  if (SUCCEEDED(hres)) hres = image->put_UDFRevision(102); //set to min
  else { printf ("Init: Image failed\n"); return 1; }
  if (FAILED(hres)) hres = image->put_UDFRevision(258);
  if (SUCCEEDED(hres)) hres = image->put_FileSystemsToCreate((FsiFileSystems)(FsiFileSystemJoliet | FsiFileSystemISO9660 | FsiFileSystemUDF ));
  else { printf ("Setting: UDF version failed\n"); return 1; }
  if (FAILED(hres)) hres = image->put_FileSystemsToCreate((FsiFileSystems)(FsiFileSystemJoliet | FsiFileSystemISO9660 ));
  if (SUCCEEDED(hres)) hres = image->put_VolumeName(wargv[3]);
  else { printf ("Setting: FS type failed\n"); return 1; }
  if (SUCCEEDED(hres)) hres = image->ChooseImageDefaultsForMediaType(IMAPI_MEDIA_TYPE_BDR); //set to max
  else { printf ("Setting: Volume name as %S failed\n", wargv[3]); return 1; }
  if (FAILED(hres)) hres = image->ChooseImageDefaultsForMediaType(IMAPI_MEDIA_TYPE_HDDVDR);
  if (FAILED(hres)) hres = image->ChooseImageDefaultsForMediaType(IMAPI_MEDIA_TYPE_DVDDASHR_DUALLAYER);
  if (FAILED(hres)) hres = image->ChooseImageDefaultsForMediaType(IMAPI_MEDIA_TYPE_DVDPLUSR_DUALLAYER);
  if (FAILED(hres)) hres = image->ChooseImageDefaultsForMediaType(IMAPI_MEDIA_TYPE_DVDDASHR);
  if (FAILED(hres)) hres = image->ChooseImageDefaultsForMediaType(IMAPI_MEDIA_TYPE_DVDPLUSR);
  if (FAILED(hres)) hres = image->ChooseImageDefaultsForMediaType(IMAPI_MEDIA_TYPE_CDR);
  if (SUCCEEDED(hres)) hres = image->get_Root(&root);
  else { printf ("Setting: Media type failed\n"); return 1; }
  if (isbootable) {
    if (SUCCEEDED(hres)) hres = CoCreateInstance(CLSID_BootOptions, NULL, CLSCTX_ALL, __uuidof(IBootOptions), (void**)&bootopt);
    else { printf ("Init: Root failed\n"); return 1; }
    if (SUCCEEDED(hres)) hres = SHCreateStreamOnFileW(bootfilename, STGM_READ | STGM_SHARE_DENY_WRITE, &bootfile);
    else { printf ("Init: Boot image failed\n"); return 1; }
    if (SUCCEEDED(hres)) hres = bootopt->put_Emulation(emul);
    else { printf ("Setting: Getting boot file failed\n"); return 1; }
    if (SUCCEEDED(hres)) hres = bootopt->put_PlatformId(PlatformX86);
    else { printf ("Setting: Boot emulation failed\n"); return 1; }
    if (SUCCEEDED(hres)) hres = bootopt->AssignBootImage(bootfile);
    else { printf ("Setting: Boot platform failed\n"); return 1; }
    if (SUCCEEDED(hres)) hres = image->put_BootImageOptions (bootopt);
    else { printf ("Setting: Assign boot file failed\n"); return 1; }
  }
  if (SUCCEEDED(hres)) hres = root->AddTree(wargv[1], false);
  else { printf ("Setting: Boot image failed\n"); return 1; }
  if (SUCCEEDED(hres)) hres = image->CreateResultImage(&result);
  else { printf ("Error: Adding content %S failed\n", wargv[1]); return 1; }
  if (SUCCEEDED(hres)) hres = result->get_ImageStream(&r_i);
  else { printf ("Error: Creation failed\n"); return 1; }

  if (FAILED(hres)){ printf("Error: Getting image to write failed\n"); return 1; }

  LONG numFiles = 0;
  LONG numDirs = 0;
  hres = image->get_FileCount(&numFiles);
  hres = image->get_DirectoryCount(&numDirs);

  printf ("Total: %d Folder(s) and %d File(s)", numDirs, numFiles);

  r_i->Stat(&stg, 1);
  char* data = new char[stg.cbSize.QuadPart];
  ULONG size;
  r_i->Read(data, stg.cbSize.QuadPart, &size);
  
  f = _wfopen(wargv[2], L"wb");
  if (f == NULL) {
    printf ("Error opening %S\n",wargv[2]);
    return 1;
  }
  fwrite(data, stg.cbSize.QuadPart, 1, f);
  fclose(f);
    
  delete data;
  r_i->Release();
  root->Release();
  result->Release();
  image->Release();
    
  CoUninitialize();
  return 0;
}