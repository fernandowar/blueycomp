# Dark Transformer

Saturación de transformador VST3 / AU (Windows / macOS) basada en **Neural Amp Modeler (NAM)**: una red neuronal entrenada con una captura de un preamp con transformadores reales.

- **Drive** (-12 a +24 dB): cuánto empujas la señal contra el hierro. El nivel se compensa solo, así que subir el Drive cambia el color y no el volumen.
- **Mix** (0-100 %): mezcla en paralelo. El seco va alineado con la latencia del camino procesado, así que no genera filtro peine.
- **Output** (-18 a +18 dB) y botón **IN** (bypass sin clics).
- **VU "IRON"**: aguja analógica con el pico que entra al transformador (-20 a +3 dBFS) y LED de PEAK sobre 0 dBFS.
- **Captura**: el botón **CARGAR** abre cualquier `.nam` mono (por ejemplo una captura recién entrenada) y **ORIGINAL** vuelve a la que trae el plugin. La ruta queda guardada en la sesión.

Si en `Models/` no hay ninguna captura, el plugin usa un **transformador DSP de respaldo** (indicador "DSP"). En cuanto se agrega un `.nam` a `Models/`, queda incluido dentro del plugin (indicador "NAM").

## Detalles técnicos

- Motor: [NeuralAmpModelerCore](https://github.com/sdatkinson/NeuralAmpModelerCore) (MIT), fijado a un commit probado; soporta capturas A1 y A2.
- Una red por canal (estéreo real). Si la sesión no está a la frecuencia de la captura (normalmente 48 kHz), se remuestrea con Lanczos. La latencia (unas 29 muestras a 44.1 kHz) se mide al preparar el plugin y se informa al DAW.
- Calibración automática: al cargar una captura se mide su ganancia a -24 dBFS, así que Drive 0 dB suena al mismo volumen que el seco.
- CPU: depende del tamaño de la captura. Para usarlo en muchas pistas conviene entrenar en **A2 / Lite** y no en Standard (ver `Models/README.md`).

Compila automático en GitHub Actions junto con los demás plugins.

## Licencias

Ver `THIRD_PARTY_NOTICES.txt` (NAM, AudioDSPTools, nlohmann/json: MIT; Eigen: MPL 2.0; tipografías: OFL).
