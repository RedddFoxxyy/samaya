<p align="center">
  <img src="data/icons/hicolor/scalable/apps/io.github.redddfoxxyy.samaya.svg" alt="Samaya app icon" width="200"/>
</p>

<h1 align="center">Samaya</h1>

<p align="center">
  <img src="data/screenshots/screenshot1.png"  alt="Samaya Screenshot 1"/>
<img src="data/screenshots/screenshot3.png"  alt="Samaya Screenshot 2"/>
</p>

#### A simple, elegant, minimalist Pomodoro timer for your desktop. Designed to help you stay focused and productive, it offers a clean, distraction-free interface to manage work and break intervals with ease.

## Features

* **Light Weight:** The size of the app is only around 55KB and around 111KB including all the data and assets!
* **Routine Management:** Switch between **Pomodoro (25 min)**, **Short Break (5 min)**, and **Long Break (20 min)** routines.
* **Custom Work/Break Durations:** Change the working or break durations in the settings menu.
* **Skip Sessions:** Skip the current session and start the next one.
* **Timer Notifications:** Get notified (using sound) when the timer ends.

## Download & Installation

- Get the app from FlatHub:

  <a href='https://flathub.org/apps/io.github.redddfoxxyy.samaya'>
  <img width='200' alt='Get it on Flathub' src='https://flathub.org/api/badge?locale=en'/>
  </a>

- Flatpak builds for linux are available in [latest release](https://github.com/RedddFoxxyy/samaya/releases/latest).
- For other operating systems, the app can only be compiled and installed manually from source 
	(and might fail because samaya depends heavily on glib, libcanberra and gsound which are linux only libs).
	
## Building from Source

### Software Dependencies Required:

  - GCC14 or later / Clang 19 or later
  - Meson (install using pip for latest version)
  - Ninja
  - CMake

### Library Dependencies Required:

| Dep Name                                                         | `pkg-config` Name | Min Version            | Justification                                       |
|------------------------------------------------------------------|-------------------|------------------------|-----------------------------------------------------|
| [gtk4](https://gitlab.gnome.org/GNOME/gtk/)                      | `gtk4`            | enforced by libadwaita | GUI                                                 |
| [libadwaita](https://gitlab.gnome.org/GNOME/libadwaita)          | `libadwaita-1`    | `1.4`                  | GNOME styling                                       |
| [gsound](https://gitlab.gnome.org/GNOME/gsound)                  | `gsound`          | `1.0.3`                | Playing System Sounds                               |
| [libcanberra](https://0pointer.de/lennart/projects/libcanberra/) | `libcanberra`     | `0.30`                 | Dependency for gsound                               |
| [glib](https://gitlab.gnome.org/GNOME/glib)                      | `glib-2.0`        | `2.74.6`               | Dependency for gtk4 and libadwaita                  |

### Steps to build:

1. **Clone the repository:**
   ```bash
   git clone [https://github.com/RedddFoxxyy/samaya.git](https://github.com/RedddFoxxyy/samaya.git)
   cd samaya
   ```
   
2. **Build the application using Meson:**
	```bash
	meson setup build_release --buildtype=release
	meson compile -C build_release
	./build_release/src/samaya
	```
	
## For Contributors:

  - The project follows a mix of C naming conventions from LLVM and GNOME/GNU style C code.
  - People using Zed Editor can directly run tasks to build and run the code (assuming all the required dependencies are installed).
	
## Code of Conduct

This project adheres to the [GNOME Code of Conduct](https://conduct.gnome.org/). By participating through any means, including PRs, Issues or Discussions, you are expected to uphold this code.

## License

This project is licensed under the **GNU Affero General Public License v3.0**. See
the [LICENSE](LICENSE) file for details.

Contributions made by all contributors are licensed strictly under **GNU AGPL v3.0 only**.

```
    Copyright (C) 2025  RedddFoxxyy(Suyog Tandel)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Affero General Public License as
    published by the Free Software Foundation, either version 3 of the
    License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Affero General Public License for more details.

    You should have received a copy of the GNU Affero General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
```

## Maintainers

[@RedddFoxxyy](https://github.com/RedddFoxxyy)
