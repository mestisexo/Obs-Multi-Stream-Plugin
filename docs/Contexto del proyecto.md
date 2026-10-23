### Principios de diseño

1. **Codificar una sola vez.** Es la decisión central. El plugin nunca crea ni configura un codificador; reutiliza los de la transmisión principal.
2. **Reutilizar lo que OBS ya trae.** Cada destino es una salida `rtmp_output` con un servicio `rtmp_custom`, los mismos componentes que usa OBS para transmitir. Así heredas su reconexión y su manejo de RTMP en vez de reimplementarlos.
3. **Aislar fallos.** Cada destino tiene su propia salida y su propio estado. Si uno falla, solo ese pasa a “Error”.
4. **Seguridad entre hilos.** libobs avisa de los eventos desde hilos que no son el de la interfaz. Los callbacks solo copian valores y encolan el trabajo al hilo de Qt, para no tocar widgets desde otro hilo.
5. **Limpieza estricta de recursos.** Antes de liberar una salida se desconectan sus señales, y los eventos viejos se descartan con un identificador de ejecución.
6. **Dependencias mínimas.** Usa `obs_data_t` de libobs para el JSON, sin librerías externas.

### Qué hace

Es un dock de OBS donde agregas destinos RTMP/RTMPS extra (URL y clave). Puedes iniciar, detener, reintentar y quitar cada uno. Muestra el estado con colores y, cada segundo, los MB enviados, los fotogramas perdidos y la congestión. Guarda los destinos por perfil en `multi-stream-ms.json`.

### Cómo funciona, y su relación con el codificador principal

Al pulsar Iniciar en un destino:

1. Comprueba que la transmisión principal esté en vivo.
2. Obtiene la salida principal con `obs_frontend_get_streaming_output()` y de ahí lee sus codificadores de video y audio.
3. Crea una salida nueva y le asigna esos **mismos** codificadores. La salida toma su propia referencia a ellos.
4. Le asigna el servicio con tu URL y clave, y la inicia.

El flujo queda así:

```
Escena → Codificador de video ─┬→ Salida principal → Twitch
         Codificador de audio ─┼→ Salida extra 1   → Destino A
                               └→ Salida extra 2   → Destino B
```

Los codificadores producen paquetes ya comprimidos, y cada salida solo los empaqueta y los envía por su conexión. Todos los destinos reciben exactamente los mismos datos.

Esto crea dos dependencias que el plugin gestiona:

- **Los destinos no pueden vivir sin la principal.** Como los destinos sostienen una referencia a los codificadores, si siguieran activos al parar la principal, los codificadores seguirían trabajando. Por eso se detienen todos cuando la principal se detiene, y también al cambiar de perfil o cerrar OBS.
- **Los codificadores no se tocan.** Detener o quitar un destino solo libera su salida, nunca los codificadores.

### Beneficios de rendimiento

- **CPU/GPU casi constante.** Codificar es lo caro, y aquí ocurre una vez sin importar cuántos destinos tengas. Con un codificador por destino, 3 destinos serían 3 veces la carga. Con NVENC, tu RTX 4060 no gasta más sesiones de codificación por destino.
- **Sincronía entre destinos.** Todos reciben los mismos fotogramas y los mismos keyframes, así que no hay deriva de audio y video entre plataformas.
- **Menos memoria.** Los paquetes comprimidos son los mismos para todos.

### Límites

- **El ancho de banda de subida se multiplica.** Con N destinos subes N veces el bitrate. Es el costo real: con 6 Mbps y 3 destinos necesitas unos 18 Mbps de subida estable.
- **Misma calidad para todos.** Resolución, bitrate y codec son los de la principal, y no se pueden ajustar por destino. Si una plataforma exige otro formato, este diseño no sirve para ella.
- **Un fallo del codificador afecta a todos.** El aislamiento cubre fallos de red y conexión, no del codificador compartido.
- **Un costo pequeño por destino.** Cada salida usa algo de CPU para el empaquetado RTMP y la red, mucho menos que codificar.

**Todo esto es el diseño, no una medición.** El plugin compila y carga, pero aún no has probado el flujo completo en OBS, y no he medido el uso de CPU. Para comprobarlo, compara en el Administrador de tareas la carga con 0, 1 y 2 destinos activos. Debería casi no moverse, y si sube mucho, algo no está compartiendo bien.