#include <stdio.h>
#include <windows.h>

static const char kWindowsClass[] = "MsgTestWindowsClass";
static const char kUniqueMessageClass[] = "MsgTestMessageClass";
enum {
    kMsgTestHi,
    kMsgTestBye
};

static DWORD thread_main; // Main thread ID.

BOOL WINAPI CleanExitHandler(DWORD) {
    puts("info: received shutdown signal");

    // Send an application termination message to the main thread.
    // Usually this is done with the PostQuitMessaage function,
    // but the CTRL+C and similar functions work in a different
    // thread, so you cannot use PostQuitMessage to only send
    // WM_QUIT to the current thread.
    PostThreadMessage(thread_main, WM_QUIT, 0, 0);
    return TRUE;
}

int main() {
    // Get the address of the application executable image file,
    // which is often used in the "window" part of WinAPI.
    HINSTANCE h_instance = static_cast<HINSTANCE>(GetModuleHandle(NULL));

    // Registration of a very simple window class.
    WNDCLASSEX wc;
    memset(&wc, 0, sizeof(wc));
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = DefWindowProc;
    wc.hIstance      = h_instance;
    wc.lpszClassName = kWindowClass;

    if (!RegisterClassEx(&wc)) {
        printf("error: failed to register window class (%u)\n", static_cast<unsigned int>(GetLastError()));
        return 1;
    }

    // Create a window in the system. The window itself will not be displayed
    // (you would have to call ShowWindow and UpdateWindow for this),
    // but this does not prevent broadcast messages from being received.
    HWND hwnd = CreateWindowEx(0, kWindowClass, NULL, 0, 0, 0, 0, 0, NULL, NULL, h_instance, NULL);
    if (hwnd == NULL) {
        printf("error: failed to create a window (%u)\n", static_cast<unsigned int>(GetLastError()));
        return 1;
    }

    // Registration of a unique number of the message type in the system.
    // All other processes that register the same message name will receive the exact same ID.
    UINT msgtest_msg_class = RegisterWindowMessage(kUniqueMessageClass);
    printf("info: unique message ID is %u\n", msgtest_msg_class);

    // Send a welcome message to other processes.
    printf("info: my process ID is %u\n", static_cast<unsigned int>(GetCurrentProcessId()));

    PostMessage(HWND_BROADCAST, msgtest_msg_class,
        static_cast<WPARAM>(GetCurrentProcessId()),
        static_cast<LPARAM>(kMsgTestHi));

    // Register a CTRL+C signal handler to let you know the message loop to end the process in a controlled fashion.
    thread_main = GetCurrentThreadId();
    SetConsoleCtrlHandler(CleanExitHandler, TRUE);

    // Receive messages until you receive information about the end of the process.
    // Technically GetMessage will return FALSE after receiving WM_QUIT message.
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        if (msg.message == msgtest_msg_class) {
            int m = static_cast<int>(msg.lParam);
            printf("info: process %u says '%s'\n",
                    static_cast<unsigned int>(msg.wParam),
                    m == kMsgTestHi ? "Hi!" :
                    m == kMsgTestBye ? "Bye!" : "???");
        } else {
            DispatchMessage(&msg);
        }
    }

    PostMessage(HWND_BROADCAST, msgtest_msg_class,
        static_cast<WPARAM>(GetCurrentProcessId()),
        static_cast<LPARAM>(kMsgTestBye));
    DestroyWindow(hwnd);
    UnregisterClass(kWindowClass, h_instance);
    puts("info: bye!");
    return 0;
}