// ExplorerApp.cpp : Defines the entry point for the application.
//
#define _CRT_SECURE_NO_WARNINGS
#pragma comment (lib, "comctl32") // что бы библиотека commctrl.h работала для х64
#include "framework.h"
#include "ExplorerApp.h"
#include "Driver.h"
#include "resource.h" // для диалога переименования из ресурсов
#include <string>
#include <shellapi.h> // для открытия файлов и SHFILEOPSTRUCT
#include <fileapi.h> // создать папку и создать файл    
#include <commctrl.h> // для ListView(право),TreeView(лево)
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
std::vector<std::pair<std::wstring, HTREEITEM>> ListOfTv; // хранит путь к файлу и элемент из ТВ
bool Replace = false; // активно ли Перемещение
bool Copy = false; // активно ли Копирование
std::wstring NowFolder; // текущая открытая папка
std::wstring FileNewName;
bool RenameProccesSuccess = false;
LPCWSTR CopyFrom; // путь откуда копировать файл
    
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

// Мои конвертации
LPWSTR ConvertWstringToLpwstr(std::wstring text) {
    WCHAR* buffer = new WCHAR[text.size() + 1]; // создать буффер на размер текста + 1 символ
    wcscpy_s(buffer, text.size() + 1, text.c_str()); // скопировать строку в буфер
    buffer[text.size()] = '\0'; // в последний символ вставить терминирующий ноль
    return buffer;
}
LPWSTR ConvertByteSize(float FileSize)
{
    if (FileSize == 0)
        return ConvertWstringToLpwstr(L""); // если размер 0 - вернуть пустую строку
    int count = 0;
    while (FileSize >= 1024) // пока число больше или равно 1024
    {
        FileSize /= 1024; // делить на 1024
        count++; // считает сколько раз поделили
    }
    std::wstring StringSizeFull = std::to_wstring(FileSize);
    // обрезать лишние знаки после запятой
    std::wstring StringSize = std::wstring(StringSizeFull.begin(), std::find(StringSizeFull.begin(), StringSizeFull.end(), L'.') + 2);

    StringSize += std::wstring(18 - (StringSize.size() * 2), L' '); // добавить пробелы что бы не было разницы в размере
    // каждая цифра - 2 символа, а не один, что бы на экране выглядело ровно
    if (count == 0)
        StringSize += std::wstring(L"БТ"); // байты
    else if (count == 1)
        StringSize += std::wstring(L"КБ"); // килобайты
    else if (count == 2)
        StringSize += std::wstring(L"МБ"); // мегабайты
    else if (count == 3)
        StringSize += std::wstring(L"ГБ"); // гигабайты

    return ConvertWstringToLpwstr(StringSize);
}
LPWSTR DataModifiedToString(FILETIME DataModified) {
    SYSTEMTIME LocalTime, UTCTime;
    FileTimeToSystemTime(&DataModified, &UTCTime); // конвертировать в UTC
    SystemTimeToTzSpecificLocalTime(NULL, &UTCTime, &LocalTime); // конвертировать в системное время

    std::wstring WstringTime;
    // добавить дни, если число меньше 10, добавить в начало 0
    if (LocalTime.wDay < 10)
        WstringTime += std::to_wstring(0);
    WstringTime += std::to_wstring(LocalTime.wDay) + L".";

    // добавить месяцы, если число меньше 10, добавить в начало 0
    if (LocalTime.wMonth < 10)
        WstringTime += std::to_wstring(0);
    WstringTime += std::to_wstring(LocalTime.wMonth) + L".";

    // добавить годы
    WstringTime += std::to_wstring(LocalTime.wYear) + L" ";

    // добавить часы, если число меньше 10, добавить в начало 0
    if (LocalTime.wHour < 10)
        WstringTime += std::to_wstring(0);
    WstringTime += std::to_wstring(LocalTime.wHour) + L":";

    // добавить минуты, если число меньше 10, добавить в начало 0
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
// Загрузить диски
void LoadDriver() {
    TVINSERTSTRUCT tvInsert;
    // добавляем Мой компьютер
    tvInsert.hParent = nullptr; // вставить в корень
    tvInsert.hInsertAfter = TVI_ROOT;  // вставить в корень
    tvInsert.item.mask = TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_PARAM; // какие поля заполнены
    tvInsert.item.pszText = ConvertWstringToLpwstr(L"Мій комп'ютер"); // текст
    tvInsert.item.lParam = (LPARAM)ConvertWstringToLpwstr(L"MyComputer"); // лпарам - MyComputer
    HTREEITEM MyComputer = TreeView_InsertItem(hWndTV, &tvInsert); // вставить в ТВ

    // загружаем и добавляет драйверы в ТВ
    Driver driver;
    for (int i = 0; i < driver.DriverCount; i++)
    {   
        tvInsert.hParent = MyComputer; // родитель - мой компьютер
        tvInsert.item.pszText = ConvertWstringToLpwstr(driver.Name[i]); // имя - как у диска
        tvInsert.item.lParam = (LPARAM)ConvertWstringToLpwstr(driver.Name[i]); // лпарам такой же как имя диска
        HTREEITEM NowItemTree = TreeView_InsertItem(hWndTV, &tvInsert); // вставить элемент в ТВ
        ListOfTv.push_back(std::make_pair(driver.Name[i], NowItemTree));
    }
}

// Добавить колонки в ЛВ
void AddColumsLV() {
    std::wstring text = L"Ім'я файлу";

    LVCOLUMN LVCOLUMN1; // создаем структуру колонки
    LVCOLUMN1.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; // какие поля структуры заполнены
    LVCOLUMN1.fmt = LVCFMT_LEFT; // расположение текста
    LVCOLUMN1.cx = 200; // ширина колонки
    LVCOLUMN1.pszText = ConvertWstringToLpwstr(text); // текст колонки
    ListView_InsertColumn(hWndLV, 0, &LVCOLUMN1); // вставить колонку

    text = L"Дата останньої зміни";
    LVCOLUMN LVCOLUMN2;
    LVCOLUMN2.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; // какие поля структуры заполнены
    LVCOLUMN2.fmt = LVCFMT_LEFT; // расположение текста
    LVCOLUMN2.cx = 200; // ширина колонки
    LVCOLUMN2.pszText = ConvertWstringToLpwstr(text); // текст колонки
    ListView_InsertColumn(hWndLV, 1, &LVCOLUMN2); // вставить колонку

    text = L"Тип";
    LVCOLUMN LVCOLUMN3;
    LVCOLUMN3.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; // какие поля структуры заполнены
    LVCOLUMN3.fmt = LVCFMT_LEFT; // расположение текста
    LVCOLUMN3.cx = 100; // ширина колонки
    LVCOLUMN3.pszText = ConvertWstringToLpwstr(text); // текст колонки
    ListView_InsertColumn(hWndLV, 2, &LVCOLUMN3); // вставить колонку

    text = L"Розмір";
    LVCOLUMN LVCOLUMN4;
    LVCOLUMN4.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT; // какие поля структуры заполнены
    LVCOLUMN4.fmt = LVCFMT_LEFT; // расположение текста
    LVCOLUMN4.cx = 100; // ширина колонки
    LVCOLUMN4.pszText = ConvertWstringToLpwstr(text); // текст колонки
    ListView_InsertColumn(hWndLV, 3, &LVCOLUMN4); // вставить колонку
}

// Взять путь к объекту из ЛВ
LPCWSTR getPathForListView(int iItem)
{
    LVITEM lv;
    lv.mask = LVIF_PARAM;
    lv.iItem = iItem;
    lv.iSubItem = 0;
    ListView_GetItem(hWndLV, &lv);
    return (LPCWSTR)lv.lParam;
}

// Взять путь к объекту из ТВ
LPCWSTR getPathForTreeView(HTREEITEM hItem)
{
    TVITEMEX tv;
    tv.mask = TVIF_PARAM;
    tv.hItem = hItem;
    TreeView_GetItem(hWndTV, &tv);
    return (LPCWSTR)tv.lParam;
}

// Найти дочернию папку с указанным путем
HTREEITEM FindTVItem(std::wstring path) {
    auto it = find_if(ListOfTv.begin(), ListOfTv.end(), // находим итератор на нужный элемент
        [&path](const std::pair<std::wstring, HTREEITEM>& element) { return element.first == path; }); // лямбда функция поиска
    return it == ListOfTv.end() ? nullptr : it->second; // возвращаем HTREEITEM из найденного итератора
}

bool MyComparator(decltype(ListOfTv[0]) a, std::wstring path) {
    return a.first.find(path) != -1; // если не нашел подстроку - не удалять
}

void DeletePathFromTVlist(std::wstring path) {
    // проверка нужно ли удалить элемент
    for (int i = 0; i < ListOfTv.size(); i++)
    {
        if (MyComparator(ListOfTv[i],path))
        {
            ListOfTv.erase(ListOfTv.begin() + i); // удаляет элемент
        }
    }
}
// конвертация HTREEITEM в TVITEM
TVITEM getItemFromHTREE(HTREEITEM hItem)
{
    TVITEM tv; // что ищем
    tv.mask = TVIF_PARAM; // что заполнено
    tv.hItem = hItem; 
    TreeView_GetItem(hWndTV, &tv); // находим по заполненому
    return tv; 
}

// Загурзить данные в ТВ
void LoadPathInTV(std::wstring ClickedPath, HTREEITEM parent, bool FirstTime = true) {
    std::wstring folder = ClickedPath;
    if (folder.size() == 3)
        folder += L"*"; // если внутри тома - добавляем только звездочку
    else
        folder += L"\\*"; // если внутри папки - добавляем еще и \\

    WIN32_FIND_DATA fd; // структура хранящая информацию о найденном файле
    HANDLE hFile = FindFirstFileW(ConvertWstringToLpwstr(folder), &fd); // находим первый файл
    if (hFile == INVALID_HANDLE_VALUE) // если директория пустая - выход из функции
        return;

    // цикл пока не найдем все файлы в директории
    do
    {
        if (!(wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L".."))) // пропустить . и ..
            continue;
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) // если не директория - пропустить
            continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) // пропускать системные файлы
            continue;

        std::wstring file_path = ClickedPath;
        if (file_path.size() != 3)
            file_path += L"\\"; // добавляем // в путь файла
        file_path += fd.cFileName; // добавляем имя файла в путь файла

        TVINSERTSTRUCT TVInsert; // элемент который вставим в ТВ
        TVInsert.hParent = parent; // родитель этого элемента ( предидущая папка )
        TVInsert.hInsertAfter = TVI_SORT;
        TVInsert.item.mask = TVIF_TEXT | TVIF_SELECTEDIMAGE | TVIF_PARAM; // какие поля заполнены
        TVInsert.item.pszText = fd.cFileName; // текст на элементе
        TVInsert.item.lParam = (LPARAM)ConvertWstringToLpwstr(file_path); // лпарам - полный путь к файлу
        HTREEITEM NowItemTree;

        if (FindTVItem(file_path) == nullptr) // если такой папки еще нету в ТВ - добавляем
        {
            NowItemTree = TreeView_InsertItem(hWndTV, &TVInsert); // родителем подпапок будет она сама
            ListOfTv.push_back(std::make_pair(file_path, NowItemTree)); // храним все элементы ТВ и их пути в векторе
        }
        else
            NowItemTree = FindTVItem(file_path); // находим родителя подпапок
        if (FirstTime) // для одного рекурсивного вызова
            LoadPathInTV(ConvertWstringToLpwstr(file_path), NowItemTree, false); // загружаем внутренние папки каждой папки

    } while (FindNextFileW(hFile, &fd));
}

