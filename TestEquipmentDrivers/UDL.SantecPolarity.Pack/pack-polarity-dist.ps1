<#
.SYNOPSIS
  Build UDL.SantecPolarity (+ Cli + PolarityTestApp) and assemble ready-to-use dist for x64 and/or Win32 (x86).

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File .\pack-polarity-dist.ps1
  powershell -ExecutionPolicy Bypass -File .\pack-polarity-dist.ps1 -Configuration Release -Platform Both
  powershell -ExecutionPolicy Bypass -File .\pack-polarity-dist.ps1 -Platform x64 -SkipBuild
#>
[CmdletBinding()]
param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",

    [ValidateSet("x64", "Win32", "Both")]
    [string]$Platform = "Both",

    [string]$MsBuild = "",

    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$PackRoot = $PSScriptRoot
$DriversRoot = Split-Path $PackRoot -Parent
$RepoRoot = Split-Path $DriversRoot -Parent

function Find-MSBuild {
    param([string]$Hint)
    if ($Hint -and (Test-Path $Hint)) { return $Hint }
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $found = & $vswhere -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
        if ($found) { return $found }
    }
    foreach ($c in @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"
    )) {
        if (Test-Path $c) { return $c }
    }
    throw "MSBuild.exe not found. Pass -MsBuild path."
}

function Copy-FileChecked {
    param([string]$Src, [string]$DstDir)
    if (-not (Test-Path $Src)) { throw "Missing required file: $Src" }
    New-Item -ItemType Directory -Force -Path $DstDir | Out-Null
    Copy-Item -Force $Src (Join-Path $DstDir (Split-Path $Src -Leaf))
}

function Get-ArchLabel {
    param([string]$Plat)
    if ($Plat -eq "Win32") { return "x86" }
    return "x64"
}

