# SPDX-License-Identifier: GPL-2.0-only
# Added 2026-11-13. Generate an identity for independently built bridge firmware.
#Requires -Version 7.0
[CmdletBinding()]
param(
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '../.local/bridge-keys')
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$destination = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $destination) {
    throw "Refusing to replace an existing identity directory: $destination. Choose a new directory to rotate keys."
}

$key = $null
try {
    $key = [Security.Cryptography.ECDsa]::Create([Security.Cryptography.ECCurve+NamedCurves]::nistP256)
    $parameters = $key.ExportParameters($false)
    if ($parameters.Q.X.Length -ne 32 -or $parameters.Q.Y.Length -ne 32) {
        throw 'The crypto provider returned an unexpected P-256 public key.'
    }
    [byte[]]$publicBytes = @(0x04) + $parameters.Q.X + $parameters.Q.Y
    $privateDer = $key.ExportPkcs8PrivateKey()
    $publicDer = $key.ExportSubjectPublicKeyInfo()
    $privatePem = "-----BEGIN PRIVATE KEY-----`n" +
        [Convert]::ToBase64String($privateDer, [Base64FormattingOptions]::InsertLineBreaks) +
        "`n-----END PRIVATE KEY-----`n"
    $publicPem = "-----BEGIN PUBLIC KEY-----`n" +
        [Convert]::ToBase64String($publicDer, [Base64FormattingOptions]::InsertLineBreaks) +
        "`n-----END PUBLIC KEY-----`n"

    # CreateDirectory is not exclusive. A lock file with CreateNew prevents competing writers.
    [IO.Directory]::CreateDirectory($destination) | Out-Null
    $lock = [IO.File]::Open((Join-Path $destination '.identity-lock'),
        [IO.FileMode]::CreateNew, [IO.FileAccess]::Write, [IO.FileShare]::None)
    try {
        foreach ($name in @('private-key.pem', 'public-key.pem', 'public-key.bin')) {
            if (Test-Path -LiteralPath (Join-Path $destination $name)) {
                throw "Refusing to overwrite $name."
            }
        }
        [IO.File]::WriteAllText((Join-Path $destination 'private-key.pem'), $privatePem, [Text.Encoding]::ASCII)
        [IO.File]::WriteAllText((Join-Path $destination 'public-key.pem'), $publicPem, [Text.Encoding]::ASCII)
        [IO.File]::WriteAllBytes((Join-Path $destination 'public-key.bin'), $publicBytes)
        [IO.File]::WriteAllText((Join-Path $destination '.gitignore'), "*`n", [Text.Encoding]::ASCII)
    } finally {
        $lock.Dispose()
    }
    $fingerprint = [Convert]::ToHexString([Security.Cryptography.SHA256]::HashData($publicBytes))
    Write-Output "Bridge identity created: $destination"
    Write-Output "Public-key SHA256: $fingerprint"
    Write-Output 'Keep private-key.pem local. The desktop build uses public-key.bin only.'
} finally {
    if ($null -ne $key) { $key.Dispose() }
}
