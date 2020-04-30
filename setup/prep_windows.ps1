$minggw_path = "C:\msys64\mingw64\bin"
$glade_file = ".\gui\scroom.glade"
$glade_dest = "C:\msys64\usr\share\scroom\"
$scroom_dir = "C:\msys64\usr\lib\scroom"
$scroom_exe = ".\gui\src\scroom.exe"

# First add mingWG64 to the path

if ($env:PATH -like "*$minggw_path*") {
    Write-Host "MinGW found in path"
} else {
    Write-Host $env:PATH
    Write-Host "Adding MinGW to system path"
    setx -m PATH "$mingw_path;$env:PATH"
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine")
}

# Next install the glade file
New-Item -Type dir $glade_dest -ErrorAction 'silentlycontinue'
cp $glade_file $glade_dest\scroom.glade -Force
Write-Host "Copied the glade file"

# Ensure the scroom dir exists
New-Item -Type dir $scroom_dir -ErrorAction 'silentlycontinue'

# Launch scroom
cmd /c $scroom_exe