function Invoke-PackOnePlatform {
    param(
        [string]$Plat,
        [string]$Configuration,
        [string]$MsBuildPath,
        [bool]$DoBuild
    )

    $arch = Get-ArchLabel $Plat
    $platformTarget = if ($Plat -eq "Win32") { "x86" } else { "x64" }
    $vcRedistLabel = if ($Plat -eq "Win32") { "x86" } else { "x64" }

    $DistName = "UDL.SantecPolarity-$Plat"
    $DistRoot = Join-Path $RepoRoot "dist\$DistName"
    $BinDir = Join-Path $DistRoot "bin"
    $IncludeDir = Join-Path $DistRoot "include"
    $DocsDir = Join-Path $DistRoot "docs"

    Write-Host ""
    Write-Host "======== Pack $Plat ($Configuration) ========"
    Write-Host "==> Dist: $DistRoot"

    if ($DoBuild) {
        $cliProj = Join-Path $DriversRoot "UDL.SantecPolarity.Cli\UDL.SantecPolarity.Cli.vcxproj"
        $natProj = Join-Path $DriversRoot "UDL.SantecPolarity\UDL.SantecPolarity.vcxproj"
        $appProj = Join-Path $DriversRoot "PolarityTestApp\PolarityTestApp.vcxproj"
        Write-Host "==> MSBuild: $MsBuildPath"
        & $MsBuildPath $cliProj /p:Configuration=$Configuration /p:Platform=$Plat /m /v:minimal
        if ($LASTEXITCODE -ne 0) { throw "Cli build failed ($Plat): $LASTEXITCODE" }
        & $MsBuildPath $natProj /p:Configuration=$Configuration /p:Platform=$Plat /m /v:minimal
        if ($LASTEXITCODE -ne 0) { throw "Native polarity build failed ($Plat): $LASTEXITCODE" }
        & $MsBuildPath $appProj /p:Configuration=$Configuration /p:Platform=$Plat /m /v:minimal
        if ($LASTEXITCODE -ne 0) { throw "PolarityTestApp build failed ($Plat): $LASTEXITCODE" }
    }

    # VS default: x64 -> <proj>\x64\<Config>\ ; Win32 -> <proj>\<Config>\
    if ($Plat -eq "x64") {
        $nativeOut = Join-Path $DriversRoot "UDL.SantecPolarity\x64\$Configuration"
        $cliOut = Join-Path $DriversRoot "UDL.SantecPolarity.Cli\x64\$Configuration"
        $appOut = Join-Path $DriversRoot "PolarityTestApp\x64\$Configuration"
    } else {
        $nativeOut = Join-Path $DriversRoot "UDL.SantecPolarity\$Configuration"
        $cliOut = Join-Path $DriversRoot "UDL.SantecPolarity.Cli\$Configuration"
        $appOut = Join-Path $DriversRoot "PolarityTestApp\$Configuration"
    }

    if (-not (Test-Path (Join-Path $nativeOut "UDL.SantecPolarity.dll"))) {
        throw "Native DLL not found under $nativeOut — build first or drop -SkipBuild"
    }
    if (-not (Test-Path (Join-Path $cliOut "UDL.SantecPolarity.Cli.dll"))) {
        throw "Cli DLL not found under $cliOut — build first or drop -SkipBuild"
    }
    if (-not (Test-Path (Join-Path $appOut "PolarityTestApp.exe"))) {
        throw "PolarityTestApp.exe not found under $appOut — build first or drop -SkipBuild"
    }

    $harnessProj = Join-Path $PackRoot "DepsHarness\DepsHarness.csproj"
    $harnessOut = Join-Path $PackRoot "DepsHarness\bin\$Configuration\$platformTarget"
    Write-Host "==> DepsHarness PlatformTarget=$platformTarget"
    dotnet restore $harnessProj | Out-Host
    dotnet build $harnessProj -c $Configuration -p:PlatformTarget=$platformTarget --no-restore | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "DepsHarness build failed ($platformTarget): $LASTEXITCODE" }

    # Prefer clean rebuild into a staging folder, then swap (avoids Explorer locks on docs/)
    $stageRoot = Join-Path $RepoRoot ("dist\.stage-UDL.SantecPolarity-$Plat")
    if (Test-Path $stageRoot) { Remove-Item -Recurse -Force $stageRoot -ErrorAction SilentlyContinue }
    $BinDir = Join-Path $stageRoot "bin"
    $IncludeDir = Join-Path $stageRoot "include"
    $DocsDir = Join-Path $stageRoot "docs"
    New-Item -ItemType Directory -Force -Path $BinDir, $IncludeDir, $DocsDir | Out-Null

    Copy-FileChecked (Join-Path $nativeOut "UDL.SantecPolarity.dll") $BinDir
    Copy-FileChecked (Join-Path $cliOut "UDL.SantecPolarity.Cli.dll") $BinDir
    Copy-FileChecked (Join-Path $appOut "PolarityTestApp.exe") $BinDir
    if (Test-Path (Join-Path $nativeOut "UDL.SantecPolarity.pdb")) {
        Copy-Item -Force (Join-Path $nativeOut "UDL.SantecPolarity.pdb") $BinDir
    }
    if (Test-Path (Join-Path $cliOut "UDL.SantecPolarity.Cli.pdb")) {
        Copy-Item -Force (Join-Path $cliOut "UDL.SantecPolarity.Cli.pdb") $BinDir
    }
    if (Test-Path (Join-Path $appOut "PolarityTestApp.pdb")) {
        Copy-Item -Force (Join-Path $appOut "PolarityTestApp.pdb") $BinDir
    }

    $libs = Join-Path $DriversRoot "UDL.SantecPolarity.Cli\libs"
    if ($Plat -eq "x64") {
        Copy-FileChecked (Join-Path $libs "x64\Santec.Hardware.dll") $BinDir
    } else {
        Copy-FileChecked (Join-Path $libs "x86\Santec.Hardware.dll") $BinDir
    }
    Copy-FileChecked (Join-Path $libs "common\Santec.Core.dll") $BinDir
    if (Test-Path (Join-Path $libs "common\TIS.Imaging.ICImagingControl35.dll")) {
        Copy-Item -Force (Join-Path $libs "common\TIS.Imaging.ICImagingControl35.dll") $BinDir
    }

    Get-ChildItem $harnessOut -File | Where-Object {
        $_.Extension -match '^\.(dll|pdb)$' -and $_.Name -notmatch '^DepsHarness\.'
    } | ForEach-Object {
        Copy-Item -Force $_.FullName $BinDir
    }

    $opencvRid = if ($Plat -eq "Win32") { "win-x86" } else { "win-x64" }
    $opencvNative = Join-Path $env:USERPROFILE ".nuget\packages\opencvsharp4.runtime.win\4.6.0.20220608\runtimes\$opencvRid\native"
    if (Test-Path $opencvNative) {
        Get-ChildItem $opencvNative -File | ForEach-Object {
            Copy-Item -Force $_.FullName $BinDir
        }
    }

    if (Test-Path (Join-Path $cliOut "Ivi.Visa.dll")) {
        Copy-Item -Force (Join-Path $cliOut "Ivi.Visa.dll") $BinDir
    }

    Copy-FileChecked (Join-Path $DriversRoot "UDL.SantecPolarity\SPTypes.h") $IncludeDir
    Copy-FileChecked (Join-Path $DriversRoot "UDL.SantecPolarity\SPExports.h") $IncludeDir

    $config = @"
