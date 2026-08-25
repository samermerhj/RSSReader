# 📰 RSSReader – Smart News Magazine

[![License: AGPLv3](https://img.shields.io/badge/License-AGPLv3-blue.svg)](https://www.gnu.org/licenses/agpl-3.0)
[![Qt Version](https://img.shields.io/badge/Qt-5.15+-green.svg)](https://www.qt.io/)
[![Platform](https://img.shields.io/badge/Platform-Linux%20%7C%20Windows-lightgrey.svg)](https://github.com/samermerhj/RSSReader)
[![Version](https://img.shields.io/badge/Version-1.1.0-orange.svg)](https://github.com/samermerhj/RSSReader/releases)

## 💡 The Story

In a place you've never heard of, time stood still.
Everything there is still stuck in the past, even XP still runs on some machines.

I closed my camera, and my printer went silent.
No one visits me anymore.

I live an urban life in the heart of the countryside, while those around me live a nomadic life.
The difference was enough to build a silent wall between me and them.

In that harsh isolation, I returned to my old archive.
But this time, I wasn't looking for a film, but for primitive code for a personal tool I once built to gather world news.
I extracted it from the ruins of oblivion, and rebuilt it in today's style.

Now, that intention has become **Smart RSS**; a smart, powerful, and vibrant news magazine.

**This is code worth trying.** 🚀

I'm sharing the first version of this code as free software, under a strong license that protects my work and respects my effort.
Professional features will come later.

## ✨ Features (Free Version)

### 🌟 Core Features

| Feature | Description |
| :--- | :--- |
| 🌐 **Multi-source RSS Fetching** | Add any number of RSS feeds. |
| 🧠 **Smart News Grouping** | Similar articles are merged into one topic. |
| 📰 **Daily Magazine Mode** | Automated journalistic formatting with cards and images. |
| 📜 **Live News Ticker** | Smooth scrolling with transition effects. |
| 💾 **SQLite Archiving** | Search and browse past news by date. |
| 🎨 **Beautiful Interface** | Modern, clean, with smart category icons. |
| ⏰ **Magazine Scheduling** | For example, every day at 6 PM. |
| ⚡ **Fast and Offline-capable** | After fetching news, you can read offline. |
| 🖱️ **Full Text Display** | "Show More/Less" button to expand text within the card. |
| 🔗 **Open Link from Source** | Click on the source name to open the link directly. |
| 🌍 **Multi-language Support** | 5 languages: English (default), Arabic, French, Russian, Chinese. |

### 🔒 Coming in the Professional Version (Closed Source, Paid)

- 📦 Ready-made feed package (100+ global and Arabic sources).
- 🎨 Custom themes and icons.
- 📄 Print newspaper support.
- 📧 Early access to updates.
- 🖼️ Animated magazine images.
- 📧 Text-to-speech for news.

**Built with Qt/C++. Lightweight. No bloat.**

## 📸 Screenshots

<div align="center">
  <img src="https://raw.githubusercontent.com/samermerhj/RSSReader/main/docs/screenshot1.jpg" alt="Main Interface" width="700"/>
  <br/>
  <em>Main Interface – News Display</em>
</div>

<br/>

<div align="center">
  <img src="https://raw.githubusercontent.com/samermerhj/RSSReader/main/docs/screenshot2.jpg" alt="Magazine Mode" width="700"/>
  <br/>
  <em>Magazine Mode – News Cards Display</em>
</div>

## 📥 Download

**Version:** `1.1.0`

| Platform | File | Link |
| :--- | :--- | :--- |
| **Linux (.deb)** | `rssreader-1.1.0-Linux.deb` | [Download](https://github.com/samermerhj/RSSReader/releases/download/deb-v1.1.0/rssreader-1.1.0-Linux.deb) |
| **Windows (Installer)** | `RSSReader_Setup.exe` | [Download](https://github.com/samermerhj/RSSReader/releases/download/v1.1.0-windows/RSSReader_Setup.exe) |
| **Windows 7 (with OpenSSL)** | `RSSReader_Win7_Setup.exe` | [Download](https://github.com/samermerhj/RSSReader/releases/download/v1.1.0-windows/RSSReader_Win7_Setup.exe) |


---

### 📦 All Releases

If you prefer to browse all available files, visit the [Releases Page](https://github.com/samermerhj/RSSReader/releases).
---
## 🔧 Windows Requirements

> ⚠️ **Important for Windows 7 users**:  
> Your system may not support HTTPS (TLS 1.2) by default.  
> To use the Windows version properly, please ensure the following:

### 📦 Required Components

| Component | Version | Why |
| :--- | :--- | :--- |
| **OpenSSL** | 1.1.x (64-bit) | Required for secure HTTPS connections. |
| **Microsoft Update** | KB3140245 | Enables TLS 1.2 support on Windows 7. |

### 🛠️ Installation Steps (Windows 7 not work)

 **fix OpenSSL  ** 

### ✅ Verification

After completing the steps above, the app should be able to fetch news from HTTPS sources (like BBC, RT, SkyNews) without errors.

> **Note for Windows 10/11 users**:  
> These steps are generally not required, as TLS 1.2 and OpenSSL are already included in the system.

## 🛠️ Building from Source (Linux)

### 1. Install Dependencies

```bash
sudo apt update
sudo apt install -y build-essential cmake qtbase5-dev \
    libqt5svg5-dev libqt5sql5-sqlite qt5-qmake dpkg-dev
```

### 2. Build .deb Package

```bash
# Clean any previous build
rm -rf build

# Create build directory
mkdir build && cd build

# Run CMake (Release build)
cmake -DCMAKE_BUILD_TYPE=Release ..

# Compile
make -j$(nproc)

# Create .deb package
cpack -G DEB

# Install the package
sudo apt install ./rssreader-1.0.0-Linux.deb
```

✅ **Done!** Find `RSSReader` in your application menu or run from terminal:
```bash
rssreader
```

---

### 📦 Build AppImage (Portable)

To get a single executable file without installation:

```bash
# 1. Install additional dependencies
sudo apt install -y libfuse2 wget file

# 2. Download linuxdeployqt
wget -c "https://github.com/probonopd/linuxdeployqt/releases/download/continuous/linuxdeployqt-continuous-x86_64.AppImage"
chmod +x linuxdeployqt-continuous-x86_64.AppImage
sudo mv linuxdeployqt-continuous-x86_64.AppImage /usr/local/bin/linuxdeployqt

# 3. Build (same steps as above)
rm -rf build && mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 4. Prepare AppDir
mkdir -p AppDir/usr/share/RSSReader
cp rssreader AppDir/usr/bin/
cp -r ../resources AppDir/usr/share/RSSReader/
cp -r ../translations AppDir/usr/share/RSSReader/
cp ../icons/tray_icon.svg AppDir/rssreader.svg

# 5. Create AppImage
linuxdeployqt AppDir/usr/bin/rssreader -appimage

# Result: RSSReader-x86_64.AppImage
```

### ✅ Run AppImage

```bash
chmod +x RSSReader-x86_64.AppImage
./RSSReader-x86_64.AppImage
```

---

### 🖼️ Install Emoji Fonts (Optional)

To ensure emojis display correctly:

```bash
sudo apt install fonts-noto-color-emoji
```

| Platform | Method |
| :--- | :--- |
| **Linux (Debian/Ubuntu)** | `sudo apt install fonts-noto-color-emoji` |
| **Linux (Fedora)** | `sudo dnf install google-noto-emoji-color-fonts` |
| **Linux (Arch)** | `sudo pacman -S noto-fonts-emoji` |
| **Windows** | Download from [Google Noto Emoji](https://fonts.google.com/noto/specimen/Noto+Color+Emoji) then install the font |
| **macOS** | `brew install --cask font-noto-color-emoji` |

---

### 🪟 Note for Windows Users

To build on Windows, use **MSVC** or **MinGW** with Qt installed, then:

```bash
# Via Qt Creator or command line
cmake -B build -S .
cmake --build build --config Release
```

> Windows does not support building .deb packages (Linux-specific).

## 📄 License

This software is free software under the GNU Affero General Public License version 3.0 (AGPLv3).

**What does this mean?**

| ✅ You can | ⚠️ You must | ⛔ You cannot |
| :--- | :--- | :--- |
| Use it for any purpose. | If you distribute a modified version, open your source code under AGPLv3. | Sell this software or any modified version without explicit permission. |
| Study and learn from the source code. | Keep the AGPLv3 license in every copy. | Use it in closed-source software without a commercial license. |
| Share it with anyone. | Indicate the changes you made. | Remove copyright and license notices from the files. |

**For Commercial Use:**

If you want to:
· Integrate the software into a commercial closed-source product.
· Modify the code without opening the source.
· Distribute the software commercially.

Contact me for a paid commercial license.
Open an Issue titled "Commercial License" or email me.

## ❤️ Support This Project

Your direct support helps me survive, pay the internet bill, and continue development.

You're seeing this, you'll be among the first to know about the professional version as soon as it launches.

👉 Support via Coindrop (https://coindrop.to/RSSReader)

₿ Direct Cryptocurrency Donation

| Currency | Address |
| :--- | :--- |
| Bitcoin (BTC) | bc1qrh4nw70w5hyrg4myuppv879z6sp40lzvr9k69m |

## 📞 Contact

| Method | Link |
| :--- | :--- |
| Email | mjosak7@gmail.com |
| GitHub Issues | Report Bugs |
| Commercial License Requests | Open an Issue titled "Commercial License" |
| Repository | https://github.com/samermerhj/RSSReader |

## 🙏 Special Thanks

· Qt Framework – For providing a powerful and open development platform.
· Open Source Community – For inspiration and continuous support.
· Everyone who supports this project – You are the reason I keep going.

---

<p align="center">
  <b>Made with ❤️ by Samer Merhj</b>
  <br/>
  <sub>© 2026 – RSSReader – Smart News Magazine</sub>
</p>
