#include <Windows.h>
#include <stdio.h>

#define MYWINDOW_CLASSNAME														L"CheckingClipboardClass"

struct clipboard_manager
{
	HWND hwnd;

	HMODULE huser32;
	HWND hwndNextViewer;

	LPWSTR argv;
};

struct clipboard_manager pclipboard_manager[1];

int PrintClipboard(VOID)
{
	HGLOBAL hmem;
	LPWSTR pstr;

	if (IsClipboardFormatAvailable(CF_UNICODETEXT))
	{
		if (OpenClipboard(NULL))
		{
			hmem = GetClipboardData(CF_UNICODETEXT);
			if (hmem)
			{
				pstr = (LPWSTR)GlobalLock(hmem);
				if (pstr)
				{
					wprintf(L"%s\r\n", pstr);

					GlobalUnlock(hmem);
				}
			}

			CloseClipboard();
		}
	}

	return(0);
}
int SetTextToClipboard(LPWSTR pstr)
{
	HGLOBAL hmem;
	LPWSTR _pstr;
	UINT l;
	int flag = 0;

	if (IsClipboardFormatAvailable(CF_UNICODETEXT))
	{
		l = (wcslen(pstr) + 1) * sizeof(WCHAR);

		if (OpenClipboard(NULL))
		{
			flag = 0;

			hmem = GetClipboardData(CF_UNICODETEXT);
			if (hmem)
			{
				_pstr = (LPWSTR)GlobalLock(hmem);
				if (_pstr)
				{
					wprintf(L"%s\r\n", pstr);
					if (lstrcmpi(_pstr, pstr))
					{
						flag = 1;
					}

					GlobalUnlock(hmem);
				}
			}

			if (flag)
			{
				flag = 0;

				EmptyClipboard();

				hmem = GlobalAlloc(GMEM_MOVEABLE, l);
				if (hmem)
				{
					_pstr = (LPWSTR)GlobalLock(hmem);
					if (_pstr)
					{
						memcpy(_pstr, pstr, l);

						GlobalUnlock(hmem);

						SetClipboardData(CF_UNICODETEXT, hmem);

						flag = 1;
					}
					else
					{
						GlobalFree(hmem);
					}
				}
			}

			CloseClipboard();
		}
	}

	return(flag);
}