<?xml version="1.0" encoding="utf-8"?>
<!-- Merge these redirects into YourHost.exe.config (same folder as host EXE). -->
<configuration>
  <runtime>
    <assemblyBinding xmlns="urn:schemas-microsoft-com:asm.v1">
      <dependentAssembly>
        <assemblyIdentity name="System.Text.Json" publicKeyToken="cc7b13ffcd2ddd51" culture="neutral" />
        <bindingRedirect oldVersion="0.0.0.0-10.0.0.0" newVersion="5.0.0.2" />
      </dependentAssembly>
      <dependentAssembly>
        <assemblyIdentity name="System.Runtime.CompilerServices.Unsafe" publicKeyToken="b03f5f7f11d50a3a" culture="neutral" />
        <bindingRedirect oldVersion="0.0.0.0-6.0.0.0" newVersion="6.0.0.0" />
      </dependentAssembly>
      <dependentAssembly>
        <assemblyIdentity name="System.Memory" publicKeyToken="cc7b13ffcd2ddd51" culture="neutral" />
        <bindingRedirect oldVersion="0.0.0.0-4.0.1.2" newVersion="4.0.1.2" />
      </dependentAssembly>
      <dependentAssembly>
        <assemblyIdentity name="System.Buffers" publicKeyToken="cc7b13ffcd2ddd51" culture="neutral" />
        <bindingRedirect oldVersion="0.0.0.0-4.0.3.0" newVersion="4.0.3.0" />
      </dependentAssembly>
      <dependentAssembly>
        <assemblyIdentity name="Zeroconf" publicKeyToken="dbbfcdb374507df0" culture="neutral" />
        <bindingRedirect oldVersion="0.0.0.0-3.7.16.0" newVersion="3.7.0.0" />
      </dependentAssembly>
      <dependentAssembly>
        <assemblyIdentity name="TIS.Imaging.ICImagingControl35" publicKeyToken="257805929e8b7928" culture="neutral" />
        <bindingRedirect oldVersion="0.0.0.0-3.5.7.4039" newVersion="3.5.6.4008" />
      </dependentAssembly>
    </assemblyBinding>
  </runtime>
</configuration>
"@
    Set-Content -Path (Join-Path $BinDir "Host.exe.config.sample") -Value $config -Encoding UTF8
    Copy-Item -Force (Join-Path $BinDir "Host.exe.config.sample") (Join-Path $BinDir "UDL.SantecPolarity.Cli.dll.config")
    # PolarityTestApp is the host EXE in this package — give it the same redirects
    Copy-Item -Force (Join-Path $BinDir "Host.exe.config.sample") (Join-Path $BinDir "PolarityTestApp.exe.config")

    foreach ($docName in @(
        "UDL.SantecPolarity-API.md",
        "UDL.SantecPolarity-SDK交付说明.md"
    )) {
        $docSrc = Join-Path $RepoRoot "docs\$docName"
        if (Test-Path $docSrc) { Copy-Item -Force $docSrc $DocsDir }
    }

    $readme = @"
