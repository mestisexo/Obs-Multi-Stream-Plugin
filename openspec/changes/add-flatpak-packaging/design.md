# Design

## Context

Ver `proposal.md` para la motivación. El estado actual relevante:

- El proyecto ya compila el plugin como módulo `.so` y lo instala con el layout de Linux: `lib/<arch>/obs-plugins/multi-stream-ms.so` y `share/obs/obs-plugins/multi-stream-ms/`.
- En Linux, `CMakeLists.txt` usa `find_package(libobs)` y `find_package(obs-frontend-api)` del sistema; el template no descarga OBS ni Qt en Linux.
- El preset `ubuntu-x86_64` fuerza `CMAKE_INSTALL_LIBDIR=lib/CMAKE_SYSTEM_PROCESSOR-linux-gnu`, pensado para paquetes Debian, no para el punto de extensión de Flatpak.
- La instalación de referencia tiene OBS Flatpak 33.0.0 (`com.obsproject.Studio`, x86_64) con runtime `org.freedesktop.Platform 26.08` y SDK `org.freedesktop.Sdk 26.08`.
- El runtime de OBS Flatpak incluye los headers de OBS (`/app/include/obs`), los paquetes CMake de `libobs`, `obs-frontend-api` y Qt6, y las librerías compartidas. Por tanto se puede compilar el plugin contra el OBS que ya está instalado, sin recompilar OBS.
- OBS 33 carga extensiones Flatpak desde `/app/plugins/<ruta-librería>/obs-modules/plugins` y mantiene la ruta legacy `/app/plugins/<ruta-librería>/obs-plugins`. También acepta plugins de usuario en `$XDG_DATA_HOME/obs-studio/plugins/<plugin>/`.

## Goals / Non-Goals

**Goals:**

- Producir una extensión Flatpak instalable del plugin, compatible con OBS Flatpak 33.
- Reutilizar el código y el CMake actuales, sin duplicar la lógica de compilación.
- Dejar metadatos y configuración preparados para una futura publicación en Flathub.
- Dar un camino de desarrollo rápido (drop-in) y otro de distribución (extensión).
- Mantener intactos Windows, macOS y el comportamiento del plugin.

**Non-Goals:**

- Publicar el PR en Flathub dentro de este cambio.
- Crear un `.deb`, AppImage o paquete nativo de Linux.
- Añadir CI o firma de artefactos.
- Cambiar `buildspec.json` ni subir la versión de OBS para Windows/macOS.
- Añadir permisos adicionales al sandbox de OBS.

## Decisions

### 1. Extensión Flatpak como mecanismo principal

Se implementa una extensión `build-extension: true` con id `com.obsproject.Studio.Plugin.MultiStreamMS` y prefijo `/app/plugins/MultiStreamMS`.

**Por qué:** es el mecanismo recomendado por Flathub, sobrevive a las actualizaciones de OBS y no requiere que el usuario copie archivos a mano. El drop-in se mantiene solo como ayuda de desarrollo.

**Alternativas descartadas:**
- Solo drop-in en `$XDG_DATA_HOME`: sirve para el usuario, pero no es una vía de distribución limpia y no valida el flujo de Flathub.
- Recompilar OBS: innecesario y mucho más costoso.

### 2. Compilar contra el runtime de OBS, no contra OBS desde fuentes

El manifiesto usa `runtime: com.obsproject.Studio`, `runtime-version: stable` y `sdk: org.freedesktop.Sdk//26.08`. El build sandbox expone `/app` con los headers y CMake de OBS; se pasa `-DCMAKE_PREFIX_PATH=/app` para que `find_package(libobs)`, `find_package(obs-frontend-api)` y Qt6 resuelvan contra el runtime.

**Por qué:** garantiza que el plugin se compila para la misma versión de OBS que lo va a cargar y evita descargar las dependencias de `buildspec.json`, que en Linux no se usan.

**Alternativas descartadas:**
- Compilar OBS desde el tarball de `buildspec.json` dentro del Flatpak: añade tiempo, tamaño y riesgo de desalineación.
- Usar el SDK de KDE de ejemplos antiguos: el OBS Flatpak actual está basado en `org.freedesktop` 26.08; el SDK debe coincidir con ese runtime.

### 3. Instalar en las rutas legacy que OBS 33 sigue leyendo

El manifiesto pasa `-DCMAKE_INSTALL_LIBDIR=lib` y el CMake existente instala:

```text
/app/plugins/MultiStreamMS/lib/obs-plugins/multi-stream-ms.so
/app/plugins/MultiStreamMS/share/obs/obs-plugins/multi-stream-ms/locale/...
```

El punto de extensión de OBS fusiona `lib/obs-plugins` y `share/obs/obs-plugins` en `/app`, y OBS 33 mantiene el cargador legacy.

**Por qué:** evita tocar el CMake del proyecto y sigue funcionando con versiones anteriores de OBS. La ruta nueva `lib/obs-modules/plugins` exigiría un cambio de layout y no aporta nada mientras el cargador legacy exista.

