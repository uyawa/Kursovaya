// ExplorerApp.cpp : Defines the entry point for the application.
//
#define _CRT_SECURE_NO_WARNINGS
#pragma comment (lib, "comctl32") // ��� �� ���������� commctrl.h �������� ��� �64
#include "framework.h"
#include "ExplorerApp.h"
#include "Driver.h"
#include "resource.h" // ��� ������� �������������� �� ��������
#include <string>
#include <shellapi.h> // ��� �������� ������ � SHFILEOPSTRUCT
#include <fileapi.h> // ������� ����� � ������� ����    
#include <commctrl.h> // ��� ListView(�����),TreeView(����)
#include <vector>

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name
HWND hWnd;
HWND hLogsList;
HWND hWndTV;
HWND hWndLV;
std::vector<std::pair<std::wstring, HTREEITEM>> ListOfTv; // ������ ���� � ����� � ������� �� ��
bool Replace = false; // ������� �� �����������
bool Copy = false; // ������� �� �����������
std::wstring NowFolder; // ������� �������� �����
std::wstring FileNewName;
bool RenameProccesSuccess = false;
LPCWSTR CopyFrom; // ���� ������ ���������� ����
    
// Forward declarations of functions included in this code module:
ATOM                MyRegisterClass(HINSTANCE hInstance);
BOOL                InitInstance(HINSTANCE, int);
LRESULT CALLBACK    WndProc(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK    About(HWND, UINT, WPARAM, LPARAM);

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
    UNREFERENCED_PARAMETER(lpCmdLine);

    // TODO: Place code here.

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_EXPLORERAPP, szWindowClass, MAX_LOADSTRING);
    MyRegisterClass(hInstance);

    // Perform application initialization:
    if (!InitInstance (hInstance, nCmdShow))
    {
        return FALSE;
    }

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_EXPLORERAPP));

    MSG msg;

    // Main message loop:
    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    return (int) msg.wParam;
}

ATOM MyRegisterClass(HINSTANCE hInstance)
{
    WNDCLASSEXW wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style          = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = WndProc;
    wcex.cbClsExtra     = 0;
    wcex.cbWndExtra     = 0;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_EXPLORERAPP));
    wcex.hCursor        = LoadCursor(nullptr, IDC_ARROW);
    wcex.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wcex.lpszMenuName   = NULL,
    wcex.lpszClassName  = szWindowClass;
    wcex.hIconSm        = LoadIcon(wcex.hInstance, MAKEINTRESOURCE(IDI_SMALL));

    return RegisterClassExW(&wcex);
}

BOOL InitInstance(HINSTANCE hInstance, int nCmdShow)
{
   hInst = hInstance; // Store instance handle in our global variable

   hWnd = CreateWindowW(szWindowClass, szTitle, WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX,
      CW_USEDEFAULT, 0, 1000, 800, nullptr, nullptr, hInstance, nullptr);

   if (!hWnd)
   {
      return FALSE;
   }

   ShowWindow(hWnd, nCmdShow);
   UpdateWindow(hWnd);

   return TRUE;
}

