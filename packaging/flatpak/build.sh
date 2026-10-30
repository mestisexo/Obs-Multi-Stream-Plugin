#!/usr/bin/env bash
# Build and install the Multi-Stream MS extension for OBS Flatpak.
set -euo pipefail

APP_ID="com.obsproject.Studio"
EXT_ID="com.obsproject.Studio.Plugin.MultiStreamMS"
EXT_BRANCH="stable"
EXT_REF="${EXT_ID}//${EXT_BRANCH}"
SDK_REF="org.freedesktop.Sdk//26.08"
BUILDER_APP="org.flatpak.Builder"

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
MANIFEST="${SCRIPT_DIR}/${EXT_ID}.yaml"
BUILD_ROOT="${REPO_ROOT}/build-flatpak"
STATE_DIR="${BUILD_ROOT}/state"
REPO_DIR="${BUILD_ROOT}/repo"
BUILD_DIR="${BUILD_ROOT}/build"
DROPIN_DIR="${HOME}/.var/app/${APP_ID}/data/obs-studio/plugins/multi-stream-ms"

MODE="install"
VERBOSE=0

usage() {
	cat <<EOF
Uso: $(basename "$0") [opciones]

Compila e instala el plugin Multi-Stream MS como extensión de OBS Flatpak.

Opciones:
  (sin opciones)  Compila e instala la extensión para el usuario actual.
  --build-only    Compila la extensión sin instalarla.
  --drop-in       Compila y copia el módulo y sus datos al directorio de
                  plugins de usuario de OBS Flatpak (útil para desarrollo).
  --verbose       Muestra la salida detallada de flatpak-builder.
  -h, --help      Muestra esta ayuda.

Rutas:
  Extensión: ${EXT_REF}
  Drop-in:   ${DROPIN_DIR}

Dependencias:
  flatpak, OBS Flatpak (${APP_ID}), SDK (${SDK_REF}) y flatpak-builder
  (nativo o la app ${BUILDER_APP}).
EOF
}

die() {
	printf 'Error: %s\n' "$*" >&2
	exit 1
}

while (($# > 0)); do
	case "$1" in
	--build-only)
		MODE="build-only"
		;;
	--drop-in)
		MODE="drop-in"
		;;
	--verbose)
		VERBOSE=1
		;;
	-h | --help)
		usage
		exit 0
		;;
	*)
		die "opción desconocida: $1 (usa --help)"
		;;
	esac
	shift
done

command -v flatpak >/dev/null 2>&1 || die "falta flatpak. Instálalo con el gestor de paquetes de tu distribución."

BUILDER=()
if command -v flatpak-builder >/dev/null 2>&1; then
	BUILDER=(flatpak-builder)
elif flatpak info "${BUILDER_APP}" >/dev/null 2>&1; then
	BUILDER=(flatpak run --command=flatpak-builder "${BUILDER_APP}")
else
	die "falta flatpak-builder. Instala el paquete nativo (Arch: sudo pacman -S flatpak-builder; Debian/Ubuntu: sudo apt install flatpak-builder) o ejecuta: flatpak install --user flathub ${BUILDER_APP}"
fi

"${BUILDER[@]}" --version >/dev/null 2>&1 || die "flatpak-builder no se puede ejecutar correctamente."

if ! flatpak info "${APP_ID}" >/dev/null 2>&1; then
	die "falta la app ${APP_ID}. Instálala con: flatpak install flathub ${APP_ID}"
fi

if ! flatpak info "${SDK_REF}" >/dev/null 2>&1; then
	die "falta el SDK ${SDK_REF}. Instálalo con: flatpak install --user flathub ${SDK_REF}"
fi

[[ -f "${MANIFEST}" ]] || die "no se encuentra el manifiesto ${MANIFEST}"

BUILDER_ARGS=(--state-dir="${STATE_DIR}" --repo="${REPO_DIR}")
if ((VERBOSE)); then
	BUILDER_ARGS+=(--verbose)
fi

build_extension() {
	local extra_args=("$@")
	mkdir -p "${BUILD_ROOT}"
	"${BUILDER[@]}" --force-clean "${BUILDER_ARGS[@]}" "${extra_args[@]}" "${BUILD_DIR}" "${MANIFEST}"
}

find_built_module() {
	local module
	module="$(find "${BUILD_ROOT}" -type f -name 'multi-stream-ms.so' -print -quit 2>/dev/null || true)"
	[[ -n "${module}" ]] || die "no se encontró multi-stream-ms.so en ${BUILD_ROOT}; revisa la salida de flatpak-builder"
	printf '%s\n' "${module}"
}

install_extension() {
	build_extension --install --user

	if ! flatpak info "${EXT_REF}" >/dev/null 2>&1; then
		die "flatpak-builder terminó pero la extensión ${EXT_REF} no aparece instalada"
	fi

	printf '\nExtensión instalada: %s\n' "${EXT_REF}"
	printf 'Reinicia OBS Flatpak y activa el dock "Multi-Stream MS" en Docks.\n'
}

build_only() {
	build_extension --build-only --keep-build-dirs
	printf '\nBuild completado. Artefactos en %s\n' "${STATE_DIR}/build"
}

install_dropin() {
	build_extension --build-only --keep-build-dirs

	local module
	module="$(find_built_module)"

	rm -rf "${DROPIN_DIR}"
	mkdir -p "${DROPIN_DIR}/data"
	install -m755 "${module}" "${DROPIN_DIR}/multi-stream-ms.so"
	cp -a "${REPO_ROOT}/data/." "${DROPIN_DIR}/data/"

	printf '\nPlugin instalado en modo drop-in:\n'
	printf '  %s/multi-stream-ms.so\n' "${DROPIN_DIR}"
	printf '  %s/data/\n' "${DROPIN_DIR}"
	printf 'Reinicia OBS Flatpak y activa el dock "Multi-Stream MS" en Docks.\n'
	printf 'Recuerda retirar la extensión si también la tienes instalada.\n'
}

case "${MODE}" in
install)
	install_extension
	;;
build-only)
	build_only
	;;
drop-in)
	install_dropin
	;;
esac
