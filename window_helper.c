typedef struct handle_data {
    unsigned long process_id;
    HWND window_handle;
} handle_data;

BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
    handle_data data = *(handle_data*)lParam;
    DWORD process_id = 0;
    GetWindowThreadProcessId(handle, &process_id);
    if (data.process_id != process_id){
        return TRUE;
    }
    (*(handle_data *)lParam).window_handle = handle;
    return FALSE;   
}

HWND find_main_window(unsigned long process_id)
{
    handle_data data;
    data.process_id = process_id;
    data.window_handle = 0;
    EnumWindows(enum_windows_callback, (LPARAM)&data);
    return data.window_handle;
}

BOOL IsForegroundProcess(DWORD pid)
{
   HWND hwnd = GetForegroundWindow();
   if (hwnd == NULL) return FALSE;

   DWORD foregroundPid;
   if (GetWindowThreadProcessId(hwnd, &foregroundPid) == 0) return FALSE;

   return (foregroundPid == pid);
}

