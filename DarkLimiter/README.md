# Dark Limiter

Limitador / maximizador de **mastering** VST3 (Windows / macOS), pensado para aguantar mucha presión sonora.

- **THRESHOLD (fader izquierdo)**: de 0 a -20 dB. Bajarlo sube la ganancia que entra al limitador (hasta +20 dB).
- **OUTPUT (fader derecho)**: techo (ceiling) de -12 a 0 dB. Por defecto -0.2 dB.
- **TRUE PEAK**: activa/desactiva la detección de picos entre muestras (interpolación 4x).
- **Medidor LUFS short-term** (ventana de 3 s, ponderación K, ITU-R BS.1770) con historial de 60 s.
- Medidores de gain reduction y de pico de salida. Botón **IN** (bypass).

## Cómo funciona

Cadena de tres pasos (lookahead, detector enlazado en estéreo):

1. **Etapa 1**: limitador de dos constantes de tiempo (lookahead 1.5 ms, release rápido que sigue el recorte necesario + cola lenta de 40 ms), ajustado contra el Ozone IRC II. Sin clipping, con True Peak.
2. **Etapa 2** de seguridad (1 ms, True Peak) que fija el techo.
3. **Etapa de seguridad** (lookahead 1 ms, release 40 ms) con detector True Peak: fija el techo.

Latencia total: unas 570 muestras (se informa al host).

## Tipografías

Michroma y Barlow (SIL Open Font License, ver `Fonts/*_OFL.txt`), incluidas dentro del plugin.
