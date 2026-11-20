#include "resource.h"
#include "PolarityTestController.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <commctrl.h>
#include <string>
#include <sstream>

#pragma comment(lib, "Comctl32.lib")

namespace
{
    const wchar_t* kWindowClass = L"SantecPolarityTestAppWindow";
    const UINT WM_TEST_COMPLETE = WM_APP + 1;

    struct WorkerParameters
    {
        HWND window;
        PolarityTestController* controller;
        int cores;
        char polarityType;
        std::string profile;
    };

    struct WindowState
    {
        PolarityTestController controller;
        HANDLE worker;
        HWND host;
        HWND port;
        HWND connect;
        HWND disconnect;
        HWND cores;
        HWND type;
        HWND profile;
        HWND run;
        HWND cancel;
        HWND results;
        HWND status;
        HWND identity;
        WindowState() : worker(NULL), host(NULL), port(NULL), connect(NULL), disconnect(NULL),
            cores(NULL), type(NULL), profile(NULL), run(NULL), cancel(NULL), results(NULL),
            status(NULL), identity(NULL) {}
    };

    std::string WideToUtf8(const std::wstring& text)
    {
        if (text.empty()) return std::string();
        const int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                                               NULL, 0, NULL, NULL);
        std::string result(static_cast<size_t>(length), '\0');
        WideCharToMultiByte(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()),
                            &result[0], length, NULL, NULL);
        return result;
    }

    std::wstring Utf8ToWide(const std::string& text)
    {
        if (text.empty()) return std::wstring();
        const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), NULL, 0);
        std::wstring result(static_cast<size_t>(length), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &result[0], length);
        return result;
    }

    std::wstring GetText(HWND control)
    {
        const int length = GetWindowTextLengthW(control);
        std::wstring value(static_cast<size_t>(length + 1), L'\0');
        if (length > 0)
            GetWindowTextW(control, &value[0], length + 1);
        value.resize(static_cast<size_t>(length));
        return value;
    }

    BOOL CALLBACK SetChildFont(HWND child, LPARAM fontValue)
    {
        SendMessageW(child, WM_SETFONT, static_cast<WPARAM>(fontValue), TRUE);
        return TRUE;
    }

    void SetStatus(WindowState* state, const std::wstring& text)
    {
        SetWindowTextW(state->status, text.c_str());
    }

    void SetConnectedUi(WindowState* state, bool connected)
    {
        EnableWindow(state->connect, !connected);
        EnableWindow(state->disconnect, connected);
        EnableWindow(state->run, connected && state->worker == NULL);
        EnableWindow(state->cancel, state->worker != NULL);
    }

    void AddColumn(HWND list, int index, int width, const wchar_t* title)
    {
        LVCOLUMNW column;
        ZeroMemory(&column, sizeof(column));
        column.mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        column.pszText = const_cast<wchar_t*>(title);
        column.cx = width;
        column.iSubItem = index;
        ListView_InsertColumn(list, index, &column);
    }

    void SetListCell(HWND list, int row, int column, const std::wstring& text)
    {
        if (column == 0)
        {
            LVITEMW item;
            ZeroMemory(&item, sizeof(item));
            item.mask = LVIF_TEXT;
            item.iItem = row;
            item.pszText = const_cast<wchar_t*>(text.c_str());
            ListView_InsertItem(list, &item);
        }
        else
            ListView_SetItemText(list, row, column, const_cast<wchar_t*>(text.c_str()));
    }

    DWORD WINAPI TestWorker(void* parameter)
    {
        WorkerParameters* data = static_cast<WorkerParameters*>(parameter);
        AppTestResult* result = new AppTestResult(
            data->controller->RunTest(data->cores, data->polarityType, data->profile, 60000));
        PostMessageW(data->window, WM_TEST_COMPLETE, 0, reinterpret_cast<LPARAM>(result));
        delete data;
        return 0;
    }

    void CreateControls(HWND window, WindowState* state)
    {
        HFONT font = static_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
#define MAKE_CONTROL(style, cls, text, x, y, w, h, id) \
        CreateWindowExW(0, cls, text, style | WS_CHILD | WS_VISIBLE, x, y, w, h, window, \
                        reinterpret_cast<HMENU>(id), GetModuleHandleW(NULL), NULL)

        MAKE_CONTROL(0, L"STATIC", L"TCP Host", 15, 18, 60, 20, 0);
        state->host = MAKE_CONTROL(WS_BORDER | ES_AUTOHSCROLL, L"EDIT", L"127.0.0.1", 80, 15, 145, 24, IDC_HOST);
        MAKE_CONTROL(0, L"STATIC", L"Port", 238, 18, 35, 20, 0);
        state->port = MAKE_CONTROL(WS_BORDER | ES_NUMBER, L"EDIT", L"5025", 275, 15, 65, 24, IDC_PORT);
        state->connect = MAKE_CONTROL(BS_PUSHBUTTON, L"BUTTON", L"Connect", 355, 14, 85, 26, IDC_CONNECT);
        state->disconnect = MAKE_CONTROL(BS_PUSHBUTTON, L"BUTTON", L"Disconnect", 450, 14, 90, 26, IDC_DISCONNECT);
        state->identity = MAKE_CONTROL(SS_LEFT, L"STATIC", L"Not connected", 555, 18, 320, 20, IDC_IDENTITY);

        MAKE_CONTROL(0, L"STATIC", L"Cores", 15, 58, 45, 20, 0);
        state->cores = MAKE_CONTROL(CBS_DROPDOWNLIST | WS_VSCROLL, L"COMBOBOX", L"", 65, 54, 80, 120, IDC_CORES);
        SendMessageW(state->cores, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"12"));
        SendMessageW(state->cores, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"24"));
        SendMessageW(state->cores, CB_SETCURSEL, 1, 0);

        MAKE_CONTROL(0, L"STATIC", L"Type", 160, 58, 35, 20, 0);
        state->type = MAKE_CONTROL(CBS_DROPDOWNLIST | WS_VSCROLL, L"COMBOBOX", L"", 200, 54, 80, 120, IDC_TYPE);
        SendMessageW(state->type, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"A"));
        SendMessageW(state->type, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"B"));
        SendMessageW(state->type, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(L"C"));
        SendMessageW(state->type, CB_SETCURSEL, 0, 0);

        MAKE_CONTROL(0, L"STATIC", L"Simulator profile", 300, 58, 100, 20, 0);
        state->profile = MAKE_CONTROL(CBS_DROPDOWNLIST | WS_VSCROLL, L"COMBOBOX", L"", 405, 54, 145, 160, IDC_PROFILE);
        const wchar_t* profiles[] = { L"DEVICE DEFAULT", L"PASS", L"CROSS", L"OPEN", L"SHORT", L"REVERSE" };
        for (int i = 0; i < 6; ++i)
            SendMessageW(state->profile, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(profiles[i]));
        SendMessageW(state->profile, CB_SETCURSEL, 1, 0);

        state->run = MAKE_CONTROL(BS_DEFPUSHBUTTON, L"BUTTON", L"Run Test", 570, 53, 100, 28, IDC_RUN);
        state->cancel = MAKE_CONTROL(BS_PUSHBUTTON, L"BUTTON", L"Cancel", 680, 53, 90, 28, IDC_CANCEL);
        state->status = MAKE_CONTROL(SS_LEFT, L"STATIC", L"Ready", 15, 92, 850, 22, IDC_STATUS);

        state->results = CreateWindowExW(WS_EX_CLIENTEDGE, WC_LISTVIEWW, L"",
            WS_CHILD | WS_VISIBLE | LVS_REPORT | LVS_SINGLESEL, 15, 120, 860, 420,
            window, reinterpret_cast<HMENU>(IDC_RESULTS), GetModuleHandleW(NULL), NULL);
        ListView_SetExtendedListViewStyle(state->results, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_DOUBLEBUFFER);
        AddColumn(state->results, 0, 85, L"Input Core");
        AddColumn(state->results, 1, 125, L"Expected Output");
        AddColumn(state->results, 2, 125, L"Observed Output");
        AddColumn(state->results, 3, 110, L"Light");
        AddColumn(state->results, 4, 180, L"Status");

        EnumChildWindows(window, SetChildFont, reinterpret_cast<LPARAM>(font));
