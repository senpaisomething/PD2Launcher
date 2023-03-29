# Requires visual studio installed
[CmdletBinding()]
Param(
    # Output folder location
    [string]
    $OutputLocation = "../bin",

    # Build release or debug
    [string]
    $Configuration = "Release",

    [switch]
    $IsBeta,

    [switch]
    $PushToBucket
)


$location = "Live"
if ($PushToBucket) {
    if($IsBeta){
        $location = "Beta"
    }

    $title = 'Push Live'
    $msg = "You are about to push the '$location' Launcher, are you sure? This will update the live launcher"
    $options = '&Yes', '&No'
    $default = 1  # 0=Yes, 1=No

    $response = $Host.UI.PromptForChoice($title, $msg, $options, $default)
    if ($response -eq 0) {
    }
    else {
        exit
    }
}

Push-Location $PSScriptRoot
$msBuildLocation = .\GetMSBuild.ps1
$outputFolder = "$PSScriptRoot/$OutputLocation/"

$launcherBucket = ""
$betaOption = ""

if($IsBeta){
    $outputFolder += "beta/"
    $launcherBucket = "pd2-beta-launcher-update"
    $betaOption = "/DBETA_LAUNCHER"
} else{
    $outputFolder += "live/"
    $launcherBucket = "pd2-launcher-update"
}

try{
    if(Test-Path -Path $outputFolder){
        Remove-Item -Path $outputFolder -Force -Recurse
    }

    # Build Launcher
    $projectFolder = "$PSScriptRoot/../src/launcher/"
    Push-Location $projectFolder
    $env:ExternalCompilerOptions = $betaOption
    &$msBuildLocation PD2Launcher.vcxproj /m /p:Configuration=$Configuration /t:rebuild /p:OutDir="$outputFolder"
    $env:ExternalCompilerOptions = ""
    Pop-Location

    # Build Updater
    $projectFolder = "$PSScriptRoot/../src/updater/"
    Push-Location $projectFolder
    $env:ExternalCompilerOptions = $betaOption
    &$msBuildLocation updater.vcxproj /m /p:Configuration=$Configuration /t:rebuild /p:OutDir="$outputFolder"
    $env:ExternalCompilerOptions = ""
    Pop-Location

    # Copy Sciter dll
    Copy-Item -Path "$PSScriptRoot/../thirdparty/sciter-sdk/bin.win/x32/sciter.dll" -Destination $outputFolder -Force

    # Update the live bucket
    if($PushToBucket){
        Write-Host "Pushing $location Launcher to $launcherBucket"
        gsutil cp "$outputFolder/PD2Launcher.exe" "gs://$launcherBucket/PD2Launcher.exe"
        gsutil cp "$outputFolder/sciter.dll" "gs://$launcherBucket/sciter.dll"
        gsutil cp "$outputFolder/updater.exe" "gs://$launcherBucket/updater.exe"
    }
} finally{
    $env:ExternalCompilerOptions = ""
    Pop-Location
}
