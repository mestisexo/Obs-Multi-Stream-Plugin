# Spec Delta

## Purpose

Permite compilar, instalar y distribuir el plugin como extensión de OBS Studio en Linux Flatpak, de modo que el módulo y sus datos queden disponibles dentro del sandbox de OBS.

## ADDED Requirements

### Requirement: Extensión Flatpak de OBS

El proyecto SHALL proporcionar un manifiesto de extensión Flatpak con el identificador `com.obsproject.Studio.Plugin.MultiStreamMS` y prefijo de instalación `/app/plugins/MultiStreamMS`, construido contra el runtime `com.obsproject.Studio` y el SDK correspondiente a la versión de OBS Flatpak soportada.

#### Scenario: Construcción de la extensión

- **WHEN** un desarrollador ejecuta el comando documentado de construcción con OBS Flatpak y el SDK instalados
- **THEN** se genera e instala la extensión `com.obsproject.Studio.Plugin.MultiStreamMS`

#### Scenario: SDK incompatible

- **WHEN** el SDK requerido o `flatpak-builder` no están instalados
- **THEN** el proceso de construcción falla con un mensaje que indica la dependencia que falta y cómo instalarla

### Requirement: Carga del plugin dentro de OBS Flatpak

La extensión SHALL instalar el módulo `multi-stream-ms` y sus datos en las rutas que OBS 33 carga dentro del sandbox, de forma que OBS reconozca el plugin al iniciar.

#### Scenario: Inicio de OBS con la extensión instalada

- **WHEN** el usuario inicia OBS Flatpak con la extensión instalada
- **THEN** el registro de OBS muestra el módulo cargado y el dock "Multi-Stream MS" aparece en el menú de docks

#### Scenario: Datos de locale disponibles

- **WHEN** OBS inicia con la extensión instalada y el idioma español seleccionado
- **THEN** los textos del dock y de sus diálogos se muestran en español

### Requirement: Instalación local reproducible

El proyecto SHALL incluir un script documentado que compile e instale la extensión en la instalación de Flatpak del usuario, sin requerir permisos de administrador ni recompilar OBS.

#### Scenario: Instalación como extensión

- **WHEN** el usuario ejecuta el script de instalación en un sistema con OBS Flatpak 33 y el SDK 26.08
- **THEN** la extensión queda instalada para el usuario y disponible en el siguiente arranque de OBS

#### Scenario: Instalación mediante drop-in

- **WHEN** el usuario elige el modo de desarrollo drop-in
- **THEN** el script copia el módulo y sus datos a `~/.var/app/com.obsproject.Studio/data/obs-studio/plugins/multi-stream-ms/` y OBS puede cargarlo desde ahí

### Requirement: Metadatos listos para distribución

El proyecto SHALL incluir los metadatos AppStream y la configuración de arquitecturas necesarios para publicar la extensión en Flathub, con licencia del proyecto GPL-2.0-or-later y extensión de `com.obsproject.Studio`.

#### Scenario: Metadatos válidos

- **WHEN** se revisan los archivos de empaquetado de la extensión
- **THEN** el identificador, la relación con OBS, la licencia y la arquitectura `x86_64` están declarados de forma coherente con los requisitos de Flathub

### Requirement: Documentación de uso en Linux Flatpak

El proyecto SHALL documentar los requisitos, la construcción, la instalación, la verificación, la desinstalación y la reconstrucción tras una actualización de OBS Flatpak.

#### Scenario: Usuario nuevo en Linux

- **WHEN** un usuario de Linux Flatpak sigue la documentación desde cero
- **THEN** puede compilar e instalar el plugin sin consultar el código fuente ni el historial de la conversación

### Requirement: Sin regresión en otras plataformas

El cambio SHALL limitarse al empaquetado de Linux Flatpak y MUST NOT alterar el comportamiento del plugin ni los procesos de construcción de Windows y macOS.

#### Scenario: Compilaciones existentes

- **WHEN** se compila el proyecto para Windows o macOS con los presets existentes
- **THEN** el resultado y el comportamiento del plugin son equivalentes a los anteriores al cambio
