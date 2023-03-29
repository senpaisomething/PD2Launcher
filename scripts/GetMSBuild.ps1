# Determine visual studio installation, you can add more folders here if you are using other version
$msBuildLocation = ""

$buildLocations = (
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe",
    "D:\Program Files\Microsoft Visual Studio\2019\Community\MSBuild\Current\Bin\MSBuild.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Preview\Msbuild\Current\Bin\MSBuild.exe",
    "C:\Program Files\Microsoft Visual Studio\2022\Community\Msbuild\Current\Bin\MSBuild.exe"
)

foreach ($location in $buildLocations) {
    if (Test-Path -Path $location) {
        $msBuildLocation = $location
    } 
}

if (!(Test-Path -Path $msBuildLocation)) {
    throw "Couldn't find msbuild! Make sure you have visual studio installed"
}

return $msBuildLocation
