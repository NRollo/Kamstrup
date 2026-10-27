# Building on Windows

## Symptom

The first CMake configure step fails with:

```
ld.exe: cannot open output file cmTC_xxxxx: No such file or directory
The C compiler ... is not able to compile a simple test program.
```

and files inside `.pio` can sometimes not be deleted from a terminal ("Access is denied").

## Cause

Windows Defender **Controlled Folder Access** (Windows Security > Virus & threat protection >
Ransomware protection) blocks programs it does not trust from writing to protected folders,
which include `Documents`. A newly installed PlatformIO toolchain has new binaries
(`ld.exe`, `as-*.exe`) that are not trusted yet, so the build directory inside the project
under `Documents` cannot be written.

To confirm, look for Event ID 1123 in Event Viewer under
Applications and Services Logs > Microsoft > Windows > Windows Defender > Operational.

## Fix 1: allow the toolchain (permanent)

Run once in an **administrator** PowerShell, adjusting the toolchain folder name to the version
shown in the blocked-event path:

```powershell
$tc = "$env:USERPROFILE\.platformio\packages\toolchain-riscv32-esp\riscv32-esp-elf\bin"
Get-ChildItem $tc -Filter *.exe | ForEach-Object { Add-MpPreference -ControlledFolderAccessAllowedApplications $_.FullName }
```

Repeat after PlatformIO installs a new toolchain version.

## Fix 2: build outside Documents (works without admin rights)

`BUILD_WINDOWS.bat` and the VS Code tasks in `.vscode/tasks.json` set
`PLATFORMIO_BUILD_DIR=C:\tmp\pio_build`, which is not protected.

- Build: `BUILD_WINDOWS.bat`, or `Ctrl+Shift+B` in VS Code
- Upload: `BUILD_WINDOWS.bat -t upload`, or the VS Code task "PlatformIO: Upload (Windows Workaround)"
- Output: `C:\tmp\pio_build\supermini_esp32c3\firmware.bin`

Incremental builds take a few seconds; a clean build takes about two minutes.
