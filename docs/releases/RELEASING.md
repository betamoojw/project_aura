# Releasing Firmware

## 1) Bump firmware version
- Edit `platformio.ini`:
  - `-DAPP_VERSION=\"X.Y.Z\"`

## 2) Prepare assets
Run:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\prepare_release_assets.ps1 -Version X.Y.Z
```

This prepares files in `release-assets/vX.Y.Z` and refreshes local website installer files in `web-installer/`.

Important:
- GitHub Release should publish OTA file only.
- Full installer binaries/manifests are for your private hosting workflow.

Main generated files:
- `bootloader.bin`
- `partitions.bin`
- `boot_app0.bin`
- `firmware.bin`
- `littlefs.bin`
- `manifest.json`
- `manifest-update.json`
- `project_aura_X.Y.Z_ota_firmware.bin`
- `sha256sums.txt`

## 3) Publish to GitHub Release
Recommended command:

```powershell
powershell -ExecutionPolicy Bypass -File scripts\publish_github_release.ps1 -Version X.Y.Z -SkipReleaseUpdate -PruneAssetsToList
```

Notes:
- `-SkipReleaseUpdate` avoids metadata PATCH (useful on flaky networks).
- `-PruneAssetsToList` removes stale assets from previous uploads.
- By default, the script uploads only:
  - `project_aura_X.Y.Z_ota_firmware.bin`

## 4) Website usage
- Keep HTML installer block on your site.
- Keep manifests on your own hosting (`/wp-content/uploads/aura-installer/...`).
- Point manifest `path` fields to your storage/CDN links.

## 5) OTA dashboard update
- Use `project_aura_X.Y.Z_ota_firmware.bin` in `/dashboard -> System -> Update Firmware`.