# UDL.SantecPolarity 业务交付包 ($Plat / $arch / $Configuration)

打开即用：将 ``bin`` 目录全部文件放在一起（勿拆散）。可用 ``PolarityTestApp.exe`` 直接测驱动，或业务 Host ``LoadLibrary``。

## 目录

| 路径 | 说明 |
|---|---|
| ``bin/PolarityTestApp.exe`` | MFC 调试工具（动态加载 ``UDL.SantecPolarity.dll`` / ``SP_*``） |
| ``bin/PolarityTestApp.exe.config`` | 已含 bindingRedirect（勿删） |
| ``bin/`` | 驱动与全部运行时依赖 |
| ``include/`` | ``SPTypes.h`` / ``SPExports.h``（C ABI） |
| ``docs/`` | 接口说明 |

## 用 PolarityTestApp 冒烟

1. 双击 ``bin/PolarityTestApp.exe``（必须在 ``bin`` 目录启动，或把工作目录设为 ``bin``）。
2. DLL 路径默认 ``UDL.SantecPolarity.dll`` → **Load** → **Create** → 填 RLM SN（建议）→ **Open**。
3. **Snapshot** 看硬件；**Run Mapping** 跑极性；**Get Results** 看纤芯表。
4. ``SP_RunMapping`` 会阻塞 UI 线程直到结束（调试可接受；产线 Host 请用工作线程）。

## 必备运行环境（业务机）

1. Windows 10/11（本包平台：**$Plat / $arch**）
2. **.NET Framework 4.8**
3. **VC++ 2015-2022 $vcRedistLabel 运行库**（[Microsoft VC++ Redistributable](https://learn.microsoft.com/cpp/windows/latest-supported-vc-redist)）
4. RD-SP 相机驱动 / Imaging Control（首次接真机按 Santec 说明安装）
5. 若使用 USB VISA：安装 NI-VISA

## Host 集成要点

1. 只 ``LoadLibrary("UDL.SantecPolarity.dll")``，**不要**直接加载 Cli / Hardware。
2. ``bin`` 内所有 DLL 必须与 ``UDL.SantecPolarity.dll`` **同目录**。
3. 将 ``bin/Host.exe.config.sample`` 中的 ``assemblyBinding`` **合并**进 ``YourApp.exe.config``。
4. ``SP_RunMapping`` 必须在**工作线程**调用（产线 Host）。
5. 与 ``UDL.SantecRLM`` 同时使用时：同一 RLM 串号互斥（``Global\UDL_SANTEC_RLM100_<serial>``）。

## 最小调用顺序

``SP_Create`` → ``SP_Open`` → ``SP_RunMapping`` → ``SP_GetSummary`` / ``SP_GetFiberResults`` → ``SP_Close`` → ``SP_Destroy``

## 打包信息

- 生成时间: $(Get-Date -Format "yyyy-MM-dd HH:mm:ss")
- Configuration: $Configuration
- Platform: $Plat ($arch)
- Santec.Hardware: 2.3.1.0（钉死）
- 含 PolarityTestApp（SP_*）
"@
    Set-Content -Path (Join-Path $stageRoot "README.md") -Value $readme -Encoding UTF8

    $files = Get-ChildItem $BinDir -File | Sort-Object Name
    $manifest = @("# bin manifest ($Plat)", "")
    foreach ($f in $files) {
        $hash = (Get-FileHash -Algorithm SHA256 $f.FullName).Hash
        $manifest += ("{0,-48} {1,12}  SHA256={2}" -f $f.Name, $f.Length, $hash)
    }
    Set-Content -Path (Join-Path $stageRoot "MANIFEST.txt") -Value ($manifest -join "`r`n") -Encoding UTF8

    $required = @(
        "UDL.SantecPolarity.dll",
        "UDL.SantecPolarity.Cli.dll",
        "PolarityTestApp.exe",
        "PolarityTestApp.exe.config",
        "Santec.Hardware.dll",
        "Santec.Core.dll",
        "OpenCvSharp.dll",
        "OpenCvSharpExtern.dll",
        "Zeroconf.dll",
        "TIS.Imaging.ICImagingControl35.dll",
        "Nito.AsyncEx.Context.dll",
        "NGettext.dll",
        "System.Text.Json.dll"
    )
    $missing = @()
    foreach ($r in $required) {
        if (-not (Test-Path (Join-Path $BinDir $r))) { $missing += $r }
    }
    if ($missing.Count -gt 0) {
        throw ("Dist incomplete ($Plat), missing: " + ($missing -join ", "))
    }

    # Promote staging -> DistRoot (best-effort replace if locked)
    if (Test-Path $DistRoot) {
        $bak = "$DistRoot.bak"
        if (Test-Path $bak) { Remove-Item -Recurse -Force $bak -ErrorAction SilentlyContinue }
        try {
            Rename-Item -LiteralPath $DistRoot -NewName (Split-Path $bak -Leaf) -ErrorAction Stop
        } catch {
            Write-Warning "Could not rename old dist ($Plat); overlaying files instead. Close Explorer on dist if possible."
        }
    }
    if (-not (Test-Path $DistRoot)) {
        Move-Item -LiteralPath $stageRoot -Destination $DistRoot
    } else {
        Copy-Item -Recurse -Force (Join-Path $stageRoot "*") $DistRoot
        Remove-Item -Recurse -Force $stageRoot -ErrorAction SilentlyContinue
    }

    $zipPath = "$DistRoot.zip"
    if (Test-Path $zipPath) { Remove-Item -Force $zipPath }
    Compress-Archive -Path (Join-Path $DistRoot "*") -DestinationPath $zipPath -Force

    Write-Host "OK: dist ready -> $DistRoot"
    Write-Host ("bin files: {0}" -f $files.Count)
    Write-Host "OK: zip ready -> $zipPath ($((Get-Item $zipPath).Length) bytes)"

    return @{ DistRoot = $DistRoot; ZipPath = $zipPath; FileCount = $files.Count }
}

