# Dark Transformer

Saturación de transformadores y preamp clase A, VST3 / AU (Windows / macOS). Tiene un solo color, pensado para dar peso, profundidad y contenido armónico. Es todo DSP: no hay que cargar nada ni elegir modelos.

## Controles

- **Drive** (-12 a +24 dB): cuánto empujas el circuito. El nivel se compensa solo.
- **Mix** (0-100 %): mezcla en paralelo. El seco va alineado en latencia, así que no hace filtro peine.
- **Output** (-18 a +18 dB) y botón **IN** (bypass sin clics).
- **Armónicos**: pantalla en vivo con H2 a H7 para el Drive actual.
- **Nivel al hierro**: medidor del pico que entra al circuito.

## El circuito

Por canal, en este orden:

1. Pérdida de sub-graves del transformador de entrada.
2. Con oversampling 4x (FIR de fase lineal):
   1. transformador de entrada (el núcleo satura más en graves);
   2. etapa clase A con sesgo que se mueve con la señal (2º armónico que "respira" con la dinámica);
   3. segunda etapa simétrica (3er armónico al empujar);
   4. transformador de salida;
   5. resonancia de agudos.
3. Curva de profundidad: +1 dB a 70 Hz, -0.6 dB a 350 Hz, +0.7 dB de aire desde 11 kHz.

## Comportamiento analógico

- **L y R no son iguales**: el canal R tiene sus propias tolerancias de componentes. Tiene -0.12 dB de ganancia, el núcleo un poco más duro y más grave, otro punto de trabajo en la clase A y la resonancia de agudos más arriba.
- **Deriva continua**: cada canal deriva lento y por su cuenta. Se mueven el punto de trabajo, la saturación y la frecuencia del núcleo, la resonancia de agudos y la ganancia (±0.1 dB). La deriva combina tres fuentes aleatorias suaves, de 0.1 a 0.7 Hz.
- **El resultado**: el color nunca queda estático. El 2º armónico oscila cerca de ±1 dB en el tiempo y entre canales, y la pantalla de armónicos lo muestra en vivo.

## Medición

Con Drive en 0 y una señal a -18 dBFS, el 2º armónico queda en unos -34 dBc. Al subir el Drive aparecen el 3º, el 4º y el 5º. La latencia es de 61 muestras y se le informa al DAW.
