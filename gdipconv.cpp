#include <windows.h>
#include <gdiplus.h>
#include <stdio.h>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "shell32.lib")

// See https://msdn.microsoft.com/en-us/library/ms533837(VS.85).aspx
using namespace Gdiplus;
int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
  UINT  num = 0;          // number of image encoders
  UINT  size = 0;         // size of the image encoder array in bytes

  ImageCodecInfo* pImageCodecInfo = NULL;

  GetImageEncodersSize(&num, &size);
  if(size == 0)
    return -1;  // Failure

  pImageCodecInfo = (ImageCodecInfo*)(malloc(size));
  if(pImageCodecInfo == NULL)
    return -1;  // Failure

  GetImageEncoders(num, size, pImageCodecInfo);

  for(UINT j = 0; j < num; ++j)
  {
    if( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
    {
      *pClsid = pImageCodecInfo[j].Clsid;
      free(pImageCodecInfo);
      return j;  // Success
    }
  }

  free(pImageCodecInfo);
  return -1;  // Failure
}

int main(int argc, char **argv)
{
  wchar_t imDrive[_MAX_DRIVE];
  wchar_t imDir[_MAX_DIR * 3];
  wchar_t imFName[_MAX_FNAME * 2 + 5];
  wchar_t imExt[_MAX_EXT + 5];
  wchar_t *mimeType;
  
  // Initialize GDI+.
  GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
   
  CLSID   encoderClsid;
  Status  stat;
  if (argc != 3) {
    printf ("Usage:\n%s infile outfile\n",argv[0]);
    return 1;
  }
   
  wchar_t** wargv = CommandLineToArgvW (GetCommandLineW(), &argc);
  _wsplitpath(wargv[2], imDrive, imDir, imFName, imExt);
  // See HKEY_CLASSES_ROOT\MIME\Database\Content Type
  if (!wcsicmp(imExt, L".png"))
    mimeType = L"image/png";
  else if (!wcsicmp(imExt, L".tif") || !wcsicmp(imExt, L".tiff"))
    mimeType = L"image/tiff";
  else if (!wcsicmp(imExt, L".jpg") || !wcsicmp(imExt, L".jpeg"))
    mimeType = L"image/jpeg";
  else if (!wcsicmp(imExt, L".bmp"))
    mimeType = L"image/bmp";
  else if (!wcsicmp(imExt, L".gif"))
    mimeType = L"image/gif";
  else {
    printf("\nUnsupported output format: %S\n",imExt);
    return 1;
  }
  Image*   image = new Image(wargv[1]);
   

  // Get the CLSID of encoder.
  GetEncoderClsid(mimeType, &encoderClsid);

  stat = image->Save(wargv[2], &encoderClsid, NULL);

  if(stat == Ok)
    printf("%S was saved successfully\n",wargv[2]);
  else
    printf("Failure: stat = %d\n", stat); 

  delete image;
  GdiplusShutdown(gdiplusToken);
  return 0;
}