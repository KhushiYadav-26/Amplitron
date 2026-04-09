// =============================================================================
// Native file dialog implementations (Windows, macOS, Linux)
// Open dialog implementation
// =============================================================================

#include "gui/file_dialog.h"
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commdlg.h>
#endif

#ifdef __APPLE__
#include <cstdio>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#endif

#ifndef _WIN32
#include <sys/wait.h>
#endif

namespace Amplitron {

#ifdef _WIN32
std::string show_open_dialog(const std::string& title,
                             const std::string& filter_desc,
                             const std::string& filter_ext) {
    char filename[MAX_PATH] = "";

    char filter[256];
    std::memset(filter, 0, sizeof(filter));
    int pos = 0;
    pos += snprintf(filter + pos, 256 - pos, "%s (*.%s)", filter_desc.c_str(), filter_ext.c_str());
    pos++;
    pos += snprintf(filter + pos, 256 - pos, "*.%s", filter_ext.c_str());
    pos++;
    pos += snprintf(filter + pos, 256 - pos, "All Files (*.*)");
    pos++;
    pos += snprintf(filter + pos, 256 - pos, "*.*");

    OPENFILENAMEA ofn;
    std::memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = filter;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = title.c_str();
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    }
    return "";
}

#elif defined(__APPLE__)
std::string show_open_dialog(const std::string& title,
                             const std::string& /*filter_desc*/,
                             const std::string& filter_ext) {
    // Sanitize title for AppleScript
    std::string safe_title;
    for (char c : title) {
        if (c == '\\') { safe_title += "\\\\"; }
        else if (c == '"') { safe_title += "\\\""; }
        else { safe_title += c; }
    }

    std::string script = "POSIX path of (choose file of type {\"" + filter_ext +
                         "\"} with prompt \"" + safe_title + "\")";

    // Use fork+exec to invoke osascript directly
    int pipefd[2];
    if (pipe(pipefd) != 0) return "";

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return "";
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) { dup2(devnull, STDERR_FILENO); close(devnull); }
        execl("/usr/bin/osascript", "osascript", "-e", script.c_str(), nullptr);
        _exit(1);
    }

    close(pipefd[1]);
    char buf[1024];
    std::string result;
    ssize_t n;
    while ((n = read(pipefd[0], buf, sizeof(buf))) > 0)
        result.append(buf, static_cast<size_t>(n));
    close(pipefd[0]);

    int status = 0;
    waitpid(pid, &status, 0);

    while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();

    return result;
}

#else // Linux
std::string show_open_dialog(const std::string& title,
                             const std::string& filter_desc,
                             const std::string& filter_ext) {
    std::string safe_title;
    for (char c : title) {
        if (c == '\'') { safe_title += "'\\''"; }
        else { safe_title += c; }
    }

    std::string cmd = "zenity --file-selection "
                      "--title='" + safe_title + "' "
                      "--file-filter='" + filter_desc + " (*." + filter_ext + ")|*." + filter_ext + "' "
                      "--file-filter='All Files (*)|*' 2>/dev/null";

    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";

    char buf[1024];
    std::string result;
    while (fgets(buf, sizeof(buf), pipe)) {
        result += buf;
    }
    int wait_status = pclose(pipe);

    if (WIFEXITED(wait_status) && WEXITSTATUS(wait_status) != 0) {
        cmd = "kdialog --getopenfilename ~/ '*." + filter_ext + "|" + filter_desc + "' "
              "--title '" + safe_title + "' 2>/dev/null";
        pipe = popen(cmd.c_str(), "r");
        if (!pipe) return "";
        result.clear();
        while (fgets(buf, sizeof(buf), pipe)) {
            result += buf;
        }
        pclose(pipe);
    }

    while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();

    return result;
}
#endif

} // namespace Amplitron
