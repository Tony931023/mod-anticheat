# ![logo](https://raw.githubusercontent.com/azerothcore/azerothcore.github.io/master/images/logo-github.png) AzerothCore

[English](README.md) | [Español](README_ES.md)

## Anticheat Module

- Latest build status with AzerothCore: [![Build Status](https://github.com/azerothcore/mod-anticheat/workflows/core-build/badge.svg?branch=master&event=push)](https://github.com/azerothcore/mod-anticheat)

This is a port of the PassiveAnticheat Script from lordpsyan's repo to [AzerothCore](http://www.azerothcore.org)

## Notice

Requires revision [c50f7feda](https://github.com/azerothcore/azerothcore-wotlk/commit/c50f7feda0ee360f7bcca7f004bf6fb22abde533) or newer.

## How to install

### 1) Simply place the module under the `modules` folder of your AzerothCore source folder.

You can do clone it via git under the azerothcore/modules directory:

`cd path/to/azerothcore/modules`

`git clone https://github.com/azerothcore/mod-anticheat.git`

or you can manually [download the module](https://github.com/azerothcore/mod-anticheat/archive/master.zip), unzip and place it under the `azerothcore/modules` directory.

### 2) Re-run cmake and launch a clean build of AzerothCore

### 3) Execute the included `\sql\characters\base\charactersdb_anticheat.sql` file on your characters database and excute  `\sql\world\Acore_strings.sql` on your world datase. This creates the necessary tables for this module.

**That's it.**

### (Optional) Edit module configuration

If you need to change the module configuration, go to your server configuration folder (e.g. **etc**), copy `Anticheat.conf.dist` to `Anticheat.conf` and edit it as you prefer.

# Known issues and TODO list:

- [ ] Identify any class\spells that gives false positives, so far only blink and killingspree has been reported
- [ ] Currently no dedicated collumn for ignore control or teleport hack reporting, but the numbers are added to total report. Custom Messages are adjusted in `acore_string` for those two anticheats. The reason being is for some damn reason we are getting unrealistic high numbers being reported in the sql if i add in another collumn or two. I have no idea why that happens.

# Localize Acore_Strings
- [x] LOCALE_enUS = 0
- [ ] LOCALE_koKR = 1
- [ ] LOCALE_frFR = 2
- [ ] LOCALE_deDE = 3
- [ ] LOCALE_zhCN = 4
- [ ] LOCALE_zhTW = 5
- [x] LOCALE_esES = 6
- [x] LOCALE_esMX = 7
- [ ] LOCALE_ruRU = 8
