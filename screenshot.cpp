/*
 * Copyright (c) 2009, The Mozilla Foundation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Mozilla Foundation nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY The Mozilla Foundation ''AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL The Mozilla Foundation BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Contributors:
 *   Ted Mielczarek <ted.mielczarek@gmail.com>
 *   Bartosz Wiklak <bwiklak@gmail.com>
 */
/*
 *  NAME:
 *		screenshot - Save a screenshot of the Windows desktop or window in .png format.
 *
 *  SYNOPSIS:
 *		screenshot [-wt WINDOW_TITLE | -rc LEFT TOP RIGHT BOTTOM | -o FILENAME | -h]
 *
 *  OPTIONS:
 *		-wt	WINDOW_TITLE 
 *							Select window with this title. Title must not contain space (" ").
 *		-rc LEFT TOP RIGHT BOTTOM 
 *							Crop source. If no WINDOW_TITLE is provided 
 *							(0,0) is left top corner of desktop,
 *							else if WINDOW_TITLE maches a desktop window
 *							(0,0) is it's top left corner.
 *		-o FILENAME
 *							Output file name, if none, the image will be saved
 *							as "screenshot.png" in the current working directory.
  *		-h 
 *							Shows this help info.
 *
 *
 *
 *  Requires GDI+. All linker dependencies are specified explicitly in this
 *  file, so you can compile screenshot.exe by simply running:
 *    cl screenshot.cpp
 *
 *  http://blog.mozilla.com/ted/2009/02/05/command-line-screenshot-tool-for-windows/
 */

#include <windows.h>
#include <gdiplus.h>
#include <string.h>  
#include <stdio.h> 

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

// From http://msdn.microsoft.com/en-us/library/ms533843%28VS.85%29.aspx
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

int wmain(int argc, wchar_t** argv)
{
  HWND windowSearched 			= NULL;
  RECT rect						= {0};
  wchar_t filename[MAX_PATH]	= {0};

  bool rectProvided = false;

  if( argc>1 && wcscmp( argv[1], L"-h" )==0 ){
	printf(	"\nNAME:\n"
 			"\tscreenshot -\tSave a screenshot of the Windows desktop\n\t\t\tor window in .png format.\n\n"
			"SYNOPSIS:\n"
 			"\tscreenshot [ -wt WINDOW_TITLE |\n\t\t     -wh WINDOW_HANDLE |\n\t\t     -rc LEFT TOP RIGHT BOTTOM |\n\t\t     -o  FILENAME |\n\t\t     -h ]\n\n"
			"OPTIONS:\n"
 			"\t-wt WINDOW_TITLE\n"
 			"\t\t\tSelect window with this title.\n\t\t\tTitle must not contain space (\" \").\n"
			"\t-wh WINDOW_HANDLE\n"
 			"\t\t\tSelect window by it's handle\n\t\t\t(representad as hex string - f.e. \"0012079E\") \n"
 			"\t-rc LEFT TOP RIGHT BOTTOM\n" 
 			"\t\t\tCrop source. If no WINDOW_TITLE is provided\n" 
 			"\t\t\t(0,0) is left top corner of desktop,\n"
 			"\t\t\telse if WINDOW_TITLE maches a desktop window\n"
 			"\t\t\t(0,0) is it's top left corner.\n"
 			"\t-o FILENAME\n"
 			"\t\t\tOutput file name, if none, the image will be saved\n"
 			"\t\t\tas \"screenshot.png\" in the current working directory.\n"
 			"\t-h\n"
 			"\t\t\tShows this help info.\n" );
	  return 1;
  }

  for(short i=1; i < argc; i++ ) {
		if( wcscmp( argv[i], L"-wt" )==0 && i+1<argc ){
			windowSearched = FindWindowW( NULL, argv[i+1] );
			if(windowSearched){
				SetWindowPos( windowSearched, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW ); 
				Sleep(200); //TODO: Arbitrary waiting time for window to become topmost
				if(!rectProvided) GetWindowRect(windowSearched, &rect);
			}
		}else  if( wcscmp( argv[i], L"-wh" )==0 && i+1<argc ){
			windowSearched = (HWND)wcstoul( argv[i+1],NULL,16); //TODO: How does it work on 64bit enviroment?
			if(windowSearched){
				SetWindowPos( windowSearched, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW ); 
				Sleep(200); //TODO: Arbitrary waiting time for window to become topmost
				if(!rectProvided) GetWindowRect(windowSearched, &rect);
			}
		}else if( wcscmp( argv[i], L"-rc" )==0 && i+4<argc ){
			rect.left	= _wtoi( argv[i+1] );
			rect.top	= _wtoi( argv[i+2] );
			rect.right	= _wtoi( argv[i+3] );
			rect.bottom	= _wtoi( argv[i+4] );

			rectProvided = true;
		}else if( wcscmp( argv[i], L"-o" )==0 && i+1<argc ){
			wcscpy( filename,  argv[i+1] );
		}
  }

  GdiplusStartupInput gdiplusStartupInput;
  ULONG_PTR gdiplusToken;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);
  
  /* If windowSearched and rectangle was provided we should recalculate rectangle to the windowSearched coordinates */
  if(windowSearched && rectProvided){
	RECT wrect;
	GetWindowRect(windowSearched, &wrect);
	OffsetRect( &rect, wrect.left, wrect.top );
  }

  if( wcslen(filename)==0 ) wcscpy( filename,  L"screenshot.png" );

  HWND desktop = GetDesktopWindow();
  HDC desktopdc = GetDC(desktop);
  HDC mydc = CreateCompatibleDC(desktopdc);

  int width  = (rect.right-rect.left==0) ? GetSystemMetrics(SM_CXSCREEN) : rect.right-rect.left;
  int height = (rect.bottom-rect.top==0) ? GetSystemMetrics(SM_CYSCREEN) : rect.bottom-rect.top;
  
  HBITMAP mybmp = CreateCompatibleBitmap(desktopdc, width, height);
  HBITMAP oldbmp = (HBITMAP)SelectObject(mydc, mybmp);
  BitBlt(mydc,0,0,width,height,desktopdc,rect.left,rect.top, SRCCOPY|CAPTUREBLT);
  SelectObject(mydc, oldbmp);
	
  if(windowSearched) SetWindowPos( windowSearched, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW ); 
  
  Bitmap* b = Bitmap::FromHBITMAP(mybmp, NULL);
  CLSID  encoderClsid;
  Status stat = GenericError;
  if (b && GetEncoderClsid(L"image/png", &encoderClsid) != -1) {
    stat = b->Save(filename, &encoderClsid, NULL);
  }
  if (b)
    delete b;
  
  // cleanup
  GdiplusShutdown(gdiplusToken);
  ReleaseDC(desktop, desktopdc);
  DeleteObject(mybmp);
  DeleteDC(mydc);
  return stat != Ok; // return 0 on success, (Issue 2) 
					 // thanks to stephan...@gmail.com for reporting this bug.
}
