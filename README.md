# Project Diablo 2 - Launcher

## Install Instructions
1. Clone the repo
2. Change directory into the repo and run the following commands:
    - `git submodule init`
    - `git submodule update`
3. Open the project in Visual Studio and run it.

## Setting up Debug and Build output Folders

Set the following environment variables according to your setup, change the folder to your Diablo II folder. This can either be done in the environment variable tab, or open up powershell as administrator and run the following commands to set it system wide:

```
[System.Environment]::SetEnvironmentVariable('DIABLO_DEBUG_FOLDER','C:\Program Files (x86)\Diablo II\ProjectD2',[System.EnvironmentVariableTarget]::Machine)
```
4. Open the project in Visual Studio and run it.