**Riesgo asumido:** si una versión futura de OBS elimina el cargador legacy, habrá que migrar el layout. Se documenta como deuda técnica.

### 4. Un manifiesto local y una variante para Flathub

El manifiesto del repo usa `type: dir` con `path: ../..` para compilar el árbol de trabajo actual. La copia que se envíe a Flathub usará `type: git` con URL, tag y commit, como exige Flathub. Los metadatos (`metainfo.xml`, `flathub.json`) se mantienen en `packaging/flatpak/`.

**Por qué:** permite iterar sin hacer commit en cada prueba y, a la vez, deja claro qué hay que cambiar para publicar.

**Alternativa descartada:** usar solo `type: git` en el repo; obligaría a commitear y empujar antes de cada prueba local.

### 5. Script con modos explícitos

`packaging/flatpak/build.sh` ofrece:

- Modo por defecto: compilar e instalar la extensión con `flatpak-builder --user --install`.
- `--drop-in`: compilar y copiar el `.so` y `data/` a `~/.var/app/com.obsproject.Studio/data/obs-studio/plugins/multi-stream-ms/`.
- `--build-only`: compilar sin instalar.

El script comprueba `flatpak-builder`, la app `com.obsproject.Studio` y el SDK 26.08 antes de empezar.

**Por qué:** un solo punto de entrada reproducible, con mensajes claros cuando falta una dependencia. El drop-in es útil para diagnosticar problemas de carga de la extensión.

**Alternativa descartada:** solo comandos sueltos en el README; más propenso a errores de rutas y flags.

### 6. Sin preset CMake nuevo

Las opciones específicas de Flatpak (`CMAKE_INSTALL_LIBDIR=lib`, `CMAKE_PREFIX_PATH=/app`, `CMAKE_INSTALL_PREFIX=${FLATPAK_DEST}`) viven en el manifiesto, no en `CMakePresets.json`.

**Por qué:** el preset `ubuntu-x86_64` está pensado para paquetes nativos y depende de la arquitectura; un preset `flatpak` necesitaría variables de entorno del sandbox y confundiría a quien compile fuera de él.

### 7. Documentación en español, metadatos en inglés

El README y la guía de `packaging/flatpak/` se escriben en español, igual que el resto de la documentación del proyecto. El `metainfo.xml` se escribe en inglés, que es la convención de AppStream/Flathub.

## Risks / Trade-offs

- **Actualizaciones de OBS Flatpak** → La extensión se compila contra el runtime actual. Hay que reconstruirla cuando OBS cambie de runtime; Flathub lo hará automáticamente para la versión publicada. Se documenta el comando de reconstrucción local.
- **Deriva de versión respecto a Windows/macOS** → El `buildspec.json` sigue fijando OBS 31.1.1, pero el Flatpak usa OBS 33. Se compila contra 33 para Linux; las APIs usadas (`obs_frontend_add_dock_by_id`, `obs_frontend_get_streaming_output`, etc.) existen en 33. Si aparece un fallo de compilación, se corrige de forma mínima sin cambiar el comportamiento.
- **Ruta legacy eliminada en el futuro** → Mitigación: mantener el manifiesto aislado para poder migrar a `lib/obs-modules/plugins` sin tocar el código del plugin.
- **Qt del SDK frente al Qt del runtime** → Se fuerza `CMAKE_PREFIX_PATH=/app` para usar el Qt que OBS trae; así se evita enlazar contra un Qt distinto. Verificar que el `.so` carga dentro del sandbox.
- **Permisos del sandbox** → La extensión hereda los permisos de OBS (`--share=network`, acceso al perfil). No se añaden `finish-args`; si el RTMP falla por red, se revisa primero el permiso de la app, no el del plugin.
- **Metadatos de Flathub** → El id, el prefijo y la licencia deben cumplir las reglas de Flathub; se validan en la revisión. La publicación queda fuera de este cambio.
- **Tamaño de la toolchain** → `flatpak-builder` y el SDK 26.08 ocupan espacio; se documenta como requisito de desarrollo, no de uso.

## Migration Plan

- No hay migración de datos: la configuración sigue en el perfil de OBS (`multi-stream-ms.json`), dentro del sandbox.
- Para usuarios nuevos: instalar la extensión y reiniciar OBS.
- Para quien ya tenga una copia nativa en `/usr` o `~/.config/obs-studio/plugins`: retirarla antes de usar la extensión para evitar dos módulos con el mismo nombre.
- Rollback: `flatpak uninstall com.obsproject.Studio.Plugin.MultiStreamMS` y, si se usó el modo drop-in, borrar `~/.var/app/com.obsproject.Studio/data/obs-studio/plugins/multi-stream-ms/`.
- Windows y macOS no se ven afectados.

## Decisiones posteriores

- **`.deb` nativo:** se añadirá más adelante para usuarios de OBS que no usan Flatpak. Queda fuera de este cambio y no altera el diseño actual.
- **Flathub:** no se enviará el PR a Flathub en este cambio. El manifiesto, el metainfo y `flathub.json` quedan preparados para una publicación futura si se decide retomarla.
