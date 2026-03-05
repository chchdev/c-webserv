# c-webserv

Simple HTTP webserver built in C for Windows.

The server serves static files from the `www/` directory.
Example: `www/healthyfitness` is available at `http://localhost:8080/healthyfitness`.

## Project layout

- `src/` - C source files
- `include/` - Header files
- `build/` - PowerShell build scripts
- `bin/` - Compiled output (`webserv.exe`, `webserv-gui.exe`)
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
bin/webserv-gui.exe
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

## GUI control panel

You can run a Windows GUI to start/stop the server and set the port:

```powershell
./bin/webserv-gui.exe
```

Behavior:

- `Start` launches `bin/webserv.exe` on the selected port
- `Stop` requests graceful shutdown and waits for clean exit
- You can change the port while stopped, then start again
- App icon is embedded in `webserv-gui.exe` from `assets/app.ico`

The GUI starts the server with an internal shutdown token and calls `GET /__shutdown`
with `X-Shutdown-Token` when stopping.

## Static files (`www`)

- URL path `/` maps to `www/index.html`
- URL path `/healthyfitness` maps to `www/healthyfitness`
- If a URL maps to a directory, the server tries `index.html` in that directory
- Requests outside `www` (for example via `..`) are blocked

## Clean

```powershell
./build/clean.ps1
```
