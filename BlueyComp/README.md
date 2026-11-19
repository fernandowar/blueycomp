# Bluey Comp

Compresor VST3 (Windows / macOS) inspirado en el comportamiento de un 1176 FET, pensado para Studio One.

- **Compress**: una sola perilla. Sube el nivel de entrada contra un umbral fijo y compensa la salida automáticamente.
- **Attack 3** (~0.4 ms), **Release 7** (~60 ms, rápido), **Ratio 4:1**: valores fijos.
- Detector enlazado en estéreo, rodilla suave y color FET leve proporcional al knob.
- Botón **Power** (bypass sin clics) y rejilla inferior que funciona como medidor de gain reduction.

## Compilar en GitHub

Cada push a `main` ejecuta el workflow `Build Bluey Comp` (Actions) y genera los ZIP de Windows y macOS
(descárgalos desde *Artifacts*). Si subes un tag `v1.0.0`, además se crea un Release con los archivos.

## Compilar local

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

El VST3 queda en `build/BlueyComp_artefacts/Release/VST3/`.

## Instalar

- Windows: copiar `Bluey Comp.vst3` a `C:\Program Files\Common Files\VST3\`
- macOS: copiar a `/Library/Audio/Plug-Ins/VST3/` (o `~/Library/Audio/Plug-Ins/VST3/`)

Luego, en Studio One: *Options > Locations > VST Plug-ins > Reset Blacklist* y reescanear.
