# Multi-Stream MS para OBS Flatpak

Esta carpeta contiene el empaquetado Linux de Multi-Stream MS como extensión de
OBS Studio. El plugin se compila contra el OBS Flatpak instalado; no se
recompila OBS ni se descargan las dependencias de `buildspec.json`.

## Cómo funciona

- La extensión se llama `com.obsproject.Studio.Plugin.MultiStreamMS` y se monta
  en `/app/plugins/MultiStreamMS`.
- Usa el runtime `com.obsproject.Studio` (rama `stable`) y el SDK
  `org.freedesktop.Sdk//26.08`.
- El CMake del proyecto instala el módulo y los datos en:
  - `lib/obs-plugins/multi-stream-ms.so`
  - `share/obs/obs-plugins/multi-stream-ms/`
- El punto de extensión de OBS fusiona esas rutas en `/app/lib/obs-plugins` y
  `/app/share/obs/obs-plugins`, que OBS 33 sigue cargando.
- El modo drop-in es una alternativa para desarrollo: copia el `.so` y `data/`
  al directorio de plugins de usuario de OBS, sin instalar una extensión.

## Requisitos

- OBS Flatpak 33 (`com.obsproject.Studio`).
- SDK `org.freedesktop.Sdk//26.08`.
- `flatpak-builder` nativo, o la app `org.flatpak.Builder`.

El runtime y el SDK deben estar en la misma instalación de Flatpak que OBS. Si
OBS está instalado para todo el sistema, instala el SDK con `--system`; si OBS
está instalado para el usuario, usa `--user`.

### Instalar las herramientas

Arch/CachyOS:

```bash
sudo pacman -S flatpak-builder
```

Debian/Ubuntu:

```bash
sudo apt install flatpak-builder
```

Fedora:

```bash
sudo dnf install flatpak-builder
```

Alternativa sin paquete nativo (útil si no tienes permisos de administrador):

```bash
flatpak install --user flathub org.flatpak.Builder
```

### Instalar OBS y el SDK

```bash
# Si OBS ya está instalado, omite esta línea.
flatpak install flathub com.obsproject.Studio

# SDK del sistema, para un OBS instalado con --system:
flatpak install --system flathub org.freedesktop.Sdk//26.08

# O SDK de usuario, si OBS también está instalado con --user:
flatpak install --user flathub org.freedesktop.Sdk//26.08
```

## Compilar e instalar

Desde la raíz del repositorio:

```bash
# Compila e instala la extensión para el usuario actual.
./packaging/flatpak/build.sh

# Solo compila, sin instalar.
./packaging/flatpak/build.sh --build-only

# Compila y copia el plugin al directorio de usuario de OBS (desarrollo).
./packaging/flatpak/build.sh --drop-in

# Muestra la salida detallada de flatpak-builder.
./packaging/flatpak/build.sh --verbose
```

El script usa `build-flatpak/` como directorio de estado y repositorio local.
Ese directorio está ignorado por git.

## Rutas

Extensión instalada:

```text
/app/plugins/MultiStreamMS/lib/obs-plugins/multi-stream-ms.so
/app/plugins/MultiStreamMS/share/obs/obs-plugins/multi-stream-ms/locale/...
```

Dentro del sandbox de OBS, el punto de extensión las fusiona en:

```text
/app/lib/obs-plugins/multi-stream-ms.so
/app/share/obs/obs-plugins/multi-stream-ms/locale/...
```

Drop-in:

```text
~/.var/app/com.obsproject.Studio/data/obs-studio/plugins/multi-stream-ms/multi-stream-ms.so
~/.var/app/com.obsproject.Studio/data/obs-studio/plugins/multi-stream-ms/data/locale/...
```

## Verificar

```bash
flatpak list | grep com.obsproject.Studio.Plugin.MultiStreamMS
flatpak info com.obsproject.Studio.Plugin.MultiStreamMS
```

Después de reiniciar OBS:

1. Abre el menú **Docks** y activa **Multi-Stream MS**.
2. Revisa el log más reciente en
   `~/.var/app/com.obsproject.Studio/config/obs-studio/logs/`.
   Debe aparecer el módulo `multi-stream-ms` y el mensaje
   `plugin loaded successfully`.
3. Si usas la interfaz en español, comprueba que los textos del dock salen en
   español (`data/locale/es-ES.ini`).

## Reconstruir tras actualizar OBS

Cuando OBS Flatpak cambie de versión o de runtime:

```bash
flatpak update com.obsproject.Studio
./packaging/flatpak/build.sh
```

Si el runtime de OBS deja de ser 26.08, actualiza también `sdk` en
`com.obsproject.Studio.Plugin.MultiStreamMS.yaml` y vuelve a compilar. La
extensión debe compilarse contra el mismo OBS que la va a cargar.

## Problemas frecuentes

**`flatpak-builder` no se encuentra**

Instala el paquete nativo o la app `org.flatpak.Builder`. El script detecta
automáticamente la app si el binario nativo no está en `PATH`.

**No se encuentra el SDK**

Comprueba la instalación:

```bash
flatpak info org.freedesktop.Sdk//26.08
```

Si OBS es del sistema, el SDK debe estar en la instalación del sistema; si OBS
es de usuario, en la del usuario. `flatpak-builder` no mezcla runtime del
sistema con SDK de usuario.

**El build no encuentra libobs, obs-frontend-api o Qt6**

El manifiesto usa `CMAKE_PREFIX_PATH=/app`, que apunta a los headers y paquetes
CMake del OBS Flatpak. Verifica que la app de OBS esté instalada y que no estés
mezclando headers nativos de OBS con el SDK.

**La extensión está instalada pero el dock no aparece**

- Reinicia OBS por completo.
- Revisa el log de OBS.
- Retira cualquier copia nativa del plugin en `/usr` o
  `~/.config/obs-studio/plugins`; dos módulos con el mismo nombre pueden
  confundir al cargador.
- Comprueba que la extensión esté en la rama `stable`:

```bash
flatpak info com.obsproject.Studio.Plugin.MultiStreamMS
```

**El modo drop-in no carga**

Comprueba que existe
`~/.var/app/com.obsproject.Studio/data/obs-studio/plugins/multi-stream-ms/multi-stream-ms.so`
y que no está instalada también la extensión. Prueba a desinstalar la extensión,
reiniciar OBS y volver a ejecutar `./packaging/flatpak/build.sh --drop-in`.

**El destino RTMP no conecta**

Comprueba el permiso de red de OBS:

```bash
flatpak info --show-permissions com.obsproject.Studio
```

Debe incluir `shared=network;`. Revisa también la URL, la clave y que MediaMTX
esté escuchando en el puerto esperado.

## Notas de empaquetado

- El manifiesto local usa una fuente `type: dir` con `path: ../..`, que compila
  el árbol de trabajo actual. Para una publicación en Flathub habría que sustituirla
  por una fuente `type: git` con la URL del repositorio y un `tag` o `commit`
  fijo, y llevar el manifiesto al repositorio de Flathub.
- En este cambio no se abre un PR a Flathub; el metainfo y `flathub.json` quedan
  preparados para una publicación futura.
- La hoja de ruta incluye un `.deb` nativo para usuarios que no usan Flatpak,
  pero no forma parte de este empaquetado.
- No se añade un preset CMake específico de Flatpak: las opciones viven en el
  manifiesto para que el build sea reproducible dentro del sandbox.
