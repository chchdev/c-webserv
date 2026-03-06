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

Build installer (requires Inno Setup with `iscc` on `PATH`):

```powershell
./build/build-installer.ps1
```

The build output is written to:

```text
bin/webserv.exe
bin/webserv-gui.exe
bin/installer/c-webserv-setup.exe
```

## Installer

- Installer script: `installer/c-webserv.iss`
- Default install directory: `C:\Program Files\c-webserv\`
- Installed runtime layout includes:
	- `bin/webserv.exe`
	- `bin/webserv-gui.exe`
	- `www/` (created by default and populated from project `www/`)

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
- Server runs in the background when started from the GUI (no CMD window)

The GUI starts the server with an internal shutdown token and calls `GET /__shutdown`
with `X-Shutdown-Token` when stopping.

## Static files (`www`)

- URL path `/` maps to `www/index.html`
- URL path `/healthyfitness` maps to `www/healthyfitness`
- If a URL maps to a directory, the server tries `index.html` in that directory
- Requests outside `www` (for example via `..`) are blocked

## PHP files

- `.php` files in `www/` are executed through `php-cgi`
- Example: `www/index.php` is available at `http://localhost:8080/index.php`
- Query strings are forwarded (for example `/index.php?name=ben`)

Requirement:

- `php-cgi` (or `php-cgi.exe`) must be installed and available on your `PATH`

## Clean

```powershell
./build/clean.ps1
```
