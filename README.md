# Multi-Stream MS

Plugin de OBS Studio: envía la transmisión principal a destinos RTMP adicionales
**reutilizando los codificadores de la principal** (no codifica dos veces).

## Compilar (Windows, Visual Studio 2026 / 18)

Desde "Developer PowerShell for VS", en esta carpeta:

    cmake --preset windows-x64-vs2026
    cmake --build --preset windows-x64-vs2026 --config RelWithDebInfo
    cmake --install build_x64_vs2026 --config RelWithDebInfo --prefix "$env:ProgramData\obs-studio\plugins"

Con Visual Studio 2022 usa el preset `windows-x64` (necesita Windows SDK 10.0.22621).
La primera configuración descarga OBS, obs-deps y Qt6 (según `buildspec.json`).

Resultado: `%ProgramData%\obs-studio\plugins\multi-stream-ms\bin\64bit\multi-stream-ms.dll`

## Compilar e instalar (Linux Flatpak)

El plugin se distribuye como extensión de OBS Flatpak, sin recompilar OBS.

Requisitos:

- OBS Flatpak 33 (`com.obsproject.Studio`) con runtime 26.08.
- SDK `org.freedesktop.Sdk//26.08`.
- `flatpak-builder`, o la app `org.flatpak.Builder` si no quieres instalar el paquete nativo.

En Arch/CachyOS:

    sudo pacman -S flatpak-builder
    flatpak install --user flathub org.freedesktop.Sdk//26.08

En Debian/Ubuntu:

    sudo apt install flatpak-builder
    flatpak install --user flathub org.freedesktop.Sdk//26.08

Alternativa sin paquete nativo:

    flatpak install --user flathub org.flatpak.Builder
    flatpak install --user flathub org.freedesktop.Sdk//26.08

Compila e instala la extensión:

    ./packaging/flatpak/build.sh

Para development también puedes usar el modo drop-in:

    ./packaging/flatpak/build.sh --drop-in

Comprueba que la extensión aparece instalada:

    flatpak list | grep com.obsproject.Studio.Plugin.MultiStreamMS

Reinicia OBS y activa el dock "Multi-Stream MS" en el menú Docks. La guía completa
(requisitos, rutas, verificación, reconstrucción y problemas) está en
[`packaging/flatpak/README.md`](packaging/flatpak/README.md).

Desinstalar:

    flatpak uninstall com.obsproject.Studio.Plugin.MultiStreamMS
    rm -rf ~/.var/app/com.obsproject.Studio/data/obs-studio/plugins/multi-stream-ms

## Uso

1. Abre OBS y activa el dock desde el menú Docks ("Multi-Stream MS").
2. Inicia la transmisión principal.
3. "Agregar destino" (URL rtmp:// o rtmps:// y clave) y pulsa Iniciar.

Los destinos solo funcionan mientras la principal está en vivo, y se detienen con ella.
La clave se guarda en texto plano en `multi-stream-ms.json` dentro de la carpeta del perfil.

## Probar sin Twitch

Usa MediaMTX y el destino `rtmp://localhost:1935/test1`, y ábrelo en VLC.
