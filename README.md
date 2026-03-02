# mkfile

A minimal Win32-only implementation of `mkfile` for Windows.

Creates empty files from the command line, similar to `touch` (but without timestamp logic).

No `<stdlib.h>`. No CRT helpers in user code. Pure Win32 API.

---

## Features

- Create one or more empty files
- `-f` overwrite existing files
- `-p` create parent directories
- Supports relative, absolute, and UNC paths
- Unicode-aware (`wmain`, wide Win32 APIs)

---

## Usage

```
mkfile [-f] [-p] <path> [path ...]
```

### Options

- `-f`  
  Overwrite file if it already exists

- `-p`  
  Create parent directories if they do not exist

- `-h`, `--help`, `/?`  
  Show usage

---

## Examples

Create a single file:

```
mkfile foo.txt
```

Create multiple files:

```
mkfile a.txt b.txt c.txt
```

Overwrite existing file:

```
mkfile -f foo.txt
```

Create nested directories automatically:

```
mkfile -p out\build\logs\trace.txt
```

---

## Building

Open a **Developer Command Prompt for Visual Studio**.

Run:

```
build.bat
```

This is a user-mode Win32 console tool. It still uses CRT startup (`wmain`) but does not depend on `<stdlib.h>` or standard C runtime helpers in its logic.

---

## Why?

Because Windows has `mkdir` but not `mkfile`.

Sometimes you just want:

```
mkfile foo.cpp
```

without opening Notepad.
