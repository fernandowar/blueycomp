# Dark Limiter

Limitador / maximizador de **mastering** VST3 (Windows / macOS), pensado para aguantar mucha presión sonora.

- **THRESHOLD (fader izquierdo)**: de 0 a -20 dB. Bajarlo sube la ganancia que entra al limitador (hasta +20 dB).
- **OUTPUT (fader derecho)**: techo (ceiling) de -12 a 0 dB. Por defecto -0.3 dB.
- **TRUE PEAK**: activa/desactiva la detección de picos entre muestras (interpolación 4x).
- **Medidor LUFS short-term** (ventana de 3 s, ponderación K, ITU-R BS.1770) con historial de 60 s.
- Medidores de gain reduction y de pico de salida. Botón **IN** (bypass).

## Cómo funciona

Dos etapas en serie con lookahead:

1. Etapa principal (lookahead 4 ms): release **dependiente del programa** (30 – 350 ms). Con transitorios aislados se suelta rápido; con limitación sostenida se suelta lento para no bombear los graves.
2. Etapa de seguridad (lookahead 1 ms, release 40 ms) con detector True Peak: atrapa lo que quede y garantiza el techo.

Latencia total: unas 230 muestras (se informa al host). Detector enlazado en estéreo.

## Tipografías

Michroma y Barlow (SIL Open Font License, ver `Fonts/*_OFL.txt`), incluidas dentro del plugin.
