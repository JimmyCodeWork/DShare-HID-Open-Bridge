@echo off
setlocal

:ProcessArgs
if "%~1" neq "" (
    call :ProcessInput "%~1"
    exit /b %ERRORLEVEL%
)

:Interactive

echo ==================================================
echo            DShare-HID Interactive Build
echo ==================================================
echo   1^) Build (Compile Only)
echo   2^) Configure Pristine (Clean ^& Reconfigure)
echo   3^) Launch
echo   4^) Configure Release (Clean ^& Config with Production Keys)
echo   5^) Build Deploy (Configure Release ^& Package)
echo   6^) Install Prebuilt Qt ^& OpenSSL (aqtinstall)
echo   q^) Quit
echo.
set "input="
set /p input="Select a task (1-6 or q): "
if not defined input goto Interactive

if /i "%input%"=="q" exit /b 0
if /i "%input%"=="quit" exit /b 0
if /i "%input%"=="exit" exit /b 0

call :ProcessInput "%input%"

goto Interactive

:ProcessInput
set choice=%~1
if "%choice%"=="1" ( call :Build & exit /b 0 )
if "%choice%"=="build" ( call :Build & exit /b 0 )
if "%choice%"=="2" ( call :Pristine & exit /b 0 )
if "%choice%"=="build pristine" ( call :Pristine & exit /b 0 )
if "%choice%"=="3" ( call :Launch & exit /b 0 )
if "%choice%"=="launch" ( call :Launch & exit /b 0 )
if "%choice%"=="4" ( call :Release & exit /b 0 )
if "%choice%"=="release" ( call :Release & exit /b 0 )
if "%choice%"=="5" ( call :Deploy & exit /b 0 )
if "%choice%"=="build deploy" ( call :Deploy & exit /b 0 )
if "%choice%"=="6" ( call :SetupQt & exit /b )
if "%choice%"=="setup-qt" ( call :SetupQt & exit /b )
echo Invalid selection: %choice%
exit /b 1

:Build
call :SetupEnv
if not exist build (
    echo Error: 'build' directory not found. Please run a configuration step (2 or 4^) first.
    exit /b 1
)
echo --- BUILDING ---
cmake --build build -j%NUMBER_OF_PROCESSORS%
exit /b %ERRORLEVEL%

:Pristine
call :SetupEnv
call :SetupDeps || exit /b 1
call :SetupCompilerCache
echo --- CLEANING AND RECONFIGURING (PRISTINE) ---
if exist build rd /s /q build
call :Configure
echo Configuration complete. Run option 1 to build.
exit /b 0

:Release
call :SetupEnv
call :SetupDeps || exit /b 1
call :SetupCompilerCache
echo --- CLEANING AND RECONFIGURING (RELEASE) ---
if exist build rd /s /q build
echo --- CONFIGURING ---
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release %DEPS_ARGS% %CACHE_ARGS% -DBUILD_TESTS=OFF -DDESKFLOW_PAYPAL_ACCOUNT="%PAYPAL_ACCOUNT%" -DDESKFLOW_PAYPAL_URL="%PAYPAL_URL%" -DDESKFLOW_CDC_PUBLIC_KEY="%DESKFLOW_CDC_PUBLIC_KEY%" -DDESKFLOW_ESP32_ENCRYPTION_KEY="%DESKFLOW_ESP32_ENCRYPTION_KEY%"
echo Release Configuration complete. Run option 1 to build.
exit /b %ERRORLEVEL%

:Deploy
call :Release
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
call :Build
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
echo --- PACKAGING ---
cmake --build build --target package
if %ERRORLEVEL% neq 0 exit /b %ERRORLEVEL%
rem Final packages (MSI, portable ZIP) are collected in outputs
if not exist outputs mkdir outputs
for %%F in (build\*.msi build\*.zip) do (
    move /y "%%F" outputs\ >nul
    echo Package: outputs\%%~nxF
)
exit /b 0

:Launch
if not exist build\bin\dshare-hid.exe (
    echo Error: Application not built. Select '1' to build first.
    exit /b 1
)
echo --- LAUNCHING DSHARE-HID ---
build\bin\dshare-hid.exe
exit /b 0

:Configure
echo --- CONFIGURING ---
cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release %DEPS_ARGS% %CACHE_ARGS% -DBUILD_TESTS=OFF -DDESKFLOW_PAYPAL_ACCOUNT="%PAYPAL_ACCOUNT%" -DDESKFLOW_PAYPAL_URL="%PAYPAL_URL%"
exit /b %ERRORLEVEL%

