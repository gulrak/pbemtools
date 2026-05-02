# Vorlage v1.9.0

[![Build Status](https://github.com/gulrak/pbemtools/actions/workflows/build.yml/badge.svg?branch=master)](https://github.com/gulrak/pbemtools/actions/workflows/build.yml)
[![GitHub release](https://img.shields.io/github/v/release/gulrak/pbemtools)](https://github.com/gulrak/pbemtools/releases/latest)
[![GitHub nightly status](https://img.shields.io/github/checks-status/gulrak/pbemtools/nightly?label=nightly)](https://github.com/gulrak/pbemtools/releases/tag/nightly)
[![Platforms](https://img.shields.io/badge/platforms-Windows%20%7C%20macOS%20%7C%20Linux-blue)](#)
[![License: MIT](https://img.shields.io/badge/license-MIT-green)](LICENSE)

## PBeM-Zugvorlagen-Generator
Copyright (C) 1999-2026 by Steffen Schümann

**Support:** [Discussions hier](https://github.com/gulrak/pbemtools/discussions) und #vorlage auf dem [Eressea Discord](https://discord.gg/mjjNMS9u)\
**Web:** https://gulrak.de/pbemtools \
**Dokumentation:** https://github.com/gulrak/pbemtools/wiki/VorlageDokuAllgemeines \
**Alte Binaries:** https://gulrak.de/pbemtools/#downloads

**[Aktuelle Releases](https://github.com/gulrak/pbemtools/releases/latest)** basierend auf der Open-Source-Variante \
([Nightly Builds](https://github.com/gulrak/pbemtools/releases/tag/nightly) für den den aktuellen Stand verfügbar!)

_Metabefehle inspiriert durch Georg Edelmayers PERL-Vorlagen-Generator._

> ## Persönliche Anmerkung
> Ich stelle dieses Projekt nach vielen Jahren unter der MIT-Lizenz als Open Source zur Verfügung,
> weil ich es selbst nicht mehr aktiv pflegen kann, es aber weiterhin von der Eressea-Community
> genutzt wird.
>
> Der Code ist das Produkt einer anderen Zeit und spiegelt weder meine heutigen Ansprüche noch 
> meinen heutigen Stil als Entwickler wider. Seit ungefähr 20 Jahren bestand die Pflege im
> Wesentlichen nur noch aus kleineren Korrekturen und Maßnahmen, um das Programm lauffähig zu halten.
> 
> Ich veröffentliche den Quellcode deshalb nicht als Vorzeigeprojekt, sondern damit die
> Community eine Grundlage hat, auf der sie weiterarbeiten kann.

> ## Personal Note (English version)
> I am releasing this project as open source under the MIT License after many years because I can
> no longer actively maintain it myself, even though it is still being used by the Eressea community.
> 
> This code comes from a very different time and does not reflect my current standards or my 
> current style as a software developer. For roughly the last 20 years, maintenance has consisted 
> mostly of small fixes and basic life support to keep the program usable.
>
> I am publishing the source code not as a showcase of good modern software engineering, but 
> simply to give the community a foundation it can continue to use, maintain, and build upon.
>

# Einleitung

`VORLAGE` ist ein Konsolen-Programm, welches es ermöglicht, aus dem Computer-Report (CR) von Eressea,
Verdanon, Empiria o.ä. eine Zugvorlage erstellen zu lassen, ähnlich der, die man mit dem normalen
Report bekommt.

Die Beispiele und Texte in der Doku sind eher auf Eressea Spiel bezogen. Dies soll weder eine 
Wertung sein, noch andeuten, nur dieses Spiel werde unterstützt.

Die von Vorlage erzeugte Zugvorlage kann in einigen Bereichen aufgepeppt werden, um die Erstellung
des Zuges zu vereinfachen.

Ein Beispiel (Auszug einer imaginären Vorlage):

```
 REGION -3,6 ; Grollbat (Berg, 175 Personen, 4132$ Silber)
 ; ECheck Lohn 13
 ;  H S  |Bauern:    453 +28|Silber:   17278 +127|Unterhalt:   810  +7|
 ; S B . |Rekruten:   22  +1|Eisen:      513  -11|Gewinn:     1812 +84|
 ;  E .  |Pferde:     13  -3|Laen:         0     |Baeume:        0    |
 ;       |Balsam:     48  +0|Gewürz:      35   +0|Juwel:         0  +7|
 ;       |Myrrhe:     50  +0|Öl:          39   +0|Seide:        84  -6|
 ;       |Weihrauch:  56  -4
 ; Regionseinnahmen:  1628 Silber
 ; Regionsausgaben:   1750 Silber
 ; > Die Testaten (87) spendete 20 Silber an Schmarotzer (12).
 ; Durchgereist: Schlampige Transporteure (p0ng)

 ; -   -   -   -   -   -   -   -   -   -   -   -
 ; In Burg 'Cammelot' (4321) [3/50]:

  EINHEIT  b1ob;      Die Waffenschmiede [3,0$] flieht
  ; Gew: 174.0GE  Gehen: -127.8GE/16.2GE
  ; I Die Waffenschmiede (b1ob) in Grollbat (-3,6) produziert 4 Kriegsäxte.
  ; Waffenbau 5 [180]

  ; 18 Holz, 27 Kriegsaxt
    MACHE Kriegsaxt
```
Die Gewichtsangaben bedeuten: Gesamtgewicht der Einheit, sowie freie / theoretische Kapazität beim Reiten bzw. Gehen. Ist Reiten nicht möglich (z.B. keine Pferde oder kein Talent), wird die Reitangabe weggelassen. Sind zu viele Pferde vorhanden, so wird statt dessen dieses angezeigt.

Die eigentliche Dokumentation ist über das Wiki realisiert:

https://github.com/gulrak/pbemtools/wiki/VorlageDokuAllgemeines

# Compilieren von Vorlage

Für alle Plattformen werden folgende Werkzeuge benötigt:

- **CMake** 3.16 oder neuer
- **Git** (wird auch von CMake benötigt, um die Abhängigkeiten zu laden)
- Ein C++20-fähiger Compiler (siehe plattformspezifische Hinweise)

Die Abhängigkeiten PCRE2, fmtlib und Catch2 werden beim ersten Build-Lauf automatisch
von CMake heruntergeladen und compiliert – es ist keine manuelle Installation erforderlich.

## Windows

Empfohlen wird **Visual Studio 2022** (Community-Edition reicht aus) mit der installierten
Komponente „Desktopentwicklung mit C++". Visual Studio bringt CMake bereits mit; alternativ
kann CMake von [cmake.org](https://cmake.org/download/) installiert werden.

**Build mit Visual Studio (Developer Command Prompt oder PowerShell):**

```bat
git clone https://github.com/gulrak/pbemtools.git
cd pbemtools
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Das fertige Programm liegt anschließend unter `build\Vorlage\Release\vorlage.exe`.

> **Hinweis:** Auf Windows werden die MSVC-Laufzeitbibliotheken statisch eingebunden
> (`/MT`), sodass die erzeugte EXE-Datei ohne zusätzliche DLLs lauffähig ist.

Alternativ lässt sich das Projekt auch mit **MSYS2/MinGW-w64** bauen – dabei gelten
dieselben Befehle wie unter Linux.

## macOS

Voraussetzung sind die **Xcode Command Line Tools** sowie **CMake**. Die Command Line
Tools lassen sich über das Terminal installieren:

```bash
xcode-select --install
```

CMake kann entweder über [Homebrew](https://brew.sh/) oder direkt von
[cmake.org](https://cmake.org/download/) bezogen werden:

```bash
brew install cmake   # optional, falls noch nicht vorhanden
```

**Build:**

```bash
git clone https://github.com/gulrak/pbemtools.git
cd pbemtools
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Das fertige Programm liegt unter `build/Vorlage/vorlage`.

> **Hinweis:** Als Mindestanforderung gilt macOS 10.12 (Sierra). Getestet wurde mit
> Apple Clang aus den aktuellen Xcode Command Line Tools.

## Linux

Benötigt wird ein C++20-fähiger Compiler (**GCC 10+** oder **Clang 10+**) sowie CMake
und Git. Unter Debian/Ubuntu lassen sich alle Voraussetzungen so installieren:

```bash
sudo apt update
sudo apt install build-essential cmake git
```

Unter Fedora/RHEL:

```bash
sudo dnf install gcc-c++ cmake git
```

**Build:**

```bash
git clone https://github.com/gulrak/pbemtools.git
cd pbemtools
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Das fertige Programm liegt unter `build/Vorlage/vorlage`.

> **Hinweis:** Unter Linux werden libgcc und libstdc++ statisch eingebunden, sodass die
> erzeugte Binärdatei ohne passende Laufzeitbibliotheken auf anderen Systemen läuft.
> Bei Verwendung von Clang wird vollständig statisch gelinkt; `libc++abi-dev` muss dann
> ggf. zusätzlich installiert werden (`sudo apt install libc++abi-dev`).


# Lizenz

Das Programm ist unter der MIT-Lizenz lizenziert. Siehe die Datei [LICENSE](LICENSE) für Details.

# Danksagungen

Ich möchte meinen Testern und Ideenlieferanten herzlich für die Mitarbeit danken.
Ohne motivierte Anwender ließe sich das Tool sicher nicht so vorantreiben. Mein Dank
geht (in alphabetischer Reihenfolge) an:

_Andreas "Trickstar" Beer, Martin "Erchamion" Ehler, Thomas Gritzan, Günter Grossberger,
Klaus Lieberum, Michael "micham" Möller, Ralf Polster, Enno "Igrarjuk" Rehling, Jens Ricke,
Guido "Locksley" Sassmannshausen, Jens "Shannera" Schulze, "Tok", Karl Schwarz,
Matthias Strunk, "Widersach" und "Wuzel"._

Wenn ich genau Dich vergessen habe, so bitte ich das meinem porösen Hirn zuzuschreiben
und mir ruhig mitzuteilen. Ich habe leider nicht Buch geführt und die Namen sind aus 
dem Gedächtnis zusammengetragen und werden viel zu selten ergänzt.