@echo off
chcp 65001 >nul
setlocal
rem Visual Studioのx64 Native Tools Command Promptから実行する。
rem 先にDevelop構成でエンジンをビルドし、DLLとインポートライブラリを用意する。
pushd "%~dp0..\.."
if not exist generated\tests mkdir generated\tests
cl /nologo /std:c++20 /EHsc /MT /utf-8 /DNOMINMAX /I. /Iexternals Tools\Tests\TextureSheetAnimationTests.cpp /Fogenerated\tests\TextureSheetAnimationTests.obj /Fegenerated\tests\TextureSheetAnimationTests.exe /link ..\generated\outputs\Develop\CalyxEngine.lib
if errorlevel 1 goto failed
rem エンジンDLLの配置先を検索パスに追加し、描画環境を起動せず再生処理を検証する。
set "PATH=%CD%\..\generated\outputs\Develop;%CD%\externals\assimp\bin\Release;%PATH%"
generated\tests\TextureSheetAnimationTests.exe
if errorlevel 1 goto failed
popd
exit /b 0
:failed
popd
exit /b 1