// ��� �����������
LPWSTR ConvertWstringToLpwstr(std::wstring text) {
    WCHAR* buffer = new WCHAR[text.size() + 1]; // ������� ������ �� ������ ������ + 1 ������
    wcscpy_s(buffer, text.size() + 1, text.c_str()); // ����������� ������ � �����
    buffer[text.size()] = '\0'; // � ��������� ������ �������� ������������� ����
    return buffer;
}
LPWSTR ConvertByteSize(float FileSize)
{
    if (FileSize == 0)
        return ConvertWstringToLpwstr(L""); // ���� ������ 0 - ������� ������ ������
    int count = 0;
    while (FileSize >= 1024) // ���� ����� ������ ��� ����� 1024
    {
        FileSize /= 1024; // ������ �� 1024
        count++; // ������� ������� ��� ��������
    }
    std::wstring StringSizeFull = std::to_wstring(FileSize);
    // �������� ������ ����� ����� �������
    std::wstring StringSize = std::wstring(StringSizeFull.begin(), std::find(StringSizeFull.begin(), StringSizeFull.end(), L'.') + 2);

    StringSize += std::wstring(18 - (StringSize.size() * 2), L' '); // �������� ������� ��� �� �� ���� ������� � �������
    // ������ ����� - 2 �������, � �� ����, ��� �� �� ������ ��������� �����
    if (count == 0)
        StringSize += std::wstring(L"��"); // �����
    else if (count == 1)
        StringSize += std::wstring(L"��"); // ���������
    else if (count == 2)
        StringSize += std::wstring(L"��"); // ���������
    else if (count == 3)
        StringSize += std::wstring(L"��"); // ���������

    return ConvertWstringToLpwstr(StringSize);
}
LPWSTR DataModifiedToString(FILETIME DataModified) {
    SYSTEMTIME LocalTime, UTCTime;
    FileTimeToSystemTime(&DataModified, &UTCTime); // �������������� � UTC
    SystemTimeToTzSpecificLocalTime(NULL, &UTCTime, &LocalTime); // �������������� � ��������� �����

    std::wstring WstringTime;
    // �������� ���, ���� ����� ������ 10, �������� � ������ 0
    if (LocalTime.wDay < 10)
        WstringTime += std::to_wstring(0);
    WstringTime += std::to_wstring(LocalTime.wDay) + L".";

    // �������� ������, ���� ����� ������ 10, �������� � ������ 0
    if (LocalTime.wMonth < 10)
        WstringTime += std::to_wstring(0);
    WstringTime += std::to_wstring(LocalTime.wMonth) + L".";

    // �������� ����
    WstringTime += std::to_wstring(LocalTime.wYear) + L" ";

    // �������� ����, ���� ����� ������ 10, �������� � ������ 0
    if (LocalTime.wHour < 10)
        WstringTime += std::to_wstring(0);
    WstringTime += std::to_wstring(LocalTime.wHour) + L":";

    // �������� ������, ���� ����� ������ 10, �������� � ������ 0
    if (LocalTime.wMinute < 10)
        WstringTime += std::to_wstring(0);
    WstringTime += std::to_wstring(LocalTime.wMinute);

    return ConvertWstringToLpwstr(WstringTime);
}
std::wstring GetFolderPath(std::wstring FullPath) {
    for (int i = FullPath.size() - 1; i >= 0; i--)
    {
        if (FullPath[i] == '\\')
        {
            break;
        }
        FullPath.pop_back();
    }
    return FullPath;
}
// ��������� �����
void LoadDriver() {
    TVINSERTSTRUCT tvInsert;
    // ��������� ��� ���������
    tvInsert.hParent = nullptr; // �������� � ������
    tvInsert.hInsertAfter = TVI_ROOT;  // �������� � ������
    tvInsert.item.mask = TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_PARAM; // ����� ���� ���������
    tvInsert.item.pszText = ConvertWstringToLpwstr(L"̳� ����'����"); // �����
    tvInsert.item.lParam = (LPARAM)ConvertWstringToLpwstr(L"MyComputer"); // ������ - MyComputer
    HTREEITEM MyComputer = TreeView_InsertItem(hWndTV, &tvInsert); // �������� � ��

    // ��������� � ��������� �������� � ��
    Driver driver;
    for (int i = 0; i < driver.DriverCount; i++)
    {   
        tvInsert.hParent = MyComputer; // �������� - ��� ���������
        tvInsert.item.pszText = ConvertWstringToLpwstr(driver.Name[i]); // ��� - ��� � �����
        tvInsert.item.lParam = (LPARAM)ConvertWstringToLpwstr(driver.Name[i]); // ������ ����� �� ��� ��� �����
        HTREEITEM NowItemTree = TreeView_InsertItem(hWndTV, &tvInsert); // �������� ������� � ��
        ListOfTv.push_back(std::make_pair(driver.Name[i], NowItemTree));
    }
}

