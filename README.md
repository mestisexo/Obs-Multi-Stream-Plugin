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

## Uso

1. Abre OBS y activa el dock desde el menú Docks ("Multi-Stream MS").
2. Inicia la transmisión principal.
3. "Agregar destino" (URL rtmp:// o rtmps:// y clave) y pulsa Iniciar.

Los destinos solo funcionan mientras la principal está en vivo, y se detienen con ella.
La clave se guarda en texto plano en `multi-stream-ms.json` dentro de la carpeta del perfil.

## Probar sin Twitch

Usa MediaMTX y el destino `rtmp://localhost:1935/test1`, y ábrelo en VLC.
