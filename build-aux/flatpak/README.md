# Guide for building the flatpak:

- Use the `io.github.redddfoxxyy.samaya.json` manifest file in the project root for building the flatpak.
- Build commands are given below:

> [!NOTE]
>
> These commands should be ran at the project root, and not in build-aux/flatpak dir.

## Runtimes required to build the flatpak:

- AMD64
  `sudo flatpak install flathub org.gnome.Sdk//49 org.gnome.Platform//49`

- AARCH64
  `sudo flatpak --arch=aarch64 install flathub org.gnome.Sdk//49 org.gnome.Platform//49`

## Commands required to build the flatpak:

- AMD64
  - `flatpak run org.flatpak.Builder --force-clean --repo=repo build-dir io.github.redddfoxxyy.samaya.json`
  - `flatpak build-bundle repo samaya.flatpak io.github.redddfoxxyy.samaya`

- AARCH64
  - `flatpak run org.flatpak.Builder --arch=aarch64 --force-clean --repo=repo build-dir io.github.redddfoxxyy.samaya.json`
  - `flatpak build-bundle --arch=aarch64 repo samaya.flatpak io.github.redddfoxxyy.samaya`

## Commands required to install the built flatpak:

- `flatpak install --user samaya.flatpak`

## Commands required to remove the installed flatpak:

- `flatpak remove io.github.redddfoxxyy.samaya`

## Commands required to lint the built flatpak:

- `flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo`
- `flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.redddfoxxyy.samaya.json`