// �������� ������� � ��
void AddColumsLV() {
    std::wstring text = L"��'� �����";

    LVCOLUMN LVCOLUMN1; // ������� ��������� �������
    LVCOLUMN1.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; // ����� ���� ��������� ���������
    LVCOLUMN1.fmt = LVCFMT_LEFT; // ������������ ������
    LVCOLUMN1.cx = 200; // ������ �������
    LVCOLUMN1.pszText = ConvertWstringToLpwstr(text); // ����� �������
    ListView_InsertColumn(hWndLV, 0, &LVCOLUMN1); // �������� �������

    text = L"���� �������� ����";
    LVCOLUMN LVCOLUMN2;
    LVCOLUMN2.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; // ����� ���� ��������� ���������
    LVCOLUMN2.fmt = LVCFMT_LEFT; // ������������ ������
    LVCOLUMN2.cx = 200; // ������ �������
    LVCOLUMN2.pszText = ConvertWstringToLpwstr(text); // ����� �������
    ListView_InsertColumn(hWndLV, 1, &LVCOLUMN2); // �������� �������

    text = L"���";
    LVCOLUMN LVCOLUMN3;
    LVCOLUMN3.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; // ����� ���� ��������� ���������
    LVCOLUMN3.fmt = LVCFMT_LEFT; // ������������ ������
    LVCOLUMN3.cx = 100; // ������ �������
    LVCOLUMN3.pszText = ConvertWstringToLpwstr(text); // ����� �������
    ListView_InsertColumn(hWndLV, 2, &LVCOLUMN3); // �������� �������

    text = L"�����";
    LVCOLUMN LVCOLUMN4;
    LVCOLUMN4.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; // ����� ���� ��������� ���������
    LVCOLUMN4.fmt = LVCFMT_LEFT; // ������������ ������
    LVCOLUMN4.cx = 100; // ������ �������
    LVCOLUMN4.pszText = ConvertWstringToLpwstr(text); // ����� �������
    ListView_InsertColumn(hWndLV, 3, &LVCOLUMN4); // �������� �������
}

// ����� ���� � ������� �� ��
LPCWSTR getPathForListView(int iItem)
{
    LVITEM lv;
    lv.mask = LVIF_PARAM;
    lv.iItem = iItem;
    lv.iSubItem = 0;
    ListView_GetItem(hWndLV, &lv);
    return (LPCWSTR)lv.lParam;
}

// ����� ���� � ������� �� ��
LPCWSTR getPathForTreeView(HTREEITEM hItem)
{
    TVITEMEX tv;
    tv.mask = TVIF_PARAM;
    tv.hItem = hItem;
    TreeView_GetItem(hWndTV, &tv);
    return (LPCWSTR)tv.lParam;
}

// ����� �������� ����� � ��������� �����
HTREEITEM FindTVItem(std::wstring path) {
    auto it = find_if(ListOfTv.begin(), ListOfTv.end(), // ������� �������� �� ������ �������
        [&path](const std::pair<std::wstring, HTREEITEM>& element) { return element.first == path; }); // ������ ������� ������
    return it == ListOfTv.end() ? nullptr : it->second; // ���������� HTREEITEM �� ���������� ���������
}

bool MyComparator(decltype(ListOfTv[0]) a, std::wstring path) {
    return a.first.find(path) != -1; // ���� �� ����� ��������� - �� �������
}

void DeletePathFromTVlist(std::wstring path) {
    // �������� ����� �� ������� �������
    for (int i = 0; i < ListOfTv.size(); i++)
    {
        if (MyComparator(ListOfTv[i],path))
        {
            ListOfTv.erase(ListOfTv.begin() + i); // ������� �������
        }
    }
}
// ����������� HTREEITEM � TVITEM
TVITEM getItemFromHTREE(HTREEITEM hItem)
{
    TVITEM tv; // ��� ����
    tv.mask = TVIF_PARAM; // ��� ���������
    tv.hItem = hItem; 
    TreeView_GetItem(hWndTV, &tv); // ������� �� �����������
    return tv; 
}

