## Obs plugin for multi-stream (rtmp outputs)

First oficial version published!

🍎 How to install:

### Windows

1. Download the source code (.zip)
2. Extract the (.zip) "multi-stream-ms folder" into
```bash
C:\ProgramData\obs-studio\plugins\
```
3. Verify the path, must be
```bash
C:\ProgramData\obs-studio\plugins\multi-stream-ms\bin\64bit\multi-stream-ms.dll
```
4. Open Obs or Restart it.

### Linux

1. Require Flatpak Obs
flatpak install flathub com.obsproject.Studio
2. Download the source code (.flatpak)
3. Open the terminal (into the download path) and run
```bash
flatpak install --user ./multi-stream-ms-0.1.0.flatpak
```
4. Verify the instalation
```bash
flatpak list | grep MultiStreamMS
```
5. Open Obs or restar it

🫕 How to uninstall:

### windows

1. Close Obs
2. Delete multi-stream-ms folder from this path
```bash
C:\ProgramData\obs-studio\plugins\
```

### Linux

1. Run
```bash
flatpak uninstall com.obsproject.Studio.Plugin.MultiStreamMS
```
