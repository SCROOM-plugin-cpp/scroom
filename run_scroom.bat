@echo off
:: Powershell doesn't have a nice way to run as admin, which we need to
:: add to the systems path. So this bat file is just here to:
::   * Set the execution policy
::   * Allow you to run it using right click > Run as Admin

powershell –ExecutionPolicy Bypass -File setup\prep_windows.ps1