// ��������� ������ � ��
void LoadPathInTV(std::wstring ClickedPath, HTREEITEM parent, bool FirstTime = true) {
    std::wstring folder = ClickedPath;
    if (folder.size() == 3)
        folder += L"*"; // ���� ������ ���� - ��������� ������ ���������
    else
        folder += L"\\*"; // ���� ������ ����� - ��������� ��� � \\

    WIN32_FIND_DATA fd; // ��������� �������� ���������� � ��������� �����
    HANDLE hFile = FindFirstFileW(ConvertWstringToLpwstr(folder), &fd); // ������� ������ ����
    if (hFile == INVALID_HANDLE_VALUE) // ���� ���������� ������ - ����� �� �������
        return;

    // ���� ���� �� ������ ��� ����� � ����������
    do
    {
        if (!(wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L".."))) // ���������� . � ..
            continue;
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) // ���� �� ���������� - ����������
            continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) // ���������� ��������� �����
            continue;

        std::wstring file_path = ClickedPath;
        if (file_path.size() != 3)
            file_path += L"\\"; // ��������� // � ���� �����
        file_path += fd.cFileName; // ��������� ��� ����� � ���� �����

        TVINSERTSTRUCT TVInsert; // ������� ������� ������� � ��
        TVInsert.hParent = parent; // �������� ����� �������� ( ���������� ����� )
        TVInsert.hInsertAfter = TVI_SORT;
        TVInsert.item.mask = TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_PARAM; // ����� ���� ���������
        TVInsert.item.pszText = fd.cFileName; // ����� �� ��������
        TVInsert.item.lParam = (LPARAM)ConvertWstringToLpwstr(file_path); // ������ - ������ ���� � �����
        HTREEITEM NowItemTree;

        if (FindTVItem(file_path) == nullptr) // ���� ����� ����� ��� ���� � �� - ���������
        {
            NowItemTree = TreeView_InsertItem(hWndTV, &TVInsert); // ��������� �������� ����� ��� ����
            ListOfTv.push_back(std::make_pair(file_path, NowItemTree)); // ������ ��� �������� �� � �� ���� � �������
        }
        else
            NowItemTree = FindTVItem(file_path); // ������� �������� ��������
        if (FirstTime) // ��� ������ ������������ ������
            LoadPathInTV(ConvertWstringToLpwstr(file_path), NowItemTree, false); // ��������� ���������� ����� ������ �����

    } while (FindNextFileW(hFile, &fd));
}

// ��������� ����� � �� ��� ���
void RefreshTVPath(std::wstring path) {
    std::wstring FolderPath = GetFolderPath(path); // ������� �������� ����� �� ����
    if (FolderPath.size() != 3) // ���� � �����, � �� �� �����
    {
        FolderPath = std::wstring(FolderPath.begin(), FolderPath.end() - 1); // �������� \\ � �����
    }
    LoadPathInTV(FolderPath, FindTVItem(FolderPath)); // ��������� ���� � �� ��� ���
}