// Загрузить папку в ТВ еще раз
void RefreshTVPath(std::wstring path) {
    std::wstring FolderPath = GetFolderPath(path); // удалить название файла из пути
    if (FolderPath.size() != 3) // если в папке, а не на диске
    {
        FolderPath = std::wstring(FolderPath.begin(), FolderPath.end() - 1); // обрезать \\ в конце
    }
    LoadPathInTV(FolderPath, FindTVItem(FolderPath)); // загрузить путь в ТВ еще раз
}

// Загурзить данные в ЛВ
void LoadPathDirInLV(std::wstring ClickedPath, bool FormatString = false)
{
    std::wstring folder = ClickedPath;
    if (folder.size() == 3)
        folder += L"*"; // если внутри тома - добавляем только звездочку
    else if (FormatString)
        folder += L"*"; // если строка передана из другой функции, а не клика пользователя и путь уже готов
    else
        folder += L"\\*"; // если внутри папки - добавляем еще и \\

    WIN32_FIND_DATA fd; // структура хранящая информацию о найденном файле
    HANDLE hFile = FindFirstFileW(ConvertWstringToLpwstr(folder), &fd); // находим первый файл
    if (hFile == INVALID_HANDLE_VALUE) // если директория пустая - выход из функции
        return;

    int ItemCounter = 0; // сколько нашли файлов
    // цикл пока не найдем все файлы в директории
    do
    {
        std::wstring FilePath = ClickedPath;
        if (FilePath.size() != 3 && !FormatString)
            FilePath += L"\\"; // добавляем \\ в путь файла
        NowFolder = FilePath; // сохраняет текущий путь

        if (!(wcscmp(fd.cFileName, L".") && wcscmp(fd.cFileName, L".."))) // пропустить . и ..
            continue;
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM) // пропускать системные файлы
            continue;

        FilePath += fd.cFileName; // добавляем имя файла в путь файла

        LV_ITEM LVInsert; // элемент который вставим в ЛВ
        LVInsert.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE; // какие поля заполнены
        LVInsert.iItem = ItemCounter; // номер элемента в папке
        LVInsert.iSubItem = 0; // подменю всегда 0, что бы работали колонки
        LVInsert.pszText = fd.cFileName; // текст на элементе
        LVInsert.lParam = (LPARAM)ConvertWstringToLpwstr(FilePath); // лпарам - полный путь к файл
        ListView_InsertItem(hWndLV, &LVInsert);

        // заполнить колонки
        LVITEM LVItemColum;
        LVItemColum.mask = LVIF_TEXT; // что заполнено в колонке
        LVItemColum.iItem = ItemCounter; // для какого элемента в папке будет заполнено
        // первая колонка
        LVItemColum.pszText = DataModifiedToString(fd.ftLastWriteTime); // текст в колонке
        LVItemColum.iSubItem = 1;  // номер колонки
        ListView_SetItem(hWndLV, &LVItemColum); // заполняем колонку

        // вторая колонка
        fd.dwFileAttributes& FILE_ATTRIBUTE_DIRECTORY ? // тернарный оператор
            LVItemColum.pszText = ConvertWstringToLpwstr(L"Папка") : LVItemColum.pszText = ConvertWstringToLpwstr(L"Файл");
        LVItemColum.iSubItem = 2; // номер колонки
        ListView_SetItem(hWndLV, &LVItemColum); // заполняем колонку

        // третья колонка
        LVItemColum.pszText = ConvertByteSize(fd.nFileSizeLow); // текст в колонке
        LVItemColum.iSubItem = 3; // номер колонки
        ListView_SetItem(hWndLV, &LVItemColum); // заполняем колонку
        ++ItemCounter; // увеличиваем количество элементов в папке

    } while (FindNextFileW(hFile, &fd));

}

