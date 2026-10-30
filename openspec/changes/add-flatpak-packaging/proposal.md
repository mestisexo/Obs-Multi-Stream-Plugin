# Proposal

## Why

En Linux, OBS se distribuye principalmente como Flatpak y el plugin actual no puede cargarse dentro del sandbox: el `.so` compilado para el sistema no es visible para la aplicación Flatpak. Hoy el proyecto solo documenta la compilación e instalación en Windows, así que no existe una forma reproducible de usar el plugin en OBS Flatpak ni de que otras personas lo instalen.

## What Changes

- Añadir el empaquetado Flatpak para OBS Linux:
  - Manifiesto de extensión `com.obsproject.Studio.Plugin.MultiStreamMS`.
  - Metadatos AppStream y `flathub.json` para dejar la extensión lista para Flathub.
  - Script de compilación e instalación local de la extensión.
  - Modo alternativo de instalación “drop-in” en el directorio de plugins de usuario de OBS 33.
- Compilar el plugin contra el runtime de OBS Flatpak (`com.obsproject.Studio`) y el SDK `org.freedesktop.Sdk//26.08`, sin recompilar OBS ni descargar las dependencias de `buildspec.json`.
- Instalar el módulo y los datos de locale en las rutas que OBS 33 carga dentro del Flatpak (`lib/obs-plugins` y `share/obs/obs-plugins`).
- Documentar requisitos, compilación, instalación, verificación, desinstalación y reconstrucción tras actualizar OBS Flatpak.
- No se modifica la lógica del plugin ni el comportamiento de las compilaciones de Windows y macOS.

## Capabilities

### New Capabilities

- `flatpak-packaging`: empaquetado, compilación e instalación del plugin como extensión de OBS Flatpak, incluidos los metadatos necesarios para su distribución y las instrucciones de uso.

### Modified Capabilities

<!-- Ninguna: no cambian requisitos de capacidades existentes. -->

## Impact

- Nuevos archivos en `packaging/flatpak/`: manifiesto, metainfo, `flathub.json`, README y script de build/instalación.
- Cambios en `README.md` (sección Linux Flatpak) y, si hace falta, `.gitignore` para artefactos de Flatpak.
- No se toca `src/`, `CMakeLists.txt`, `buildspec.json` ni los presets existentes, salvo que la compilación contra OBS 33 exija un ajuste mínimo.
- Dependencias de desarrollo: `flatpak-builder`, la app `com.obsproject.Studio` y el SDK `org.freedesktop.Sdk//26.08`.
- Queda fuera de alcance: `.deb`/AppImage nativos, CI, envío del PR a Flathub, firma/notarización de macOS y empaquetado de Windows. Los artefactos de metadatos quedarán preparados para una publicación posterior en Flathub.
