# Dark Level

Plugin VST3 de **gain staging** (Windows / macOS), mismo estilo que Bluey Comp y Dark VCA.

- **GAIN** (perilla grande): -24 a +24 dB, doble clic = 0 dB.
- **Medidor de pico** (dBFS, después de la ganancia): zona objetivo **-12 a -6 dB** en verde.
  Por encima de -6 dB se pone rojo (**TOO HOT**); por debajo de -12 dB queda en ámbar (**TOO LOW**).
- **AUTO**: ajusta la ganancia sola para llevar el pico a -9 dBFS (centro de la zona verde). Con lookahead de 2 ms baja antes de que llegue el pico, así que nunca clipea; sube despacio (~2 dB/s). Es solo ganancia, no colorea. Funciona en mono y estéreo (pico enlazado, no cambia la imagen). Latencia: 2 ms.
- Medidor de -18 a 0 dB. Marca de pico con retención de ~2.5 s y lectura digital. Botón **IN** = bypass.
