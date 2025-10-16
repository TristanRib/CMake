repo_uploader
===============

Small cross-platform CLI (C++17) to copy files/resources into a local git repository and run git add/commit/push.

Build
-----

Requirements: CMake >= 3.10, a C++17 compiler, git.

Windows (PowerShell):

```powershell
mkdir build; cd build; cmake ..; cmake --build . --config Release
```

Linux/macOS:

```bash
mkdir build; cd build; cmake ..; make
```

Usage
-----

Basic:

```text
repo_uploader <source_path> <repo_path> <commit_message> [--push]
```

Example:

```text
repo_uploader C:\Users\trist\Documents\challenge1 C:\repos\my-challenges "Add challenge1 files" --push
```

Notes
-----
- The tool uses the system `git` command. Ensure you have proper credentials configured (SSH keys or credential helper / personal access token).
- On Windows the program uses PowerShell/cmd-compatible `cd /d` to change drives.
- This is a simple utility. Be careful when running `git add -A` in your repo; make sure `repo_path` points to the intended repository.
