// mkfile.c
// Win32-only "mkfile" for Windows.
// Creates empty files (or overwrites with -f). Optionally creates parent dirs (-p).

#define UNICODE
#define _UNICODE
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shellapi.h>

static void WriteStrW(HANDLE h, const wchar_t *s) {
  DWORD n = 0;
  if (s) {
    int len = lstrlenW(s);
    if (len > 0) {
      WriteConsoleW(h, s, (DWORD)len, &n, 0);
    }
  }
}

static void WriteLnW(HANDLE h, const wchar_t *s) {
  WriteStrW(h, s);
  WriteStrW(h, L"\r\n");
}

static void WriteLastErrorW(const wchar_t *prefix) {
  HANDLE err = GetStdHandle(STD_ERROR_HANDLE);

  DWORD e = GetLastError();
  wchar_t *msg = 0;

  DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;
  DWORD got = FormatMessageW(flags, 0, e, 0, (wchar_t *)&msg, 0, 0);

  if (prefix) {
    WriteStrW(err, prefix);
  }
  if (got && msg) {
    WriteStrW(err, msg);
    LocalFree(msg);
  } else {
    WriteLnW(err, L"(FormatMessageW failed)");
  }
}

static int IsSep(wchar_t c) {
  return (c == L'\\') || (c == L'/');
}

static BOOL EnsureParentDirs(const wchar_t *path) {
  // Create directories along the path up to the final component.
  // Does nothing if there is no parent component.
  if (!path || !path[0]) {
    return FALSE;
  }

  int len = lstrlenW(path);
  if (len <= 0) {
    return FALSE;
  }

  // Copy path to a mutable buffer.
  SIZE_T bytes = (SIZE_T)(len + 1) * sizeof(wchar_t);
  wchar_t *buf = (wchar_t *)HeapAlloc(GetProcessHeap(), 0, bytes);
  if (!buf) {
    SetLastError(ERROR_OUTOFMEMORY);
    return FALSE;
  }
  CopyMemory(buf, path, bytes);

  // Identify how far we should iterate (stop before final component).
  int end = len;
  while (end > 0 && IsSep(buf[end - 1])) {
    end--;
  }
  while (end > 0 && !IsSep(buf[end - 1])) {
    end--;
  }
  if (end <= 0) {
    HeapFree(GetProcessHeap(), 0, buf);
    return TRUE;
  }

  // Skip prefixes we must not try to CreateDirectory on:
  // - Drive root: "C:\"
  // - UNC prefix: "\\server\share\"
  int i = 0;

  // UNC: \\server\share\...
  if (len >= 2 && IsSep(buf[0]) && IsSep(buf[1])) {
    i = 2;
    // server
    while (i < end && !IsSep(buf[i])) {
      i++;
    }
    while (i < end && IsSep(buf[i])) {
      i++;
    }
    // share
    while (i < end && !IsSep(buf[i])) {
      i++;
    }
    while (i < end && IsSep(buf[i])) {
      i++;
    }
  } else if (len >= 3 && buf[1] == L':' && IsSep(buf[2])) {
    // Drive root "C:\"
    i = 3;
  }

  // Walk separators; create each intermediate directory.
  for (; i < end; i++) {
    if (IsSep(buf[i])) {
      wchar_t saved = buf[i];
      buf[i] = 0;

      if (buf[0]) {
        if (!CreateDirectoryW(buf, 0)) {
          DWORD e = GetLastError();
          if (e != ERROR_ALREADY_EXISTS) {
            buf[i] = saved;
            HeapFree(GetProcessHeap(), 0, buf);
            SetLastError(e);
            return FALSE;
          }
        }
      }

      buf[i] = saved;
      // Skip repeated seps
      while (i + 1 < end && IsSep(buf[i + 1])) {
        i++;
      }
    }
  }

  HeapFree(GetProcessHeap(), 0, buf);
  return TRUE;
}

static void PrintUsage(void) {
  HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
  WriteLnW(out, L"mkfile - create empty file(s)");
  WriteLnW(out, L"");
  WriteLnW(out, L"Usage:");
  WriteLnW(out, L"  mkfile [-f] [-p] <path> [path ...]");
  WriteLnW(out, L"");
  WriteLnW(out, L"Options:");
  WriteLnW(out, L"  -f   overwrite if file exists");
  WriteLnW(out, L"  -p   create parent directories");
}

int wmain(void) {
  int exit_code = 0;

  int argc = 0;
  wchar_t **argv = CommandLineToArgvW(GetCommandLineW(), &argc);
  if (!argv) {
    WriteLastErrorW(L"CommandLineToArgvW failed: ");
    return 1;
  }

  BOOL overwrite = FALSE;
  BOOL make_parents = FALSE;

  int first_path = 1;
  for (int i = 1; i < argc; i++) {
    const wchar_t *a = argv[i];
    if (a && a[0] == L'-' && a[1] != 0) {
      if (lstrcmpiW(a, L"-f") == 0) {
        overwrite = TRUE;
      } else if (lstrcmpiW(a, L"-p") == 0) {
        make_parents = TRUE;
      } else if (lstrcmpiW(a, L"-h") == 0 || lstrcmpiW(a, L"--help") == 0 || lstrcmpiW(a, L"/?") == 0) {
        PrintUsage();
        LocalFree(argv);
        return 0;
      } else {
        HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
        WriteStrW(err, L"Unknown option: ");
        WriteLnW(err, a);
        PrintUsage();
        LocalFree(argv);
        return 1;
      }
      first_path = i + 1;
    } else {
      break;
    }
  }

  if (first_path >= argc) {
    PrintUsage();
    LocalFree(argv);
    return 1;
  }

  for (int i = first_path; i < argc; i++) {
    const wchar_t *path = argv[i];
    if (!path || !path[0]) {
      continue;
    }

    if (make_parents) {
      if (!EnsureParentDirs(path)) {
        HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
        WriteStrW(err, L"Failed to create parent dirs for: ");
        WriteLnW(err, path);
        WriteLastErrorW(L"  Error: ");
        exit_code = 1;
        continue;
      }
    }

    DWORD disposition = overwrite ? CREATE_ALWAYS : CREATE_NEW;

    HANDLE h = CreateFileW(
      path,
      GENERIC_WRITE,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      0,
      disposition,
      FILE_ATTRIBUTE_NORMAL,
      0
    );

    if (h == INVALID_HANDLE_VALUE) {
      HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
      WriteStrW(err, L"Failed to create: ");
      WriteLnW(err, path);
      WriteLastErrorW(L"  Error: ");
      exit_code = 1;
      continue;
    }

    CloseHandle(h);
  }

  LocalFree(argv);
  return exit_code;
}