#undef MAKE_CONTROL
    }

    void BeginTest(HWND window, WindowState* state)
    {
        if (state->worker != NULL || !state->controller.IsConnected())
            return;
        const int coreSelection = static_cast<int>(SendMessageW(state->cores, CB_GETCURSEL, 0, 0));
        const int typeSelection = static_cast<int>(SendMessageW(state->type, CB_GETCURSEL, 0, 0));
        const int profileSelection = static_cast<int>(SendMessageW(state->profile, CB_GETCURSEL, 0, 0));
        wchar_t profileText[64] = { 0 };
        SendMessageW(state->profile, CB_GETLBTEXT, profileSelection, reinterpret_cast<LPARAM>(profileText));

        WorkerParameters* parameters = new WorkerParameters();
        parameters->window = window;
        parameters->controller = &state->controller;
        parameters->cores = coreSelection == 0 ? 12 : 24;
        parameters->polarityType = static_cast<char>('A' + typeSelection);
        parameters->profile = WideToUtf8(profileText);
        ListView_DeleteAllItems(state->results);
        SetStatus(state, L"Test running...");
        state->worker = CreateThread(NULL, 0, TestWorker, parameters, 0, NULL);
        if (state->worker == NULL)
        {
            delete parameters;
            SetStatus(state, L"Unable to create test worker thread");
        }
        SetConnectedUi(state, true);
    }

    LRESULT CALLBACK WindowProcedure(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
    {
        WindowState* state = reinterpret_cast<WindowState*>(GetWindowLongPtrW(window, GWLP_USERDATA));
        switch (message)
        {
        case WM_CREATE:
            state = new WindowState();
            SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(state));
            CreateControls(window, state);
            SetConnectedUi(state, false);
            return 0;

        case WM_COMMAND:
            if (state == NULL) break;
            switch (LOWORD(wParam))
            {
            case IDC_CONNECT:
            {
                std::string identity, error;
                const std::string host = WideToUtf8(GetText(state->host));
                const int portValue = _wtoi(GetText(state->port).c_str());
                SetStatus(state, L"Connecting...");
                if (portValue > 0 && portValue <= 65535 &&
                    state->controller.Connect(host, static_cast<unsigned short>(portValue), identity, error))
                {
                    SetWindowTextW(state->identity, Utf8ToWide(identity).c_str());
                    SetStatus(state, L"Connected");
                    SetConnectedUi(state, true);
                }
                else
                {
                    SetStatus(state, Utf8ToWide(error.empty() ? "Invalid port" : error));
                    SetConnectedUi(state, false);
                }
                return 0;
            }
            case IDC_DISCONNECT:
                state->controller.Disconnect();
                SetWindowTextW(state->identity, L"Not connected");
                SetStatus(state, L"Disconnected (laser disable requested)");
                SetConnectedUi(state, false);
                return 0;
            case IDC_RUN:
                BeginTest(window, state);
                return 0;
            case IDC_CANCEL:
                state->controller.RequestCancel();
                SetStatus(state, L"Cancellation requested...");
                return 0;
            }
            break;

        case WM_TEST_COMPLETE:
            if (state != NULL)
            {
                AppTestResult* result = reinterpret_cast<AppTestResult*>(lParam);
                if (state->worker != NULL)
                {
                    CloseHandle(state->worker);
                    state->worker = NULL;
                }
                if (result->communicationSucceeded)
                {
                    for (size_t i = 0; i < result->cores.size(); ++i)
                    {
                        const AppCoreResult& item = result->cores[i];
                        std::wostringstream number;
                        number << item.inputCore; SetListCell(state->results, static_cast<int>(i), 0, number.str());
                        number.str(L""); number.clear(); number << item.expectedOutput;
                        SetListCell(state->results, static_cast<int>(i), 1, number.str());
                        number.str(L""); number.clear(); number << item.observedOutput;
                        SetListCell(state->results, static_cast<int>(i), 2, number.str());
                        SetListCell(state->results, static_cast<int>(i), 3, item.lightDetected ? L"Yes" : L"No");
                        SetListCell(state->results, static_cast<int>(i), 4, Utf8ToWide(item.status));
                    }
                    SetStatus(state, result->passed ? L"PASS - all cores match" : L"FAIL - inspect the core table");
                }
                else
                    SetStatus(state, Utf8ToWide(result->error));
                delete result;
                SetConnectedUi(state, state->controller.IsConnected());
            }
            return 0;

        case WM_CLOSE:
            if (state != NULL && state->worker != NULL)
            {
                state->controller.RequestCancel();
                SetStatus(state, L"Cancel the running test before closing.");
                return 0;
            }
            DestroyWindow(window);
            return 0;

        case WM_DESTROY:
            if (state != NULL)
            {
                state->controller.Disconnect();
                delete state;
                SetWindowLongPtrW(window, GWLP_USERDATA, 0);
            }
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }
}

int APIENTRY wWinMain(HINSTANCE instance, HINSTANCE, wchar_t*, int showCommand)
{
    INITCOMMONCONTROLSEX controls;
    controls.dwSize = sizeof(controls);
    controls.dwICC = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx(&controls);

    WNDCLASSEXW windowClass;
    ZeroMemory(&windowClass, sizeof(windowClass));
    windowClass.cbSize = sizeof(windowClass);
    windowClass.lpfnWndProc = WindowProcedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    windowClass.lpszClassName = kWindowClass;
    windowClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    windowClass.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    if (!RegisterClassExW(&windowClass))
        return 1;

    HWND window = CreateWindowExW(0, kWindowClass, L"Santec Polarity TCP/SCPI Test Tool",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 910, 590, NULL, NULL, instance, NULL);
    if (window == NULL)
        return 2;

    ShowWindow(window, showCommand);
    UpdateWindow(window);
    MSG message;
    while (GetMessageW(&message, NULL, 0, 0) > 0)
    {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return static_cast<int>(message.wParam);
}
