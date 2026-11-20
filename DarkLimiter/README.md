# Dark Limiter

Limitador / maximizador de **mastering** VST3 (Windows / macOS), pensado para aguantar mucha presión sonora.

- **THRESHOLD (fader izquierdo)**: de 0 a -20 dB. Bajarlo sube la ganancia que entra al limitador (hasta +20 dB).
- **OUTPUT (fader derecho)**: techo (ceiling) de -12 a 0 dB. Por defecto -0.3 dB.
- **TRUE PEAK**: activa/desactiva la detección de picos entre muestras (interpolación 4x).
- **Medidor LUFS short-term** (ventana de 3 s, ponderación K, ITU-R BS.1770) con historial de 60 s.
- Medidores de gain reduction y de pico de salida. Botón **IN** (bypass).

## Cómo funciona

Cadena de tres pasos (lookahead, detector enlazado en estéreo):

1. **Limitador de 3 bandas** (cruces a 120 Hz y 2.5 kHz), lookahead de 10 ms (ataque lento y suave) y release dependiente del programa en cada banda (graves más largo). Así un bombo no "agacha" a la voz ni a los platillos.
2. **Clipper suave** (sobremuestreo 4x) que se lleva los últimos ~2 dB de picos sin bajar la ganancia: conserva la pegada de los transitorios y reduce el bombeo.
3. **Etapa de seguridad** (lookahead 1 ms, release 40 ms) con detector True Peak: fija el techo.

Latencia total: unas 570 muestras (se informa al host).

## Tipografías

Michroma y Barlow (SIL Open Font License, ver `Fonts/*_OFL.txt`), incluidas dentro del plugin.
