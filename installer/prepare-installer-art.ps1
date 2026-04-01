[CmdletBinding()]
param(
    [string]$OutputDir = (Join-Path $PSScriptRoot "art")
)

$ErrorActionPreference = "Stop"

Add-Type -AssemblyName System.Drawing

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$logoPath = Join-Path $repoRoot "assets\orion\Orion_logo_transpant_bg.png"

if (-not (Test-Path $logoPath)) {
    throw "Missing logo image at '$logoPath'."
}

if (-not (Test-Path $OutputDir)) {
    New-Item -Path $OutputDir -ItemType Directory -Force | Out-Null
}

function New-OrionBitmap {
    param(
        [Parameter(Mandatory = $true)][int]$Width,
        [Parameter(Mandatory = $true)][int]$Height,
        [Parameter(Mandatory = $true)][string]$OutputPath
    )

    $bitmap = New-Object System.Drawing.Bitmap($Width, $Height, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)

    try {
        $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
        $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
        $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::ClearTypeGridFit

        $rect = New-Object System.Drawing.Rectangle(0, 0, $Width, $Height)
        $gradient = New-Object System.Drawing.Drawing2D.LinearGradientBrush(
            $rect,
            [System.Drawing.Color]::FromArgb(12, 19, 31),
            [System.Drawing.Color]::FromArgb(6, 10, 18),
            90.0
        )

        try {
            $graphics.FillRectangle($gradient, $rect)
        } finally {
            $gradient.Dispose()
        }

        $accentBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 12, 122, 255))
        try {
            $graphics.FillRectangle($accentBrush, 0, 0, [Math]::Max(3, [int]($Width * 0.06)), $Height)
        } finally {
            $accentBrush.Dispose()
        }

        $logo = [System.Drawing.Image]::FromFile($logoPath)
        try {
            $logoPadding = if ($Width -le 64) { 8 } else { 14 }
            $maxLogoWidth = [Math]::Max(20, $Width - ($logoPadding * 2))
            $maxLogoHeight = [Math]::Max(20, [int]($Height * 0.42))
            $scale = [Math]::Min($maxLogoWidth / $logo.Width, $maxLogoHeight / $logo.Height)
            $drawWidth = [int]([Math]::Round($logo.Width * $scale))
            $drawHeight = [int]([Math]::Round($logo.Height * $scale))
            $drawX = [int](($Width - $drawWidth) / 2)
            $drawY = [int]($Height * 0.14)

            $graphics.DrawImage($logo, $drawX, $drawY, $drawWidth, $drawHeight)
        } finally {
            $logo.Dispose()
        }

        $headlineFontSize = if ($Width -le 64) { 8.5 } else { 17.0 }
        $subtitleFontSize = if ($Width -le 64) { 6.5 } else { 10.0 }

        $headlineFont = New-Object System.Drawing.Font("Segoe UI Semibold", $headlineFontSize, [System.Drawing.FontStyle]::Bold)
        $subtitleFont = New-Object System.Drawing.Font("Segoe UI", $subtitleFontSize, [System.Drawing.FontStyle]::Regular)
        $headlineBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(245, 245, 245))
        $subtitleBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(190, 205, 222))

        try {
            $textLeft = [int]($Width * 0.1)
            $textWidth = [int]($Width * 0.8)
            $headlineTop = [int]($Height * 0.66)

            $headlineRect = New-Object System.Drawing.RectangleF([single]$textLeft, [single]$headlineTop, [single]$textWidth, [single]($Height * 0.14))
            $subtitleRect = New-Object System.Drawing.RectangleF([single]$textLeft, [single]($headlineTop + ($Height * 0.12)), [single]$textWidth, [single]($Height * 0.16))

            $format = New-Object System.Drawing.StringFormat
            try {
                $format.Alignment = [System.Drawing.StringAlignment]::Center
                $format.LineAlignment = [System.Drawing.StringAlignment]::Near

                $graphics.DrawString("Orion", $headlineFont, $headlineBrush, $headlineRect, $format)
                $graphics.DrawString("Dreamer's DAW", $subtitleFont, $subtitleBrush, $subtitleRect, $format)
            } finally {
                $format.Dispose()
            }
        } finally {
            $headlineFont.Dispose()
            $subtitleFont.Dispose()
            $headlineBrush.Dispose()
            $subtitleBrush.Dispose()
        }

        $bitmap.Save($OutputPath, [System.Drawing.Imaging.ImageFormat]::Bmp)
    } finally {
        $graphics.Dispose()
        $bitmap.Dispose()
    }
}

$wizardBmp = Join-Path $OutputDir "wizard.bmp"
$wizardSmallBmp = Join-Path $OutputDir "wizard-small.bmp"

New-OrionBitmap -Width 164 -Height 314 -OutputPath $wizardBmp
New-OrionBitmap -Width 55 -Height 55 -OutputPath $wizardSmallBmp

Write-Host "Generated installer artwork:"
Write-Host "  $wizardBmp"
Write-Host "  $wizardSmallBmp"