# ---- main ----
$platforms = @()
if ($Platform -eq "Both") {
    $platforms = @("x64", "Win32")
} else {
    $platforms = @($Platform)
}

$msb = $null
if (-not $SkipBuild) {
    $msb = Find-MSBuild -Hint $MsBuild
}

$results = @()
foreach ($p in $platforms) {
    $results += Invoke-PackOnePlatform -Plat $p -Configuration $Configuration -MsBuildPath $msb -DoBuild (-not $SkipBuild)
}

if ($platforms.Count -gt 1) {
    $bundleRoot = Join-Path $RepoRoot "dist\UDL.SantecPolarity-Both"
    if (Test-Path $bundleRoot) { Remove-Item -Recurse -Force $bundleRoot }
    New-Item -ItemType Directory -Force -Path $bundleRoot | Out-Null
    foreach ($p in $platforms) {
        $src = Join-Path $RepoRoot "dist\UDL.SantecPolarity-$p"
        Copy-Item -Recurse -Force $src (Join-Path $bundleRoot $p)
    }
    $bundleReadme = @"
# UDL.SantecPolarity 双平台包

| 子目录 | 平台 |
|---|---|
| ``x64/`` | 64-bit Host / PolarityTestApp |
| ``Win32/`` | 32-bit (x86) Host / PolarityTestApp |

每个子目录内结构相同：``bin``（含 ``PolarityTestApp.exe``）/ ``include`` / ``docs``。
位数必须与业务 Host 一致，勿混用。
"@
    Set-Content -Path (Join-Path $bundleRoot "README.md") -Value $bundleReadme -Encoding UTF8
    $bundleZip = "$bundleRoot.zip"
    if (Test-Path $bundleZip) { Remove-Item -Force $bundleZip }
    Compress-Archive -Path (Join-Path $bundleRoot "*") -DestinationPath $bundleZip -Force
    Write-Host ""
    Write-Host "OK: dual-platform bundle -> $bundleRoot"
    Write-Host "OK: dual-platform zip -> $bundleZip ($((Get-Item $bundleZip).Length) bytes)"
}

Write-Host ""
Write-Host "All done."
