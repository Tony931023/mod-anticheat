# ![logo](https://raw.githubusercontent.com/azerothcore/azerothcore.github.io/master/images/logo-github.png) AzerothCore

[English](README.md) | [Español](README_ES.md)

## Módulo Antitrampas

- Último estado de compilación con AzerothCore: [![Build Status](https://github.com/azerothcore/mod-anticheat/workflows/core-build/badge.svg?branch=master&event=push)](https://github.com/azerothcore/mod-anticheat)

Este es una puertada del PassiveAnticheat Script del repositorio de lordpsyan's a [AzerothCore](http://www.azerothcore.org)

## Aviso

Requiere revisión [c50f7feda](https://github.com/azerothcore/azerothcore-wotlk/commit/c50f7feda0ee360f7bcca7f004bf6fb22abde533) o posterior.

## Cómo instalar

### 1) Simplemente coloque el módulo en la carpeta `modules` de su carpeta de origen de AzerothCore.

Puede clonarlo a través de git en el directorio azerothcore/modules:

`cd path/to/azerothcore/modules`

`git clone https://github.com/azerothcore/mod-anticheat.git`

o puede [descargar manualmente el módulo](https://github.com/azerothcore/mod-anticheat/archive/master.zip), descomprimirlo y colocarlo en el directorio `azerothcore/modules`.

### 2) Vuelva a ejecutar cmake e inicie una compilación limpia de AzerothCore

### 3) Ejecute el archivo `\sql\characters\base\charactersdb_anticheat.sql` incluido en su base de datos de personajes y ejecute `\sql\world\Acore_strings.sql` en su base de datos world. Esto crea las tablas necesarias para este módulo.

**Eso es todo.**

### (Opcional) Editar configuración del módulo

Si necesita cambiar la configuración del módulo, vaya a la carpeta de configuración de su servidor (p.ej. **etc**. ), copie `Anticheat.conf.dist` a `Anticheat.conf` y edítelo como prefiera.

# Problemas conocidos y lista de TODO:

- [ ] Identifique cualquier clase/hechizo que dé falsos positivos, hasta ahora solo se ha informado blink y killingspree.
- [ ] CActualmente no hay una columna dedicada para ignorar el control o el informe de hack de teletransporte, pero los números se agregan al informe total. Los mensajes personalizados se ajustan en `acore_string` para esos dos anticheats. La razón es que, por alguna maldita razón, estamos obteniendo números altos poco realistas que se informan en el sql si agrego otra columna o dos. No tengo idea de por qué sucede eso.

# Localizar Acore_Strings
- [x] LOCALE_enUS = 0
- [ ] LOCALE_koKR = 1
- [ ] LOCALE_frFR = 2
- [ ] LOCALE_deDE = 3
- [ ] LOCALE_zhCN = 4
- [ ] LOCALE_zhTW = 5
- [x] LOCALE_esES = 6
- [x] LOCALE_esMX = 7
- [ ] LOCALE_ruRU = 8
