# c-webserv

Simple HTTP webserver built in C for Windows.

## Project layout

- `src/` - C source files
- `include/` - Header files
- `build/` - PowerShell build scripts
- `bin/` - Compiled output (`webserv.exe`)

## Build (PowerShell)

From the project root:

```powershell
./build/build.ps1
```

Debug build:

```powershell
./build/build.ps1 -Configuration Debug
```

Custom compiler command:

```powershell
./build/build.ps1 -Compiler gcc
```

The build output is written to:

```text
bin/webserv.exe
```

## Run

Default port (`8080`):

```powershell
./bin/webserv.exe
```

Custom port:

```powershell
./bin/webserv.exe 9090
```

Then open `http://localhost:8080` (or your chosen port) in a browser.

## Clean

```powershell
./build/clean.ps1
```
