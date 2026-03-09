#include "app/App.h"

#include <windows.h>

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, PWSTR, int showCmd) {
    rv::app::App app;
    return app.Run(instance, showCmd);
}
