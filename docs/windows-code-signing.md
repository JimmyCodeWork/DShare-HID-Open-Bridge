# Windows code signing and release verification

Browsers and Windows judge downloads by **reputation**. Chrome Safe Browsing and Microsoft
SmartScreen warn about executables and installers that are unsigned or rarely downloaded. An
Authenticode signature lets that reputation build up against the publisher instead of against
each individual file, which changes on every build.

This document describes how the Windows packages are signed, published and verified.

## What the build produces

The [`Windows preview build`](../.github/workflows/windows-preview.yml) workflow produces:

| File | Description |
| --- | --- |
| `dshare-hid-<version>-win-x64.msi` | Installer |
| `dshare-hid-<version>-win-x64-portable.zip` | Portable build |
| `SHA256SUMS.txt` | SHA-256 checksums of the files above |

Every MSI and ZIP also gets a [build provenance attestation][attestations]. It is signed with
Sigstore and records which repository, workflow and commit produced the file.

Pushing a tag that matches `v*` runs the
[`Windows release`](../.github/workflows/windows-release.yml) workflow. It builds the tag, checks
the checksums and uploads the packages to the GitHub release for that tag. It creates the release
first if it does not exist. Tags that contain a hyphen, such as `v0.1.0-alpha.2`, are published as
prereleases. You can also run the workflow by hand for an existing tag.

## Enabling Authenticode signing

Signing is optional. Without a certificate, the build stays unsigned and the workflow adds a
notice to the run summary. With a certificate:

1. During packaging, CPack signs every unsigned `.exe` and `.dll` in the staging directory,
   including the MSI custom action DLL. It does this in
   [`deploy/windows/pre-cpack.cmake.in`](../deploy/windows/pre-cpack.cmake.in). Third-party
   files that already have a valid signature, such as the Qt and Microsoft runtime DLLs, are
   left as they are.
2. The finished MSI is then signed.
3. Each signature is checked with `Get-AuthenticodeSignature`. The build fails if any check
   fails.

All signing goes through
[`deploy/windows/Invoke-CodeSigning.ps1`](../deploy/windows/Invoke-CodeSigning.ps1), which uses
`signtool` with a PFX certificate. Set these in the repository settings
(**Settings → Secrets and variables → Actions**):

| Name | Kind | Value |
| --- | --- | --- |
| `WINDOWS_SIGNING_CERTIFICATE` | Secret | The PFX file, Base64-encoded |
| `WINDOWS_SIGNING_CERTIFICATE_PASSWORD` | Secret | Password of the PFX file |
| `WINDOWS_SIGNING_TIMESTAMP_URL` | Variable (optional) | RFC 3161 timestamp server. Defaults to `http://timestamp.digicert.com` |

To encode a PFX file:

```powershell
[Convert]::ToBase64String([IO.File]::ReadAllBytes('codesign.pfx')) | Set-Clipboard
```

You can sign a local build the same way: set the same environment variables before
`cmake --build build --target package`.

### Obtaining a certificate

| Option | Notes |
| --- | --- |
| [SignPath Foundation][signpath] | Free for qualifying open-source projects. Signing happens on SignPath's service through its own GitHub Action, so it would replace the PFX step instead of feeding it |
| Certum Open Source Code Signing | Low-cost certificate that individual open-source developers can get. The key is kept in a cloud HSM (SimplySign), not in a PFX file |
| Azure Trusted Signing | Monthly subscription. Identity validation is only available in some regions |
| OV code signing certificate from a CA | Since 2023, private keys must be kept on hardware tokens or in a cloud HSM, so you can only export a PFX in limited cases |

Certificates whose private key cannot be exported as a PFX need a provider-specific `signtool`
invocation, such as `/dlib` with a metadata file or a CSP/KSP name. Adapt
`Invoke-CodeSigning.ps1` for the provider you choose. The rest of the pipeline stays the same.

Signing does not remove warnings right away. Since 2024, EV certificates no longer get immediate
SmartScreen reputation either. Reputation builds up as signed releases are downloaded.

## Verifying a download

On Windows (PowerShell):

```powershell
Get-FileHash .\dshare-hid-<version>-win-x64.msi -Algorithm SHA256
gh attestation verify .\dshare-hid-<version>-win-x64.msi --repo JimmyCodeWork/DShare-HID-Open-Bridge
```

On Linux or macOS:

```sh
sha256sum -c SHA256SUMS.txt --ignore-missing
gh attestation verify dshare-hid-<version>-win-x64.msi --repo JimmyCodeWork/DShare-HID-Open-Bridge
```

Compare the hash with the line in `SHA256SUMS.txt` from the same release. The attestation check
confirms that the file was built by this repository's GitHub Actions workflow and has not been
changed since.

## Reporting false positives

If a browser or antivirus product calls a verified file malicious, rather than merely uncommon,
report it as a false positive:

- Google Safe Browsing: <https://safebrowsing.google.com/safebrowsing/report_error/>
- Microsoft Defender and SmartScreen: <https://www.microsoft.com/wdsi/filesubmission>

[attestations]: https://docs.github.com/actions/security-for-github-actions/using-artifact-attestations
[signpath]: https://signpath.org/
