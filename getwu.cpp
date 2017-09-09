#ifndef _UNICODE
#define _UNCODE
#endif
#ifndef UNICODE
#define UNICODE
#endif

#include <time.h>
#include <windows.h>
#include <tchar.h>
#include <wuapi.h>
#include <iostream>
#include <wuerror.h>
#include <shellapi.h>
#include <wininet.h>

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")

//See https://stackoverflow.com/questions/12949018/windows-update-agent-pure-win32-apis

using namespace std;


int _tmain(int argc, _TCHAR* argv[])
{
    HRESULT hr;
    hr = CoInitialize(NULL);
    int verbose = 0;
 
    if (!InternetCheckConnection(TEXT("http://www.windowsupdate.com"), FLAG_ICC_FORCE_CONNECTION , NULL)) {
      if (verbose)
        wcout << L"Error connecting to microsoft..."<<endl;
      return 0;
      
    }

    IUpdateSession* iUpdate;
    IUpdateSearcher* searcher;
    ISearchResult* results;
    BSTR criteria;
    if (argc >= 2) {
      if (_tcscmp(argv[1], TEXT("-v")) == 0)
        verbose = 1;
      else
        if (argc == 2)
          criteria = SysAllocString(argv[1]);
      if (argc == 3) 
          criteria = SysAllocString(argv[2]);
    } else criteria = SysAllocString(L"Type='Software' and IsInstalled=0");

    hr = CoCreateInstance(CLSID_UpdateSession, NULL, CLSCTX_INPROC_SERVER, IID_IUpdateSession, (LPVOID*)&iUpdate);
    hr = iUpdate->CreateUpdateSearcher(&searcher);
    if (verbose)
      wcout << L"Searching for updates..."<<endl;

    hr = searcher->Search(criteria, &results); 
    SysFreeString(criteria);

    switch(hr)
    {
    case S_OK:
        if (verbose)
          wcout<<L"List of applicable items on the machine:"<<endl;
        break;
    case WU_E_LEGACYSERVER:
        if (verbose)
          wcout<<L"No server selection enabled"<<endl;
        return 0;
    case WU_E_INVALID_CRITERIA:
        if (verbose)
          wcout<<L"Invalid search criteria"<<endl;
        return 0;
    default:
        if (verbose)
          wcout << L"Error getting update catalog, please retry"<<endl;
        return 0;
    }

    IUpdateCollection *updateList, *bundledList;
    IUpdate *updateItem, *bundledItem;
    IStringCollection *replacedIDs;
    IUpdateIdentity *identity;
    LONG updateSize, bundledSize, downloadSize, replacedSize;
    BSTR updateName, urlString, descString, replacedID, identityID;
    IUpdateDownloadContentCollection *downloadList;
    IUpdateDownloadContent *downloadItem;
    VARIANT_BOOL deltaAvailable, deltaPreferred, isBeta;
    DownloadPriority prioLevel;
    DeploymentAction actState;
    DECIMAL maxDSize, minDSize;
    DATE lastDate;
    int isDelta = 0;
    __time64_t nixtime;
    struct tm *gm;

    results->get_Updates(&updateList);
    updateList->get_Count(&updateSize);

    if (updateSize > 0)
		for (LONG i = 0; i < updateSize; i++)
		{
			updateList->get_Item(i,&updateItem);
			updateItem->get_BundledUpdates(&bundledList);
			updateItem->get_SupersededUpdateIDs(&replacedIDs);
			bundledList->get_Count(&bundledSize);
			replacedIDs->get_Count(&replacedSize);
			if (verbose) {
				updateItem->get_Title(&updateName);
				updateItem->get_Description(&descString);
				updateItem->get_MaxDownloadSize(&maxDSize);
				updateItem->get_MinDownloadSize(&minDSize);
				updateItem->get_DeltaCompressedContentAvailable(&deltaAvailable);
				updateItem->get_DeltaCompressedContentPreferred(&deltaPreferred);
				if (deltaAvailable & deltaPreferred)
					isDelta = 1;
        updateItem->get_Identity(&identity);
        identity->get_UpdateID(&identityID);
				updateItem->get_DownloadPriority(&prioLevel);
				updateItem->get_IsBeta(&isBeta);
				updateItem->get_DeploymentAction(&actState);
				updateItem->get_LastDeploymentChangeTime(&lastDate);
				nixtime = (double)lastDate * 86400 - 2209161600;
				gm = _gmtime64(&nixtime);
				wprintf(L"\n%d. %s\nSize=%I64d\nDate=%sPriority=%d\nDelta=%d\nBeta=%d\nAction=%d\n",i+1,updateName, maxDSize.Lo64, _wasctime(gm), prioLevel, isDelta, isBeta, actState);
        //wprintf(L"Description=%s\n", descString);
        wcout << L"ID=" << identityID << endl;
        if (replacedSize > 0) {
          wcout << L"Superseded ID(s):" << endl;
          for (LONG i = 0; i < replacedSize; i++) {
            replacedIDs->get_Item(i,&replacedID);
            wcout << replacedID << endl;
          }
        }
        wcout << L"Download URL(s):" << endl;
			}
		
			if (bundledSize > 0)
				for (LONG i = 0; i < bundledSize; i++) {
					bundledList->get_Item(i,&bundledItem);
					bundledItem->get_DownloadContents(&downloadList);
					downloadList->get_Count(&downloadSize);
					if (downloadSize > 0)
						for (LONG i = 0; i < downloadSize; i++) {
							downloadList->get_Item(i,&downloadItem);
							downloadItem->get_DownloadUrl(&urlString);
							wcout << urlString << endl;
						}
				}
			else {
        updateItem->get_DownloadContents(&downloadList);
        downloadList->get_Count(&downloadSize);
        if (downloadSize > 0)
          for (LONG i = 0; i < downloadSize; i++) {
            downloadList->get_Item(i,&downloadItem);
            downloadItem->get_DownloadUrl(&urlString);
            wcout << urlString << endl;
          }
      }
		}
    ::CoUninitialize();
    return 0;
}