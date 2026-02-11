Set-Location $PSScriptRoot

$folder = $PSScriptRoot
$exe = Join-Path $folder "TextureConverter.exe"

# mip レベル（不要なら 0 に）
$mipLevel = 4

# jpg と png を再帰取得
$files = Get-ChildItem -Path $folder -Include *.jpg, *.png -File -Recurse

foreach($f in $files){

    # 拡張子取得（.png → png）
    $ext = $f.Extension.TrimStart('.')

    # 出力ファイル名：a_png.dds 形式
    $outputPath = Join-Path $f.DirectoryName ($f.BaseName + "_" + $ext + ".dds")

    # 既に存在するならスキップ
    if(Test-Path $outputPath){
        Write-Host "Skip (already exists): $outputPath"
        continue
    }

    Write-Host "Converting: $($f.FullName)"

    $args = @(
        "`"$($f.FullName)`"",
        "-ml", "$mipLevel"
    )

    Start-Process -FilePath $exe -ArgumentList $args -Wait
}

Write-Host "Done."
pause
