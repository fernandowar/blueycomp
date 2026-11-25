# Dark Transformer

Saturación de transformadores y preamps clásicos, VST3 / AU (Windows / macOS). Todo es DSP: no hay que cargar capturas ni archivos.

## Controles

- **Voz**: 6 equipos en dos grupos.
  - Transformador:
    - **NICKEL**: núcleo de níquel; abierto, rápido y sutil.
    - **STEEL**: núcleo de acero; graves gordos que saturan antes y agudos redondeados.
    - **VINTAGE**: línea vintage; pares temprano y después subida de 3er armónico.
  - Preamp:
    - **BRIT A**: clase A británico con transformadores de entrada y salida, graves y aire.
    - **US DISC**: discreto americano con op-amp, codo más duro y medios al frente.
    - **TUBE**: válvula; 2º armónico dominante y compresión suave por corriente de reja.
- **Drive** (-12 a +24 dB): cuánto empujas el equipo. El nivel se compensa solo.
- **Mix** (0-100 %): paralelo, con el seco alineado en latencia (sin filtro peine).
- **Output** (-18 a +18 dB) y lámpara **IN** (bypass sin clics).
- **VU "IRON"**: aguja con el pico que entra al equipo (-20 a +3 dBFS) y LED de PEAK sobre 0 dBFS.

## Cómo funciona

Por canal, en este orden:

1. Corte de graves del transformador de entrada.
2. Con oversampling 4x (filtros FIR de fase lineal):
   - el núcleo, que satura más en graves, como el flujo magnético real;
   - la etapa de ganancia (clase A, op-amp discreto o triodo);
   - el transformador de salida;
   - la resonancia de agudos.
3. EQ con la personalidad del equipo.

A nivel bajo la ganancia es unitaria, así que el Drive cambia el color y no el volumen. Cambiar de voz hace un fundido de 15 ms, sin clics. La latencia es de 61 muestras y se le informa al DAW. Usa poca CPU, como para ponerlo en todas las pistas.

Las voces están *inspiradas* en esos tipos de equipos; no son capturas de unidades específicas.

Se compila automático en GitHub Actions junto con los demás plugins.
