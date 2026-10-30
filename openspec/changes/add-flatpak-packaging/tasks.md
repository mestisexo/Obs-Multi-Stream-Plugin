# Tasks

## 1. Preparación y estructura

- [x] 1.1 Verificar el entorno Flatpak: `flatpak info com.obsproject.Studio` muestra OBS 33.x con runtime 26.08, y `flatpak list --runtime` incluye `org.freedesktop.Sdk` 26.08; si falta el SDK o `flatpak-builder`, instalarlos (paquete nativo o la app `org.flatpak.Builder`) y confirmar con `flatpak-builder --version` o `flatpak run --command=flatpak-builder org.flatpak.Builder --version`.
- [ ] 1.2 Crear `packaging/flatpak/` y añadir a `.gitignore` las rutas de trabajo `/.flatpak-builder/` y cualquier artefacto de build no cubierto; verificar con `git status --short` que no aparecen directorios de build.

## 2. Manifiesto y metadatos

- [x] 2.1 Crear `packaging/flatpak/com.obsproject.Studio.Plugin.MultiStreamMS.yaml` con id `com.obsproject.Studio.Plugin.MultiStreamMS`, `runtime: com.obsproject.Studio`, `runtime-version: stable`, `sdk: org.freedesktop.Sdk//26.08`, `build-extension: true`, prefijo `/app/plugins/MultiStreamMS`, fuente `type: dir` con `path: ../..` y el módulo `cmake-ninja` con `CMAKE_INSTALL_LIBDIR=lib`, `CMAKE_INSTALL_PREFIX=${FLATPAK_DEST}` y `CMAKE_PREFIX_PATH=/app`; verificar que el manifiesto es YAML válido y que `flatpak-builder --show-manifest` lo acepta.
- [x] 2.2 Crear `packaging/flatpak/com.obsproject.Studio.Plugin.MultiStreamMS.metainfo.xml` con `type="addon"`, `extends com.obsproject.Studio`, licencia `GPL-2.0-or-later` y URL del proyecto; validar con `appstreamcli validate` si está disponible o, en su defecto, `xmllint --noout`.
- [x] 2.3 Crear `packaging/flatpak/flathub.json` con `skip-icons-check: true` y `only-arches: ["x86_64"]`; validar con `python3 -m json.tool packaging/flatpak/flathub.json`.
- [x] 2.4 Añadir al manifiesto el `post-install` que instala el metainfo en `${FLATPAK_DEST}/share/metainfo` y verificar que la ruta aparece en el manifiesto final con `flatpak-builder --show-manifest`.

## 3. Script de compilación e instalación

- [x] 3.1 Implementar `packaging/flatpak/build.sh` con comprobación de `flatpak-builder`, la app `com.obsproject.Studio` y el SDK 26.08, usando `--state-dir=build-flatpak/state` y `--repo=build-flatpak/repo`; verificar sintaxis con `bash -n` y que `--help` muestra los modos disponibles.
- [x] 3.2 Implementar el modo por defecto de compilación e instalación de la extensión con `flatpak-builder --user --install --force-clean`; verificar que `flatpak list | grep com.obsproject.Studio.Plugin.MultiStreamMS` muestra la extensión instalada.
- [x] 3.3 Implementar el modo `--drop-in` que copia el `.so` y `data/` a `~/.var/app/com.obsproject.Studio/data/obs-studio/plugins/multi-stream-ms/`; verificar que los archivos existen en esa ruta y que el script informa la ruta final.
- [x] 3.4 Implementar `--build-only` y un mensaje de error claro cuando falte una dependencia; verificar ejecutando el script con una dependencia simulada ausente o revisando las comprobaciones del propio script.

## 4. Documentación

- [ ] 4.1 Añadir al `README.md` la sección “Linux (Flatpak)” con requisitos, comandos de build/instalación, rutas de la extensión y del drop-in, comprobación en OBS y desinstalación; verificar que cada comando documentado existe en `build.sh` o en el manifiesto.
- [ ] 4.2 Crear `packaging/flatpak/README.md` con los detalles de desarrollo, la diferencia entre extensión y drop-in, la reconstrucción tras actualizar OBS Flatpak y el troubleshooting básico; verificar que las rutas y comandos coinciden con el script y con `flatpak info`.
- [ ] 4.3 Documentar en `packaging/flatpak/README.md` el cambio de fuente `type: dir` a `type: git` necesario para una futura publicación en Flathub; verificar que la explicación incluye URL, tag/commit y el requisito de que el manifiesto viva en el repo de Flathub.

## 5. Verificación de integración

- [ ] 5.1 Compilar e instalar la extensión en OBS Flatpak 33; verificar en el log de OBS (`~/.var/app/com.obsproject.Studio/config/obs-studio/logs/`) que el módulo `multi-stream-ms` carga y que el dock “Multi-Stream MS” aparece en Docks.
- [ ] 5.2 Probar un destino RTMP contra MediaMTX en `rtmp://localhost:1935/test1`: verificar que el estado pasa a “En vivo”, que las métricas de MB/frames/congestión se actualizan y que VLC recibe vídeo.
- [ ] 5.3 Verificar el ciclo de vida: detener la transmisión principal detiene los destinos, cambiar de perfil no deja salidas activas y cerrar OBS no provoca crash.
- [ ] 5.4 Probar el fallback drop-in: desinstalar la extensión, ejecutar `build.sh --drop-in`, arrancar OBS y confirmar que el módulo y el dock siguen funcionando.
- [ ] 5.5 Confirmar que no hay cambios en `buildspec.json` ni en `CMakePresets.json`; revisar que cualquier ajuste en `src/` o `CMakeLists.txt` sea solo de compatibilidad de compilación con OBS 33 y no altere el comportamiento; verificar con `git diff --stat` y revisión de los archivos modificados.
- [ ] 5.6 Desinstalar la extensión con `flatpak uninstall com.obsproject.Studio.Plugin.MultiStreamMS`, borrar el drop-in si se creó y verificar que OBS ya no carga el plugin ni muestra el dock.

## Workflow follow-up

- Archivar el cambio después de que la revisión del proyecto esté satisfecha.
- Si se decide publicar, abrir el PR a Flathub con el manifiesto en `type: git` y los metadatos de `packaging/flatpak/`.