BOOL WindowOnCreate(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	struct clipboard_manager *pcm = pclipboard_manager;
	BOOL result = FALSE;
	typedef BOOL (WINAPI *t_AddClipboardFormatListener)(HWND);
	t_AddClipboardFormatListener pfn_AddClipboardFormatListener = NULL;
	HMODULE huser32;

	pcm->hwnd = hwnd;

	pcm->huser32 = 
	huser32 = LoadLibrary(L"User32.dll");
	if (huser32 != NULL)
	{
		pfn_AddClipboardFormatListener = (t_AddClipboardFormatListener)GetProcAddress(huser32, "AddClipboardFormatListener");
	}
	if (pfn_AddClipboardFormatListener == NULL)
	{
		pcm->hwndNextViewer = SetClipboardViewer(hwnd);
	}
	else
	{
		pfn_AddClipboardFormatListener(hwnd);
	}

	return(result);
}
BOOL WindowOnDestroy(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	struct clipboard_manager* pcm = pclipboard_manager;
	BOOL result = FALSE;
	typedef BOOL (WINAPI* t_RemoveClipboardFormatListener)(HWND);
	t_RemoveClipboardFormatListener pfn_RemoveClipboardFormatListener = NULL;
	HMODULE huser32;

	huser32 = pcm->huser32;
	if (huser32)
	{
		pfn_RemoveClipboardFormatListener = (t_RemoveClipboardFormatListener)GetProcAddress(huser32, "RemoveClipboardFormatListener");
	}
	if (pfn_RemoveClipboardFormatListener == NULL)
	{
		if (pcm->hwndNextViewer != NULL)
		{
			ChangeClipboardChain(pcm->hwnd, pcm->hwndNextViewer);
		}
	}
	else
	{
		pfn_RemoveClipboardFormatListener(hwnd);
	}
	return(result);
}
BOOL WindowOnChangeCBChain(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	struct clipboard_manager *pcm = pclipboard_manager;
	BOOL result = FALSE;

	if ((HWND)wparam == pcm->hwndNextViewer)
	{
		pcm->hwndNextViewer = (HWND)lparam;
	}
	else
	{
		// Otherwise, pass the message to the next link. 
		if (pcm->hwndNextViewer != NULL)
		{
			SendMessage(pcm->hwndNextViewer, WM_CHANGECBCHAIN, wparam, lparam);
		}
	}

	return(result);
}
BOOL WindowOnDrawClipboard(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	struct clipboard_manager *pcm = pclipboard_manager;
	int result;
	DWORD size;

	if (pcm->hwndNextViewer != NULL)
	{
		SendMessage(pcm->hwndNextViewer, WM_DRAWCLIPBOARD, wparam, lparam);
	}

	if (result = SetTextToClipboard(pcm->argv))
	{
		DestroyWindow(hwnd);
	}

	return(result);
}
BOOL WindowOnClipboardUpdate(HWND hwnd, WPARAM wparam, LPARAM lparam)
{
	struct clipboard_manager* pcm = pclipboard_manager;
	int result;

	if (result = SetTextToClipboard(pcm->argv))
	{
		DestroyWindow(hwnd);
	}

	return(result);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
	BOOL flag = FALSE;
	LRESULT result = 0;

	switch (message)
	{
	case WM_CREATE:
		flag = WindowOnCreate(hwnd, wparam, lparam);
		break;
	case WM_DESTROY:
		flag = WindowOnDestroy(hwnd, wparam, lparam);
		PostQuitMessage(0);
		break;
	case WM_CHANGECBCHAIN:
		flag = WindowOnChangeCBChain(hwnd, wparam, lparam);
		break;
	case WM_DRAWCLIPBOARD:
		flag = WindowOnDrawClipboard(hwnd, wparam, lparam);
		break;
	case WM_CLIPBOARDUPDATE:
		flag = WindowOnClipboardUpdate(hwnd, wparam, lparam);
		break;
	default:
		break;
	}
	if (!flag)
	{
		result = DefWindowProc(hwnd, message, wparam, lparam);
	}
	return(result);
}

VOID CreateMainForm(VOID)
{
	HINSTANCE hinstance;
	HWND hwnd;
	MSG msg;
	WNDCLASSEX wcex;
	LONG width = 420;
	LONG height = 290;

	hinstance = GetModuleHandle(NULL);

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hinstance;
	//wcex.hIcon = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(100));
	//wcex.hIconSm = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(100));
	wcex.hIcon = LoadIcon(NULL, MAKEINTRESOURCE(100));
	wcex.hIconSm = LoadIcon(NULL, MAKEINTRESOURCE(100));
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)COLOR_BACKGROUND;
	wcex.lpszMenuName = NULL;
	wcex.lpszClassName = MYWINDOW_CLASSNAME;

	RegisterClassEx(&wcex);

	hwnd = CreateWindow(wcex.lpszClassName, 
		NULL, 
		WS_OVERLAPPEDWINDOW, 
		(GetSystemMetrics(SM_CXSCREEN) - width) / 2, (GetSystemMetrics(SM_CYSCREEN) - height) / 2, 
		width, height, 
		NULL, 
		NULL, 
		wcex.hInstance, 
		(LPVOID)NULL);
	if (hwnd != NULL)
	{
		//pcm->hwnd = hwnd;

		UpdateWindow(hwnd);

		while (GetMessage(&msg, NULL, 0, 0))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	UnregisterClass(wcex.lpszClassName, hinstance);
}

int wmain(int argc, WCHAR *argv[])
{
	struct clipboard_manager *pcm = pclipboard_manager;

	if (argc > 1)
	{
		if (argv[1][0] == '-')
		{
			if (lstrcmpi(argv[1] + 1, L"read") == 0 && argc == 2)
			{
				PrintClipboard();
			}
			else if (lstrcmpi(argv[1] + 1, L"replace") == 0)
			{
				if (argc == 2)
				{
					HWND hwnd = FindWindow(MYWINDOW_CLASSNAME, NULL);
					if (hwnd)
					{
						PostMessage(hwnd, WM_QUIT, 0, 0);
					}
				}
				else if (argc == 3)
				{
					pcm->argv = argv[2];

					SetTextToClipboard(pcm->argv);

					CreateMainForm();
				}
			}
		}
	}

	return(0);
}