:SetupEnv
where cl >nul 2>nul && exit /b 0
set "_SAVED_VCPKG_ROOT=%VCPKG_ROOT%"
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
if defined _SAVED_VCPKG_ROOT set "VCPKG_ROOT=%_SAVED_VCPKG_ROOT%"
exit /b 0

:ReadQtVersion
set "QT_VERSION="
for /f "usebackq delims=" %%V in ("%~dp0cmake\qt-version.txt") do if not defined QT_VERSION set "QT_VERSION=%%V"
if not defined QT_VERSION (
    echo Error: cmake\qt-version.txt is missing or empty.
    exit /b 1
)
exit /b 0

:SetupDeps
rem Prebuilt Qt and OpenSSL when QT_ROOT is set, otherwise the legacy vcpkg build.
call :ReadQtVersion || exit /b 1
if not defined QT_ROOT goto SetupDepsVcpkg
set "QT_DIR=%QT_ROOT%\%QT_VERSION%\msvc2022_64"
set "OPENSSL_DIR=%QT_ROOT%\Tools\OpenSSLv3\Win_x64"
if not exist "%QT_DIR%\lib\cmake\Qt6\Qt6Config.cmake" (
    echo Error: Qt %QT_VERSION% was not found in %QT_DIR%.
    echo Run "run_task.bat setup-qt" to install it.
    exit /b 1
)
if not exist "%OPENSSL_DIR%\include\openssl\ssl.h" (
    echo Error: OpenSSL 3 was not found in %OPENSSL_DIR%.
    echo Run "run_task.bat setup-qt" to install it.
    exit /b 1
)
set "DEPS_ARGS=-DCMAKE_PREFIX_PATH="%QT_DIR:\=/%" -DOPENSSL_ROOT_DIR="%OPENSSL_DIR:\=/%""
echo Using prebuilt Qt %QT_VERSION% from %QT_DIR%
exit /b 0

:SetupDepsVcpkg
if not defined VCPKG_ROOT (
    echo Error: set QT_ROOT to use prebuilt Qt and OpenSSL ^(see "run_task.bat setup-qt"^),
    echo or set VCPKG_ROOT to build the dependencies with vcpkg.
    exit /b 1
)
set "DEPS_ARGS=-DCMAKE_TOOLCHAIN_FILE=%VCPKG_ROOT:\=/%/scripts/buildsystems/vcpkg.cmake -DVCPKG_MANIFEST_MODE=ON -DVCPKG_TARGET_TRIPLET=x64-windows -DVCPKG_QT=ON"
echo Using vcpkg from %VCPKG_ROOT% ^(QT_ROOT is not set^)
exit /b 0

:SetupCompilerCache
rem Use sccache as the compiler launcher when it is installed, so rebuilding after a clean
rem configure (options 2, 4 and 5) reuses earlier compile results. Set USE_SCCACHE=0 to disable.
set "CACHE_ARGS="
if "%USE_SCCACHE%"=="0" (
    echo Compiler cache: disabled ^(USE_SCCACHE=0^)
    exit /b 0
)
where sccache >nul 2>nul
if errorlevel 1 (
    echo Compiler cache: sccache not found; install it with "winget install Mozilla.sccache" to speed up rebuilds.
    exit /b 0
)
set "CACHE_ARGS=-DCMAKE_C_COMPILER_LAUNCHER=sccache -DCMAKE_CXX_COMPILER_LAUNCHER=sccache"
echo Compiler cache: sccache
exit /b 0

:SetupQt
rem Installs the pinned Qt version and Qt's OpenSSL 3 package with aqtinstall.
call :ReadQtVersion || exit /b 1
if not defined QT_ROOT set "QT_ROOT=C:\Qt"
where python >nul 2>nul || (
    echo Error: Python 3 is required to run aqtinstall.
    exit /b 1
)
python -m aqt version >nul 2>nul
if errorlevel 1 (
    echo --- INSTALLING aqtinstall ---
    python -m pip install --user aqtinstall || exit /b 1
)
echo --- INSTALLING Qt %QT_VERSION% INTO %QT_ROOT% ---
python -m aqt install-qt windows desktop %QT_VERSION% win64_msvc2022_64 -O "%QT_ROOT%" || exit /b 1
echo --- INSTALLING OpenSSL 3 INTO %QT_ROOT%\Tools ---
python -m aqt install-tool windows desktop tools_opensslv3_x64 -O "%QT_ROOT%" || exit /b 1
echo.
echo Qt %QT_VERSION% and OpenSSL 3 are installed in %QT_ROOT%.
echo To use them in new terminals, run once:  setx QT_ROOT "%QT_ROOT%"
echo Then run option 2 ^(Configure Pristine^).
exit /b 0
