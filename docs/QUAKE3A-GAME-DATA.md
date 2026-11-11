# Quake III Arena game-data setup

This repository contains the GPL engine source and tools; it does not include the commercial Quake III Arena or Team Arena game-data archives. Obtain those files from an installation you are authorized to use.

## Required base-game data

Arrange the files like this:

```text
source-data/
  baseq3/
    pak0.pk3
    pak1.pk3
    pak2.pk3
    pak3.pk3
    pak4.pk3
    pak5.pk3
    pak6.pk3
    pak7.pk3
    pak8.pk3
  missionpack/             # optional Team Arena expansion
    pak0.pk3
    ...
```

The base-game staging scripts require and validate `baseq3/pak0.pk3` through `pak8.pk3`. They verify that each archive is a readable, non-empty ZIP/PK3 before copying it into the game install root. They do not extract the archives, alter their contents, download them, or commit them to Git. If a `missionpack/` source directory contains PK3 files, those are validated and copied as optional Team Arena data.

## Linux / macOS

From the repository root:

```sh
sh ./scripts/install-q3a-data.sh "/path/to/source-data" "/path/to/Quake3"
```

Python 3 is required for archive validation.

## Windows PowerShell

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\install-q3a-data.ps1 -SourceDataRoot "C:\Path\To\source-data" -GameInstallRoot "C:\Games\Quake3"
```

## About the uploaded `pak1(1).pk3`

The file currently supplied in this conversation is a small ZIP containing seven `.cfg` configuration files. It is not the official `baseq3/pak1.pk3` game-data archive and cannot satisfy the required base-game data set. Keep it separate from the official PK3 files.

## Protocol and compatibility

The selected engine source is intended to use Quake III Arena protocol 68, compatible at the network-protocol level with official 1.32 / 1.32c clients and servers. Staging game data does not itself prove runtime compatibility, reproduce the original 1.32c executable, or certify that every historical patch fix is present. Build and test the engine separately with the supplied official data.
