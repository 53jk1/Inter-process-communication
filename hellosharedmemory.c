#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

int main(void) {
    // Create a new section (shared memory) and map it to the process space.

    HANDLE h = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, "Local\\HelloWorld");

    void *p = MapViewOfFile(h, FILE_MAP_WRITE, 0, 0, 0);

    // Copy the message to the beginning of the mapped section.
    const char *message = "Hello World!\x1A";
    memcpy(p, message, strlen(message));

    // Wait for the program to end.
    puts("Press ENTER to leave...");
    getchar();

    UnmapViewOfFile(p);
    CloseHandle(h);
    return 0;
}