// ��������� ������ � ��
void LoadPathDirInLV(std::wstring ClickedPath, bool FormatString = false)
{
    std::wstring folder = ClickedPath;
    if (folder.size() == 3)
        folder += L"*"; // ���� ������ ���� - ��������� ������ ���������
    else if (FormatString)
        folder += L"*"; // ���� ������ �������� �� ������ �������, � �� ����� ������������ � ���� ��� �����
    else
        folder += L"\\*"; // ���� ������ ����� - ��������� ��� � \\

    WIN32_FIND_DATA fd; // ��������� �������� ���������� � ��������� �����
    HANDLE hFile = FindFirstFileW(ConvertWstringToLpwstr(folder), &fd); // ������� ������ ����
    if (hFile == INVALID_HANDLE_VALUE) // ���� ���������� ������ - ����� �� �������
        return;

    int ItemCounter = 0; // ������� ����� ������
    // ���� ���� �� ������ ��� ����� � ����������
    do
    {
        std::wstring FilePath = ClickedPath;
        if (FilePath.size() != 3 && !FormatString)
            FilePath += L"\\"; // ��������� \\ � ���� �����
        NowFolder = FilePath; // ��������� ������� ����

        if (!(wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L".."))) // ���������� . � ..
            continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) // ���������� ��������� �����
            continue;

        FilePath += fd.cFileName; // ��������� ��� ����� � ���� �����

        LV_ITEM LVInsert; // ������� ������� ������� � ��
        LVInsert.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE; // ����� ���� ���������
        LVInsert.iItem = ItemCounter; // ����� �������� � �����
        LVInsert.iSubItem = 0; // ������� ������ 0, ��� �� �������� �������
        LVInsert.pszText = fd.cFileName; // ����� �� ��������
        LVInsert.lParam = (LPARAM)ConvertWstringToLpwstr(FilePath); // ������ - ������ ���� � ����
        ListView_InsertItem(hWndLV, &LVInsert);

        // ��������� �������
        LVITEM LVItemColum;
        LVItemColum.mask = LVIF_TEXT; // ��� ��������� � �������
        LVItemColum.iItem = ItemCounter; // ��� ������ �������� � ����� ����� ���������
        // ������ �������
        LVItemColum.pszText = DataModifiedToString(fd.ftLastWriteTime); // ����� � �������
        LVItemColum.iSubItem = 1;  // ����� �������
        ListView_SetItem(hWndLV, &LVItemColum); // ��������� �������

        // ������ �������
        fd.dwFileAttributes& FILE_ATTRIBUTE_DIRECTORY ? // ��������� ��������
            LVItemColum.pszText = ConvertWstringToLpwstr(L"�����") : LVItemColum.pszText = ConvertWstringToLpwstr(L"����");
        LVItemColum.iSubItem = 2; // ����� �������
        ListView_SetItem(hWndLV, &LVItemColum); // ��������� �������

        // ������ �������
        LVItemColum.pszText = ConvertByteSize(fd.nFileSizeLow); // ����� � �������
        LVItemColum.iSubItem = 3; // ����� �������
        ListView_SetItem(hWndLV, &LVItemColum); // ��������� �������
        ++ItemCounter; // ����������� ���������� ��������� � �����

    } while (FindNextFileW(hFile, &fd));

}

void OpenOrAddInLv(std::wstring path)
{
    WIN32_FIND_DATA fd;
    // ��������� �������� ����� �� ���� � ��������� �����
    if (GetFileAttributesEx(ConvertWstringToLpwstr(path), GetFileExInfoStandard, &fd) != 0)
    {
        // ��������� ����� �� ���
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // ���� ����� - ������� ��� �� �� � ��������� ��� �����
            ListView_DeleteAllItems(hWndLV);
            LoadPathDirInLV(ConvertWstringToLpwstr(path));
        }
        else
        {
            // ���� ���� - ������� ����� ����
            ShellExecute(NULL, L"open", ConvertWstringToLpwstr(path), NULL, NULL, SW_SHOWNORMAL);
        }
    }
}
// ��������� �������
INT_PTR RenameFileProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    
    switch (message)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK: {
            HWND hEdit = GetDlgItem(hDlg, 10); // ����� HWND �����
            WCHAR FileName[100]; // ����� ��� ������
            GetWindowText(hEdit, FileName, 99); // ������� ����� � ������ � ����
            FileNewName = FileName; // �������� ����� � ���������� ����������  
            RenameProccesSuccess = true; // �������� ������ �������
            EndDialog(hDlg, 0);
            break;
        }
        case IDCANCEL: {
            RenameProccesSuccess = false;
            EndDialog(hDlg, 0);
            break;
        }

        }
    }
    return FALSE;
}


LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CREATE: {
        //������������ �������� ���������� DLL ������ ���������� (Comctl32.dll) 
        INITCOMMONCONTROLSEX ICEX;
        ICEX.dwSize = sizeof(INITCOMMONCONTROLSEX);
        ICEX.dwICC = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES; // ��� ������������
        // ������������ ������������ ������ ������ ���������� �� ���������� DLL. 
        InitCommonControlsEx(&ICEX);

        const int TV_x = 0, TV_y = 0, TV_width = 400, TV_height = 500; // ��������� ��� ��
        const int LV_x = 400, LV_y = 0, LV_width = 600, LV_height = 500; // ��������� ��� ��
        hWndLV = CreateWindow(WC_LISTVIEWW, L"Explorer", WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | WS_HSCROLL | WS_VSCROLL,
            LV_x, LV_y, LV_width, LV_height, hWnd, (HMENU)0, hInst, nullptr); // ������� ��
        hWndTV = CreateWindow(WC_TREEVIEW, L"TREE VIEW", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, 
            TV_x, TV_y, TV_width, TV_height, hWnd, (HMENU)1, hInst, nullptr); // ������� ��
        hLogsList = CreateWindow(L"listbox", L"Logs",  WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY, // ������� ������ listbox
            0, TV_y + TV_height, TV_width + LV_width - 50, 260, hWnd, HMENU(4), NULL, NULL);
        LoadDriver();
        AddColumsLV();
        break;
    }
    case WM_NOTIFY: {
        NMHDR* notifyMess = (NMHDR*)lParam; // ����������� ���� ��������� �� �������
        switch (notifyMess->code)
        {
        case NM_DBLCLK: {
            // ���� ���� �����
            if (notifyMess->hwndFrom == hWndLV)
                // ���� ��������� ������ �� �� -> ��������� ��������� �����\����
            {
                LPCWSTR path = getPathForListView(ListView_GetSelectionMark(hWndLV));
                if (path != nullptr) {
                    OpenOrAddInLv(path);
                }
            }
            else if(notifyMess->hwndFrom == hWndTV)
                // ���� ��������� ������ �� �� -> ��������� ����� � ��
            {
                ListView_DeleteAllItems(hWndLV);
                LPCWSTR path = getPathForTreeView(TreeView_GetNextItem(hWndTV, NULL, TVGN_CARET));
                if (path != nullptr)
                    LoadPathDirInLV(path);
            }
            break;
        }
        case NM_RCLICK: {
            // ���� ������
            if (notifyMess->hwndFrom == hWndLV)
            {
                POINT cursor; 
                GetCursorPos(&cursor); // ����� ������� �������
                // ������� ����������� ����
                TrackPopupMenu((HMENU)GetSubMenu(LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU1)), 0), TPM_LEFTALIGN | TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hWnd, NULL);
            }
            break;
        }
        case TVN_ITEMEXPANDING: {
            // ���������� ��� �������� ����� � ������
            NMTREEVIEW* data = (NMTREEVIEW*)lParam; // ����������� ���� ��������� �� �������
            if (data->action == 2)
                // ���� �������������
            {
                LPCWSTR path = getPathForTreeView(data->itemNew.hItem);
                if (path != nullptr) {
                    LoadPathInTV(path, data->itemNew.hItem);
                }
            }
            break;
        }
        case TVN_SELCHANGED: {
            // ������ ��� �� ������ ����� � ������
            ListView_DeleteAllItems(hWndLV);
            LPCWSTR path = getPathForTreeView(TreeView_GetNextItem(hWndTV, NULL, TVGN_CARET));
            if (path != nullptr) {
                LoadPathDirInLV(path);
                LoadPathInTV(path, TreeView_GetNextItem(hWndTV, NULL, TVGN_CARET));
            }
            break;
        }       
        default: {
            break;
        }
        }
        break;
    }
    case WM_COMMAND:
        // ��������� ������� ����������� ����
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case 1: case 2: // ���������� - 1 ��� �������� - 2
            { 
                int index = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                if (index == -1) // ���� ���� �� ������
                    break;
                Replace = wmId == 1 ? false : true; // ���� ����������� - false, ���� ����������� - true
                Copy = wmId == 1 ? true : false; // ���� ����������� - true, ���� ����������� - false
                CopyFrom = getPathForListView(index); // ���� ����� �����������
                break;
            }
            case 3: // ��������
            { 
                if (!Copy && !Replace) // ���� �������� �� ������� - �����
                    break;
                WCHAR* CopyTo = new WCHAR[NowFolder.size() + 2];
                wcscpy(CopyTo, NowFolder.c_str());
                CopyTo[NowFolder.size()] = L'\0';
                CopyTo[NowFolder.size() + 1] = L'\0';

                std::wstring CopyFromWstring = CopyFrom;
                WCHAR* CopyFromComplete = new WCHAR[CopyFromWstring.size() + 3];
                wcscpy(CopyFromComplete, CopyFromWstring.c_str());
                CopyFromComplete[CopyFromWstring.size()] = L'\\';
                CopyFromComplete[CopyFromWstring.size() + 1] = L'\0';
                CopyFromComplete[CopyFromWstring.size() + 2] = L'\0';

                SHFILEOPSTRUCTW OperationInfo = { 0 }; // ��������� �������� ��������
                OperationInfo.hwnd = hWnd;
                OperationInfo.pFrom = CopyFromComplete;
                OperationInfo.pTo = CopyTo;
                OperationInfo.wFunc = Replace ? FO_MOVE : FO_COPY;
                OperationInfo.fFlags = FOF_RENAMEONCOLLISION;
                SHFileOperationW(&OperationInfo); // ���������� �������� ��������
                // ���������� ��
                ListView_DeleteAllItems(hWndLV);
                LoadPathDirInLV(GetFolderPath(CopyTo), true);
                // ���������� ��
                if (Replace)
                {
                    HTREEITEM DeletItem = FindTVItem(CopyFromWstring);
                    if (DeletItem != nullptr)
                    {
                        DeletePathFromTVlist(CopyFromWstring);
                        TreeView_DeleteItem(hWndTV, DeletItem);
                    }
                }
                RefreshTVPath(CopyFromWstring + std::wstring(L"\\"));
                RefreshTVPath(NowFolder);

                std::wstring log = L"���� ��������� � ";
                log += CopyFromComplete;
                log += L" � ";
                log += CopyTo;
                SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                break;
            }
            case 4: // �������
            {
                int index = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                if (index == -1) // ���� ���� �� ������
                    break;

                std::wstring ErasePath = getPathForListView(index);
                WCHAR* ErasePathComplete = new WCHAR[ErasePath.size() + 3];
                wcscpy(ErasePathComplete, ErasePath.c_str());
                ErasePathComplete[ErasePath.size() ] = L'\\';
                ErasePathComplete[ErasePath.size() + 1] = L'\0';
                ErasePathComplete[ErasePath.size() + 2] = L'\0';
                SHFILEOPSTRUCTW OperationInfo = { 0 }; // ��������� �������� ��������
                OperationInfo.hwnd = hWnd;
                OperationInfo.pFrom = ErasePathComplete;
                OperationInfo.wFunc = FO_DELETE;
                OperationInfo.fFlags = FOF_ALLOWUNDO; // ���� ������� � �������, � �� ����� �������� 
                SHFileOperationW(&OperationInfo); // ���������� �������� ��������
                DWORD error = GetLastError();
                if (error != 0)
                {
                    std::wstring log = L"�� ������� �������� �����, ������ ����� ������������";
                    SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                    break;
                }
                // �������� �� �� 
                HTREEITEM DeletItem = FindTVItem(ErasePath);
                if (DeletItem != nullptr)
                {
                    DeletePathFromTVlist(ErasePath);
                    TreeView_DeleteItem(hWndTV, DeletItem);
                }
                // ���������� ��
                ListView_DeleteAllItems(hWndLV);
                LoadPathDirInLV(GetFolderPath(ErasePath), true);

                std::wstring log = L"���� ��������� - ";
                log += ErasePathComplete;
                SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                break;
            }
            case 5: { // �������������
                int index = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                if (index == -1) // ���� ���� �� ������
                    break;
                std::wstring RenamePath = std::wstring(getPathForListView(index));
                WCHAR* RenamePathComplete = new WCHAR[RenamePath.size() + 3];
                wcscpy(RenamePathComplete, RenamePath.c_str());
                RenamePathComplete[RenamePath.size()] = L'\\';
                RenamePathComplete[RenamePath.size() + 1] = L'\0';
                RenamePathComplete[RenamePath.size() + 2] = L'\0';

                std::wstring NewNameWstring = GetFolderPath(std::wstring(RenamePath.begin(), RenamePath.end() - 1));
                DialogBox(hInst, MAKEINTRESOURCE(DialogRenameFileProc), hWnd, RenameFileProc);
                if (!RenameProccesSuccess) // ���� ������ ������ �������� � �������
                    break;
                    
                NewNameWstring += FileNewName;
                WCHAR* NewName = new WCHAR[NewNameWstring.size() + 3];
                wcscpy(NewName, NewNameWstring.c_str());
                NewName[NewNameWstring.size()] = L'\\';
                NewName[NewNameWstring.size() + 1] = L'\0';
                NewName[NewNameWstring.size() + 2] = L'\0';

                SHFILEOPSTRUCTW OperationInfo = { 0 }; // ��������� �������� ��������
                OperationInfo.hwnd = hWnd;
                OperationInfo.pFrom = RenamePathComplete;
                OperationInfo.pTo = NewName;
                OperationInfo.wFunc = FO_RENAME;
                OperationInfo.fFlags = FOF_RENAMEONCOLLISION | FOF_ALLOWUNDO;
                // ���������� �������� ��������
                SHFileOperationW(&OperationInfo);
                // ���������� ��
                ListView_DeleteAllItems(hWndLV);
                LoadPathDirInLV(GetFolderPath(RenamePath), true);
                // ���������� ��
                HTREEITEM DeletItem = FindTVItem(RenamePath); 
                if (DeletItem != nullptr)
                {
                    DeletePathFromTVlist(RenamePath); // ������� ������ �����, ���� ����� �� ������ ������
                    TreeView_DeleteItem(hWndTV, DeletItem); // ������� ������ �����, ���� ����� �� ��
                }
                RefreshTVPath(RenamePath + std::wstring(L"\\"));
                RefreshTVPath(NowFolder);


                std::wstring log = L"���� �������������� � ";
                log += RenamePathComplete;
                log += L" � ";
                log += NewName;
                SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                break;
            }
            case 6: // ������� ����� 
            {
                std::wstring NewFolderPath = NowFolder + std::wstring(L"���� �����");
                CreateDirectory(ConvertWstringToLpwstr(NewFolderPath), NULL);
                DWORD error = GetLastError();
                if (error != 0)
                {
                    std::wstring log = L"����� �� ��������, �������� ������������ ������ �������� �����";
                    SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                    break;
                }
                // ���������� ��
                ListView_DeleteAllItems(hWndLV);
                LoadPathDirInLV(GetFolderPath(NowFolder), true); 
                // ���������� ��
                RefreshTVPath(NewFolderPath);
                std::wstring log = L"�������� ����� ";
                log += NewFolderPath;
                SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                break;
            }
            case 7: // ������� ����
            {
                std::wstring NewFilePath = NowFolder + std::wstring(L"����� ����.txt");
                // ������� ����� ����, ���� ���� ����� � ����� �� ��������� � ��������
                HANDLE file = CreateFile( NewFilePath.c_str(),NULL, FILE_SHARE_DELETE,NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,  NULL);
                DWORD error = GetLastError();
                if (error != 0)
                {
                    std::wstring log = L"���� �� �������, �������� ������������ ��������� ��������� ����";
                    SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                    break;
                }
                CloseHandle(file);
                // ���������� ��
                ListView_DeleteAllItems(hWndLV);
                LoadPathDirInLV(GetFolderPath(NowFolder), true);
                std::wstring log = L"������� ���� ";
                log += NewFilePath;
                SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                break;
            }
            default:
                return DefWindowProc(hWnd, message, wParam, lParam);
            }
        }
        break;
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hWnd, &ps);
        // TODO: Add any drawing code that uses hdc here...
        EndPaint(hWnd, &ps);
        break;
    }
    case WM_SIZING:
    {
        // ��� �� ������ ���� �������� ������ ����
        RECT rc;
        GetWindowRect(hWnd, &rc);
        *((LPRECT)lParam) = rc;
        break;
    }
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}
