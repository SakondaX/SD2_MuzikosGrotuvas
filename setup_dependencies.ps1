param([ValidateSet('Release','Debug')][string]$Configuration='Release')
$ErrorActionPreference='Stop'
$env:MSBUILDDISABLENODEREUSE='1'
$root=Join-Path $PSScriptRoot 'deps'
function Get-Sha256($path)
{
    $algorithm=[Security.Cryptography.SHA256]::Create()
    $stream=[IO.File]::OpenRead($path)
    try
    {
        return [BitConverter]::ToString($algorithm.ComputeHash($stream)).Replace('-','')
    }
    finally
    {
        $stream.Dispose()
        $algorithm.Dispose()
    }
}
function Get-JsonFile($relativePath,$url,$hash)
{
    $target=Join-Path $root "json\$relativePath"
    if(-not (Test-Path -LiteralPath $target))
    {
        New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
        [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
        $download="$target.download"
        Invoke-WebRequest -Uri $url -OutFile $download -UseBasicParsing
        if((Get-Sha256 $download) -ne $hash)
        {
            throw "Checksum mismatch: $download"
        }
        Move-Item -LiteralPath $download -Destination $target
    }
    if((Get-Sha256 $target) -ne $hash)
    {
        throw "Checksum mismatch: $target"
    }
}
Get-JsonFile 'include\nlohmann\json.hpp' 'https://raw.githubusercontent.com/nlohmann/json/v3.12.0/single_include/nlohmann/json.hpp' 'aaf127c04cb31c406e5b04a63f1ae89369fccde6d8fa7cdda1ed4f32dfc5de63'
Get-JsonFile 'LICENSE.MIT' 'https://raw.githubusercontent.com/nlohmann/json/v3.12.0/LICENSE.MIT' '46a65cffd1ea955132d95a8dd921640714a8d6b537d2e4e482d31145ae95b603'
$prefix=Join-Path $root $Configuration
$stamp=Join-Path $prefix '.ready'
$stampValue="taglib-2.3.2_zlib-1.3.2_x64_v143_$Configuration"
$zlibName=if($Configuration -eq 'Debug') {'zsd.lib'} else {'zs.lib'}
if((Test-Path -LiteralPath $stamp) -and
   (Get-Content -LiteralPath $stamp -Raw).Trim() -eq $stampValue -and
   (Test-Path -LiteralPath (Join-Path $prefix 'lib\tag.lib')) -and
   (Test-Path -LiteralPath (Join-Path $prefix "lib\$zlibName")) -and
   (Test-Path -LiteralPath (Join-Path $prefix 'include\taglib\fileref.h')))
{
    Write-Output "TagLib ready ($Configuration)."
    exit 0
}
function Get-Source($name,$version,$url,$hash)
{
    $folder=Join-Path $root "$name-$version"
    $downloads=Join-Path $root 'downloads'
    New-Item -ItemType Directory -Path $downloads -Force | Out-Null
    $archive=Join-Path $downloads "$name-$version.tar.gz"
    if(-not (Test-Path -LiteralPath $archive))
    {
        [Net.ServicePointManager]::SecurityProtocol=[Net.SecurityProtocolType]::Tls12
        Invoke-WebRequest -Uri $url -OutFile $archive -UseBasicParsing
    }
    if((Get-Sha256 $archive) -ne $hash)
    {
        throw "Checksum mismatch: $archive"
    }
    if(-not (Test-Path -LiteralPath (Join-Path $folder 'CMakeLists.txt')))
    {
        & tar -xzf $archive -C $root
        if($LASTEXITCODE -ne 0) { throw "Cannot extract $archive" }
    }
    return $folder
}
function Run-CMake([string[]]$Arguments)
{
    & $script:cmake @Arguments
    if($LASTEXITCODE -ne 0) { throw 'CMake dependency build failed.' }
}
$cmakeCommand=Get-Command cmake -ErrorAction SilentlyContinue
if($cmakeCommand)
{
    $script:cmake=$cmakeCommand.Source
}
else
{
    $vswhere=Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if(-not (Test-Path -LiteralPath $vswhere)) { throw 'Visual Studio Installer was not found.' }
    $script:cmake=& $vswhere -all -products '*' -find 'Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe' | Select-Object -First 1
    if(-not $script:cmake) { throw 'Install the Visual Studio C++ CMake tools component.' }
}
$taglib=Get-Source 'taglib' '2.3.2' 'https://github.com/taglib/taglib/releases/download/v2.3.2/taglib-2.3.2.tar.gz' '3ca2d8afaa7f1cf7f6ed10e511ebc368bfacd6dcaa3dbfa690b89e502e8963dc'
$zlib=Get-Source 'zlib' '1.3.2' 'https://github.com/madler/zlib/releases/download/v1.3.2/zlib-1.3.2.tar.gz' 'bb329a0a2cd0274d05519d61c667c062e06990d72e125ee2dfa8de64f0119d16'
$suffix=if($Configuration -eq 'Debug') {'d'} else {'r'}
$zlibBuild=Join-Path $root "z$suffix"
Run-CMake @('-S',$zlib,'-B',$zlibBuild,'-G','Visual Studio 17 2022','-A','x64','-T','v143',
    "-DCMAKE_INSTALL_PREFIX=$prefix",'-DCMAKE_TRY_COMPILE_CONFIGURATION=Release',
    '-DZLIB_BUILD_SHARED=OFF','-DZLIB_BUILD_STATIC=ON','-DZLIB_BUILD_TESTING=OFF')
Run-CMake @('--build',$zlibBuild,'--config',$Configuration,'--parallel','1','--','/nodeReuse:false')
Run-CMake @('--install',$zlibBuild,'--config',$Configuration)
$taglibBuild=Join-Path $root "t$suffix"
Run-CMake @('-S',$taglib,'-B',$taglibBuild,'-G','Visual Studio 17 2022','-A','x64','-T','v143',
    "-DCMAKE_INSTALL_PREFIX=$prefix",'-DCMAKE_TRY_COMPILE_CONFIGURATION=Release',
    '-DBUILD_SHARED_LIBS=OFF','-DBUILD_TESTING=OFF','-DBUILD_BINDINGS=OFF','-DBUILD_EXAMPLES=OFF',
    '-DENABLE_STATIC_RUNTIME=OFF','-DWITH_ZLIB=ON',"-DZLIB_INCLUDE_DIR=$prefix/include", "-DZLIB_LIBRARY=$prefix/lib/$zlibName",
    '-DWITH_APE=OFF','-DWITH_ASF=OFF','-DWITH_DSF=OFF','-DWITH_MATROSKA=OFF','-DWITH_MOD=OFF',
    '-DWITH_MP4=OFF','-DWITH_RIFF=OFF','-DWITH_SHORTEN=OFF','-DWITH_TRUEAUDIO=OFF','-DWITH_VORBIS=ON')
Run-CMake @('--build',$taglibBuild,'--config',$Configuration,'--parallel','1','--','/nodeReuse:false')
Run-CMake @('--install',$taglibBuild,'--config',$Configuration)
if(-not (Test-Path -LiteralPath (Join-Path $prefix 'lib\tag.lib'))) { throw 'TagLib library is missing.' }
if(-not (Test-Path -LiteralPath (Join-Path $prefix "lib\$zlibName"))) { throw 'zlib library is missing.' }
$stampValue | Set-Content -LiteralPath $stamp -Encoding ASCII
Write-Output "TagLib ready ($Configuration)."
