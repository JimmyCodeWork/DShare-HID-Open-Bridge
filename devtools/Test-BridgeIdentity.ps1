# SPDX-License-Identifier: GPL-2.0-only
# Added 2026-11-13. End-to-end checks for the local bridge identity generator.
#Requires -Version 7.0
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$output = Join-Path $repo ('.local/identity-tests/' + [Guid]::NewGuid().ToString('N'))
$generator = Join-Path $PSScriptRoot 'New-BridgeIdentity.ps1'
& $generator -OutputDirectory $output | Out-Null

if (-not ('BridgeIdentityTests' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.IO;
using System.Security.Cryptography;
using System.Text;

public static class BridgeIdentityTests
{
    public static void Verify(string directory)
    {
        using var signer = ECDsa.Create();
        signer.ImportFromPem(File.ReadAllText(Path.Combine(directory, "private-key.pem")));
        using var verifier = ECDsa.Create();
        verifier.ImportFromPem(File.ReadAllText(Path.Combine(directory, "public-key.pem")));
        var p = verifier.ExportParameters(false);
        var raw = File.ReadAllBytes(Path.Combine(directory, "public-key.bin"));
        if (raw.Length != 65 || raw[0] != 4 || p.Q.X.Length != 32 || p.Q.Y.Length != 32)
            throw new Exception("Invalid uncompressed P-256 key shape.");
        for (int i = 0; i < 32; i++)
            if (raw[i + 1] != p.Q.X[i] || raw[i + 33] != p.Q.Y[i])
                throw new Exception("Raw public key differs from PEM public key.");

        // Match CdcTransport's 32-byte host nonce + 32-byte device nonce message and R||S signature.
        byte[] nonces = RandomNumberGenerator.GetBytes(64);
        byte[] signature = signer.SignData(nonces, HashAlgorithmName.SHA256,
            DSASignatureFormat.IeeeP1363FixedFieldConcatenation);
        if (signature.Length != 64 || !verifier.VerifyData(nonces, signature,
            HashAlgorithmName.SHA256, DSASignatureFormat.IeeeP1363FixedFieldConcatenation))
            throw new Exception("Generated identity cannot complete the bridge signature contract.");
        nonces[0] ^= 1;
        if (verifier.VerifyData(nonces, signature, HashAlgorithmName.SHA256,
            DSASignatureFormat.IeeeP1363FixedFieldConcatenation))
            throw new Exception("A tampered handshake message was accepted.");
    }
}
'@
}
[BridgeIdentityTests]::Verify($output)
$privatePath = Join-Path $output 'private-key.pem'
$before = (Get-FileHash -LiteralPath $privatePath -Algorithm SHA256).Hash
$refused = $false
try {
    & $generator -OutputDirectory $output | Out-Null
} catch {
    if ($_.Exception.Message -notlike 'Refusing to replace an existing identity directory:*') { throw }
    $refused = $true
}
if (-not $refused) { throw 'An existing identity was not protected against overwrite.' }
if ((Get-FileHash -LiteralPath $privatePath -Algorithm SHA256).Hash -ne $before) {
    throw 'Existing private key changed.'
}
if ((Get-Content -LiteralPath (Join-Path $output '.gitignore') -Raw).Trim() -ne '*') {
    throw 'The identity directory is not excluded from Git.'
}
Write-Output 'PASS: P-256 key exports, compatible handshake signature, tamper rejection, overwrite protection, Git exclusion.'
Write-Output "Temporary test identity remains in ignored directory: $output"
