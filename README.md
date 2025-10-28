# 🚀 LED Pixels Library

¡Bienvenido a la librería **LED Pixels**! Esta biblioteca está diseñada para controlar tiras de LEDs basadas en los drivers **WS2811/WS2812** desde un PIC a 32 MHz usando CCS C.

---

## 📘 Índice

1. 🔧 Requisitos y Configuración  
2. ⚙️ Macros y Definiciones  
3. ✨ Funciones Principales  
4. 🎨 Colores Predefinidos  
5. 💡 Ejemplos de Uso  
6. 🛠️ Consejos y Buenas Prácticas  
7. 📜 Historial de Versiones  
8. 🔓 Licencia  

---

## 🔧 Requisitos y Configuración

- **Compilador**: CCS C v5.064 o superior.  
- **Frecuencia del PIC**: 32 MHz (400Khz y 800Khz) o 16Mhz (400Khz).  
- **RAM mínima**: `PIX_NUM_LEDS * 3` bytes (máximo 85 LEDs).  
- **I/O rápido**: usar `#use fast_io(a)` para el puerto de datos.  
- **Conexión**:  
  - Pin de datos definido como `PIX_PIN` (ej. `#define PIX_PIN PIN_A0`)  
  - Número de LEDs: `#define PIX_NUM_LEDS 20`  

---

## ⚙️ Macros y Definiciones

```c
#define PIX_NUM_BYTES   (PIX_NUM_LEDS * 3)  // Bytes totales en el buffer
#define PIX_NEGRO       0x000000            // Color negro
#define PIX_ROJO        0xFF0000            // Rojo
#define PIX_NARANJA     0xFF6A00            // Naranja
#define PIX_AMARILLO    0x7D7D00            // Amarillo
#define PIX_VERDE       0x00FF00            // Verde
#define PIX_CELESTE     0x1284A5            // Celeste
#define PIX_AZUL        0x0000FF            // Azul
#define PIX_FUCSIA      0xFF005A            // Fucsia
#define PIX_VIOLETA     0xAC00FF            // Violeta
#define PIX_BLANCO      0xFFFFFF            // Blanco

// Opciones de configuración (activar con #define):
// - PIX_400KHZ         : transmisión a 400 kHz (legacy strips)
// - PIX_RGB            : orden de bytes RGB (por defecto GRB)
// - PIX_DELAY_TIMER2   : usar Timer2 para el reset de 50 µs
```

---

## ✨ Funciones Principales

- `void InitPixels(void)`: Inicializa hardware, timer (opcional), pone pin LOW y limpia los LEDs.  
- `void SetPixelColor(int n, int r, int g, int b)`: Escribe color RGB en el LED número `n`.  
- `void SetPixelColor(int n, int32 c)`: Igual que la anterior, pero con color empaquetado en 32 bits.  
- `int32 Color32(int r, int g, int b)`: Empaqueta R, G, B en un entero de 32 bits.  
- `int32 Wheel(int pos)`: Devuelve un color desde una “rueda” cromática (0–255). Útil para hacer barridos de colores o arcoiris sin cálculos.
- `int32 GetPixelColor(int n)`: Lee el color actual del LED `n` (en RGB).  
- `void CambiarBrillo(int b)`: Ajusta el brillo global (0–255). Esta funcion es "destructiva" ya que modifica el color almacenado sin posibilidad de recuperar el original.  
- `void LlenarDeColor(int from, int to, int32 c)`: Rellena un rango de LEDs con un color.  
- `void MostrarPixels(void)`: Envía el buffer por el pin de datos y deshabilita interrupciones durante la transmisión.  

---

## 🎨 Colores Predefinidos

Utiliza los macros `PIX_ROJO`, `PIX_VERDE`, etc., o crea tus propios colores:

```c
int32 miColor = Color32(128, 64, 255);  // Morado suave
SetPixelColor(0, miColor);
```

---

## 💡 Ejemplos de Uso

```c
#include <led_pixels.h>

// Defines: PIX_PIN, PIX_NUM_LEDS, opcionales PIX_RGB, PIX_400KHZ...

void main() {
    // Configuración del PIC
    #use fast_io(a)
    set_tris_a(0x00);

    InitPixels();                 // Inicializa librería
    LlenarDeColor(0, PIX_NUM_LEDS-1, PIX_AZUL);
    CambiarBrillo(128);           // 50% de brillo
    MostrarPixels();              // Actualiza LEDs

    // Efecto “rainbow”
    for(int i=0; i<255; i++) {
        LlenarDeColor(0, PIX_NUM_LEDS-1, Wheel(i));
        MostrarPixels();
        delay_ms(20);
    }

    while(TRUE);
}
```

---

## 🛠️ Consejos y Buenas Prácticas

- **Interrupciones**: al enviar datos se deshabilitan interrupciones; evítalas durante `MostrarPixels()`.  
- **Brillo**: `CambiarBrillo()` reasigna valores en RAM, provocando pérdida de resolución. Para brillo dinámico sin pérdida, mantén un buffer “original” y uno “modificado”.  
- **Tiempos**: asegúrate de usar FAST_IO, y comprueba los delays si cambias la frecuencia del CPU.  

---

## 🔓 Licencia

Esta librería es **software libre** bajo **GNU GPL**. Puedes modificarla y redistribuirla conforme a http://www.gnu.org/licenses/gpl.html.

---

¡Gracias por usar **LED Pixels**! 🌟
