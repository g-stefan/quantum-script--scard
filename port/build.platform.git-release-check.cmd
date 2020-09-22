@echo off
rem Public domain
rem http://unlicense.org/
rem Created by Grigore Stefan <g_stefan@yahoo.com>

SETLOCAL ENABLEDELAYEDEXPANSION

echo -^> git-release-check quantum-script-extension-scard

set PROJECT=quantum-script-extension-scard
if not exist source\%PROJECT%.version.ini echo Error - no version & exit 1
FOR /F "tokens=* USEBACKQ" %%F IN (`xyo-version --no-bump --get "--version-file=source\%PROJECT%.version.ini" %PROJECT%`) DO (
	SET VERSION=%%F
)

github-release info --repo %PROJECT% --tag v%VERSION%

