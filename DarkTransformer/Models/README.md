# Capturas del transformador

El primer archivo `.nam` de esta carpeta (en orden alfabético) queda **incluido dentro del plugin** al compilar. Si la carpeta no tiene ninguno, el plugin usa el transformador DSP de respaldo.

## Cómo capturar el preamp

1. **Señal de entrenamiento**: baja el archivo de entrada oficial de NAM (en tone3000.com, sección de captura, o en el repo del trainer de NAM). Es un WAV a 48 kHz.
2. **Conexión**: salida de línea de la interfaz → **entrada de LÍNEA** del preamp; salida del preamp → entrada de línea de la interfaz. Nada más en la cadena: sin phantom, sin HPF, sin EQ y sin plugins en el canal de grabación.
3. **Sesión**: 48 kHz, 24 bit. Reproduce el archivo de entrenamiento y graba la salida del preamp en una pista **mono**, desde el mismo inicio.
4. **Niveles**:
   - Ajusta la ganancia del preamp en el punto donde te gusta cómo colorea (el modelo aprende ese punto). Si el preamp tiene varios "sweet spots", haz una captura por cada uno.
   - Lo grabado debe tener picos entre **-6 y -3 dBFS**. Si el conversor de la interfaz clipea, el modelo aprende ese clip.
5. **Exporta** la toma tal cual (mono, 48 kHz, 24 bit) sin normalizar, con el mismo largo que el archivo de entrada.
6. **Entrena** en tone3000.com (gratis) subiendo el archivo de entrada y el grabado. Para una saturación que vas a poner en muchas pistas, elige **A2 / Lite**: un transformador es mucho más simple que un ampli de guitarra y un modelo chico lo captura bien, gastando mucha menos CPU. Apunta a un ESR **menor a 0.01**.
7. **Prueba**: en Dark Transformer, botón **CARGAR** → elige el `.nam`.
8. Cuando te guste, deja el `.nam` en esta carpeta (por ejemplo `Neve-Style-Iron.nam`). El nombre del archivo es el que aparece en el plugin.

Ojo: capturar hardware tuyo es lo limpio. Capturar plugins de otras marcas para vender el resultado puede chocar con sus licencias de uso.
