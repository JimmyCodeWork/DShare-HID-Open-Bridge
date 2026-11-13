# SPDX-FileCopyrightText: 2026 DShare-HID Open Bridge contributors
# SPDX-License-Identifier: MIT

<#
.SYNOPSIS
  Authenticode-signs Windows binaries and installers with signtool.

.DESCRIPTION
  Signs every .exe, .dll and .msi under the given paths. Files that already carry a valid
  signature (for example Qt or Microsoft runtime DLLs) are left untouched.

  The certificate is read from environment variables so that it never appears on the command
  line or in the build tree:

    WINDOWS_SIGNING_CERTIFICATE           Base64-encoded PFX file.
    WINDOWS_SIGNING_CERTIFICATE_PASSWORD  Password of the PFX file.
    WINDOWS_SIGNING_TIMESTAMP_URL         RFC 3161 timestamp server
                                          (default: http://timestamp.digicert.com).

  When no certificate is configured the script prints a notice and exits successfully, so
  unsigned development and preview builds keep working. Pass -Required to fail instead.

.EXAMPLE
  ./deploy/windows/Invoke-CodeSigning.ps1 build/bin build/dshare-hid.msi
#>
[CmdletBinding()]
param(
  [switch] $Required,

  [Parameter(Mandatory, ValueFromRemainingArguments)]
  [string[]] $Path
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

if ([string]::IsNullOrWhiteSpace($env:WINDOWS_SIGNING_CERTIFICATE)) {
  if ($Required) { throw 'Code signing is required, but WINDOWS_SIGNING_CERTIFICATE is not set.' }
  Write-Host 'Code signing skipped: WINDOWS_SIGNING_CERTIFICATE is not set.'
  return
}

function Find-SignTool {
  $command = Get-Command signtool.exe -ErrorAction SilentlyContinue
  if ($command) { return $command.Source }

  $kitsRoot = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits\10\bin'
  $candidate = Get-ChildItem $kitsRoot -Filter signtool.exe -Recurse -ErrorAction SilentlyContinue |
    Where-Object { $_.Directory.Name -eq 'x64' } |
    Sort-Object { $_.Directory.Parent.Name -as [version] } -Descending |
    Select-Object -First 1
  if ($candidate) { return $candidate.FullName }

  throw 'signtool.exe was not found. Install the Windows SDK or run from a Visual Studio developer shell.'
}

$extensions = @('.exe', '.dll', '.msi')
$files = foreach ($item in $Path) {
  if (-not (Test-Path $item)) { throw "Path not found: $item" }
  Get-ChildItem $item -File -Recurse:((Get-Item $item).PSIsContainer) |
    Where-Object { $extensions -contains $_.Extension.ToLowerInvariant() }
}

$unsigned = @($files | Where-Object { (Get-AuthenticodeSignature $_.FullName).Status -ne 'Valid' })
if ($unsigned.Count -eq 0) {
  Write-Host 'Code signing: nothing to sign.'
  return
}

$signTool = Find-SignTool
$timestampUrl = if ($env:WINDOWS_SIGNING_TIMESTAMP_URL) { $env:WINDOWS_SIGNING_TIMESTAMP_URL } else { 'http://timestamp.digicert.com' }
$pfx = Join-Path ([System.IO.Path]::GetTempPath()) ("codesign-{0}.pfx" -f [guid]::NewGuid())

try {
  [System.IO.File]::WriteAllBytes($pfx, [Convert]::FromBase64String($env:WINDOWS_SIGNING_CERTIFICATE))

  $arguments = @('sign', '/fd', 'SHA256', '/td', 'SHA256', '/tr', $timestampUrl, '/f', $pfx)
  if ($env:WINDOWS_SIGNING_CERTIFICATE_PASSWORD) {
    $arguments += @('/p', $env:WINDOWS_SIGNING_CERTIFICATE_PASSWORD)
  }

  # Sign in small batches to keep the command line short and to retry timestamp hiccups.
  for ($i = 0; $i -lt $unsigned.Count; $i += 20) {
    $batch = @($unsigned[$i..([Math]::Min($i + 19, $unsigned.Count - 1))] | ForEach-Object FullName)
    $attempt = 0
    do {
      $attempt++
      & $signTool @arguments @batch | Out-Host
      if ($LASTEXITCODE -eq 0) { break }
      if ($attempt -ge 3) { throw "signtool failed with exit code $LASTEXITCODE." }
      Start-Sleep -Seconds (5 * $attempt)
    } while ($true)
  }
}
finally {
  if (Test-Path $pfx) { Remove-Item $pfx -Force }
}

$invalid = @($unsigned | Where-Object { (Get-AuthenticodeSignature $_.FullName).Status -ne 'Valid' })
if ($invalid.Count -gt 0) {
  throw "Signature verification failed for: $($invalid.FullName -join ', ')"
}
Write-Host "Code signing: signed $($unsigned.Count) file(s)."
