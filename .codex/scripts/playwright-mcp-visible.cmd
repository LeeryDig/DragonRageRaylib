@echo off
setlocal EnableExtensions

for %%A in (%*) do (
  if /I "%%~A"=="--headless" (
    >&2 echo ERROR: headless mode is blocked in this repository. QA must keep Chrome visible during Playwright runs.
    exit /b 1
  )
)

npx -y @playwright/mcp@latest --browser chrome --output-dir qa_playwright %*