void OpenOrAddInLv(std::wstring path)
{
    WIN32_FIND_DATA fd;
    // загрузить атрибуты файла из пути в структуру файла
    if (GetFileAttributesEx(ConvertWstringToLpwstr(path), GetFileExInfoStandard, &fd) != 0)
    {
        // Проверить папка ли это
        if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            // если папка - удалить все из ЛВ и загрузить эту папку
            ListView_DeleteAllItems(hWndLV);
            LoadPathDirInLV(ConvertWstringToLpwstr(path));
        }
        else
        {
            // если файл - открыть через шелл
            ShellExecute(NULL, L"open", ConvertWstringToLpwstr(path), NULL, NULL, SW_SHOWNORMAL);
        }
    }
}
// процедура диалога
INT_PTR RenameFileProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam) {
    
    switch (message)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDOK: {
            HWND hEdit = GetDlgItem(hDlg, 10); // взять HWND едита
            WCHAR FileName[100]; // буфер для текста
            GetWindowText(hEdit, FileName, 99); // считать текст с буфера в едит
            FileNewName = FileName; // записать текст в глобальную переменную  
            RenameProccesSuccess = true; // операция прошла успешно
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
        //Обеспечивает загрузку библиотеки DLL общего управления (Comctl32.dll) 
        INITCOMMONCONTROLSEX ICEX;
        ICEX.dwSize = sizeof(INITCOMMONCONTROLSEX);
        ICEX.dwICC = ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES; // что используется
        // Регистрирует определенные классы общего управления из библиотеки DLL. 
        InitCommonControlsEx(&ICEX);

        const int TV_x = 0, TV_y = 0, TV_width = 400, TV_height = 500; // константы для ТВ
        const int LV_x = 400, LV_y = 0, LV_width = 600, LV_height = 500; // константы для ЛВ
        hWndLV = CreateWindow(WC_LISTVIEWW, L"Explorer", WS_CHILD | WS_VISIBLE | WS_BORDER | LVS_REPORT | WS_HSCROLL | WS_VSCROLL,
            LV_x, LV_y, LV_width, LV_height, hWnd, (HMENU)0, hInst, nullptr); // создать ЛВ
        hWndTV = CreateWindow(WC_TREEVIEW, L"TREE VIEW", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | WS_TABSTOP | TVS_HASLINES | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_SHOWSELALWAYS, 
            TV_x, TV_y, TV_width, TV_height, hWnd, (HMENU)1, hInst, nullptr); // создать ТВ
        hLogsList = CreateWindow(L"listbox", L"Logs",  WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_NOTIFY, // создать нижний listbox
            0, TV_y + TV_height, TV_width + LV_width - 50, 260, hWnd, HMENU(4), NULL, NULL);
        LoadDriver();
        AddColumsLV();
        break;
    }
    case WM_NOTIFY: {
        NMHDR* notifyMess = (NMHDR*)lParam; // реинтерпрет каст структуры из лпарама
        switch (notifyMess->code)
        {
        case NM_DBLCLK: {
            // дабл клик левой
            if (notifyMess->hwndFrom == hWndLV)
                // если сообщение пришло от ЛВ -> открываем выбранную папку\файл
            {
                LPCWSTR path = getPathForListView(ListView_GetSelectionMark(hWndLV));
                if (path != nullptr) {
                    OpenOrAddInLv(path);
                }
            }
            else if(notifyMess->hwndFrom == hWndTV)
                // если сообщение пришло от ТВ -> загружаем папку в ЛВ
            {
                ListView_DeleteAllItems(hWndLV);
                LPCWSTR path = getPathForTreeView(TreeView_GetNextItem(hWndTV, NULL, TVGN_CARET));
                if (path != nullptr)
                    LoadPathDirInLV(path);
            }
            break;
        }
        case NM_RCLICK: {
            // клик правой
            if (notifyMess->hwndFrom == hWndLV)
            {
                POINT cursor; 
                GetCursorPos(&cursor); // взять позицию курсора
                // создать контектсное меню
                TrackPopupMenu((HMENU)GetSubMenu(LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU1)), 0), TPM_LEFTALIGN | TPM_RIGHTBUTTON, cursor.x, cursor.y, 0, hWnd, NULL);
            }
            break;
        }
        case TVN_ITEMEXPANDING: {
            // развернули или сверунли ветку в дереве
            NMTREEVIEW* data = (NMTREEVIEW*)lParam; // реинтерпрет каст структуры из лпарама
            if (data->action == 2)
                // если разворачивают
            {
                LPCWSTR path = getPathForTreeView(data->itemNew.hItem);
                if (path != nullptr) {
                    LoadPathInTV(path, data->itemNew.hItem);
                }
            }
            break;
        }
        case TVN_SELCHANGED: {
            // нажали лкм на другую папку в дереве
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
        // обработка фукнций выпадающего меню
        {
            int wmId = LOWORD(wParam);
            switch (wmId)
            {
            case 1: case 2: // копировать - 1 или вставить - 2
            { 
                int index = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                if (index == -1) // если файл не выбран
                    break;
                Replace = wmId == 1 ? false : true; // если копирование - false, если перемещение - true
                Copy = wmId == 1 ? true : false; // если копирование - true, если перемещение - false
                CopyFrom = getPathForListView(index); // путь файла копирования
                break;
            }
            case 3: // вставить
            { 
                if (!Copy && !Replace) // если операция не выбрана - выход
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

                SHFILEOPSTRUCTW OperationInfo = { 0 }; // стркутура файловой операции
                OperationInfo.hwnd = hWnd;
                OperationInfo.pFrom = CopyFromComplete;
                OperationInfo.pTo = CopyTo;
                OperationInfo.wFunc = Replace ? FO_MOVE : FO_COPY;
                OperationInfo.fFlags = FOF_RENAMEONCOLLISION;
                SHFileOperationW(&OperationInfo); // выполнение файловой операции
                // обновление ЛВ
                ListView_DeleteAllItems(hWndLV);
                LoadPathDirInLV(GetFolderPath(CopyTo), true);
                // обновление ТВ
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

                std::wstring log = L"Файл вставлено з ";
                log += CopyFromComplete;
                log += L" в ";
                log += CopyTo;
                SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                break;
            }
            case 4: // удалить
            {
                int index = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                if (index == -1) // если файл не выбран
                    break;

                std::wstring ErasePath = getPathForListView(index);
                WCHAR* ErasePathComplete = new WCHAR[ErasePath.size() + 3];
                wcscpy(ErasePathComplete, ErasePath.c_str());
                ErasePathComplete[ErasePath.size() ] = L'\\';
                ErasePathComplete[ErasePath.size() + 1] = L'\0';
                ErasePathComplete[ErasePath.size() + 2] = L'\0';
                SHFILEOPSTRUCTW OperationInfo = { 0 }; // стркутура файловой операции
                OperationInfo.hwnd = hWnd;
                OperationInfo.pFrom = ErasePathComplete;
                OperationInfo.wFunc = FO_DELETE;
                OperationInfo.fFlags = FOF_ALLOWUNDO; // файл попадет в корзину, а не сразу удалится 
                SHFileOperationW(&OperationInfo); // выполнение файловой операции
                DWORD error = GetLastError();
                if (error != 0)
                {
                    std::wstring log = L"Не вдалося видалити папку, потрібні права адміністратора";
                    SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                    break;
                }
                // удаление из ТВ 
                HTREEITEM DeletItem = FindTVItem(ErasePath);
                if (DeletItem != nullptr)
                {
                    DeletePathFromTVlist(ErasePath);
                    TreeView_DeleteItem(hWndTV, DeletItem);
                }
                // обновление ЛВ
                ListView_DeleteAllItems(hWndLV);
                LoadPathDirInLV(GetFolderPath(ErasePath), true);

                std::wstring log = L"Файл видалений - ";
                log += ErasePathComplete;
                SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                break;
            }
            case 5: { // переименовать
                int index = ListView_GetNextItem(hWndLV, -1, LVNI_SELECTED);
                if (index == -1) // если файл не выбран
                    break;
                std::wstring RenamePath = std::wstring(getPathForListView(index));
                WCHAR* RenamePathComplete = new WCHAR[RenamePath.size() + 3];
                wcscpy(RenamePathComplete, RenamePath.c_str());
                RenamePathComplete[RenamePath.size()] = L'\\';
                RenamePathComplete[RenamePath.size() + 1] = L'\0';
                RenamePathComplete[RenamePath.size() + 2] = L'\0';

                std::wstring NewNameWstring = GetFolderPath(std::wstring(RenamePath.begin(), RenamePath.end() - 1));
                DialogBox(hInst, MAKEINTRESOURCE(DialogRenameFileProc), hWnd, RenameFileProc);
                if (!RenameProccesSuccess) // если нажали кнопку отменить в диалоге
                    break;
                    
                NewNameWstring += FileNewName;
                WCHAR* NewName = new WCHAR[NewNameWstring.size() + 3];
                wcscpy(NewName, NewNameWstring.c_str());
                NewName[NewNameWstring.size()] = L'\\';
                NewName[NewNameWstring.size() + 1] = L'\0';
                NewName[NewNameWstring.size() + 2] = L'\0';

                SHFILEOPSTRUCTW OperationInfo = { 0 }; // стркутура файловой операции
                OperationInfo.hwnd = hWnd;
                OperationInfo.pFrom = RenamePathComplete;
                OperationInfo.pTo = NewName;
                OperationInfo.wFunc = FO_RENAME;
                OperationInfo.fFlags = FOF_RENAMEONCOLLISION | FOF_ALLOWUNDO;
                // выполнение файловой операции
                SHFileOperationW(&OperationInfo);
                // обновление ЛВ
                ListView_DeleteAllItems(hWndLV);
                LoadPathDirInLV(GetFolderPath(RenamePath), true);
                // обновление ТВ
                HTREEITEM DeletItem = FindTVItem(RenamePath); 
                if (DeletItem != nullptr)
                {
                    DeletePathFromTVlist(RenamePath); // удаляем старую папку, если нужно из списка итемов
                    TreeView_DeleteItem(hWndTV, DeletItem); // удаляем старую папку, если нужно из ТВ
                }
                RefreshTVPath(RenamePath + std::wstring(L"\\"));
                RefreshTVPath(NowFolder);


                std::wstring log = L"Файл переименований з ";
                log += RenamePathComplete;
                log += L" в ";
                log += NewName;
                SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                break;
            }
            case 6: // создать папку 
            {
                std::wstring NewFolderPath = NowFolder + std::wstring(L"Нова папка");
                CreateDirectory(ConvertWstringToLpwstr(NewFolderPath), NULL);
                DWORD error = GetLastError();
                if (error != 0)
                {
                    std::wstring log = L"Папка не створена, спочатку перейменуйте минулу створену папку";
                    SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                    break;
                }
                // обновление ЛВ
                ListView_DeleteAllItems(hWndLV);
                LoadPathDirInLV(GetFolderPath(NowFolder), true); 
                // обновление ТВ
                RefreshTVPath(NewFolderPath);
                std::wstring log = L"Створена папка ";
                log += NewFolderPath;
                SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                break;
            }
            case 7: // создать файл
            {
                std::wstring NewFilePath = NowFolder + std::wstring(L"Новий файл.txt");
                // создает новый файл, если нету файла с таким же названием в каталоге
                HANDLE file = CreateFile( NewFilePath.c_str(),NULL, FILE_SHARE_DELETE,NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,  NULL);
                DWORD error = GetLastError();
                if (error != 0)
                {
                    std::wstring log = L"Файл не створен, спочатку перейменуйте попередній створений файл";
                    SendMessage(hLogsList, LB_ADDSTRING, 0, (LPARAM)log.c_str());
                    break;
                }
                CloseHandle(file);
                // обновление ЛВ
                ListView_DeleteAllItems(hWndLV);
                LoadPathDirInLV(GetFolderPath(NowFolder), true);
                std::wstring log = L"Створен файл ";
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
        // что бы нельзя было изменить размер окна
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
