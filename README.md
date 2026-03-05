# c-webserv

Simple HTTP webserver built in C for Windows.

The server serves static files from the `www/` directory.
Example: `www/healthyfitness` is available at `http://localhost:8080/healthyfitness`.

## Project layout

- `src/` - C source files
- `include/` - Header files
- `build/` - PowerShell build scripts
- `bin/` - Compiled output (`webserv.exe`)
- `www/` - Static web root

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

## Static files (`www`)

- URL path `/` maps to `www/index.html`
- URL path `/healthyfitness` maps to `www/healthyfitness`
- If a URL maps to a directory, the server tries `index.html` in that directory
- Requests outside `www` (for example via `..`) are blocked

## Clean

```powershell
./build/clean.ps1
```
