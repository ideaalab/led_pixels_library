/* 
 * File:   led_pixels.h
 * Author: Martin
 *
 * Created on 23 de enero de 2017, 20:52
 */

/* ------------------------------- INFORMACION --------------------------------
 * 
 *					WS2811 / WS2812 LED DRIVER
 * 
 * Libreria creada por Martin Andersen para IDEAA Lab el 14/abr/2014
 * Usando compilador CCS v5.064
 * 
 * <http://www.martinandersen.es> / <http://www.ideaalab.com>
 * 
 * Informacion relevante sobre conexionado, funcionamiento y consejos en:
 * <https://learn.adafruit.com/adafruit-neopixel-uberguide/overview>
 * 
 * Basada en libreria Neo_Pixel de Adafruit:
 * <https://github.com/adafruit/Adafruit_NeoPixel>
 * 
 * Rutina de assembler creada con la ayuda del foro TodoPic:
 * <http://www.todopic.com.ar/foros/index.php?topic=42611>
 * 
 * Esta libreria se distribuye como software libre. Se puede modificar
 * y/o redistribuir bajo los terminos de licencia GNU:
 * <http://www.gnu.org/licenses/gpl.html>
 * ---------------------------------------------------------------------------*/

/* -------------------------- INSTRUCCIONES DE USO ----------------------------
 * 
 * Definir el pin a usar con el nombre "PIX_PIN" como #bit, ejemplo:
 * #define PIX_PIN	PIN_A0
 * 
 * Definir la cantidad de LEDs a usar con "PIX_NUM_LEDS":
 * #define PIX_NUM_LEDS	20
 * 
 * Por defecto la libreria envia datos a 800Khz. Hay tiras de LED antiguas que no
 * soportan el envio de datos a 800Khz. Para enviar a 400Khz definir "PIX_400KHZ":
 * #define PIX_400KHZ
 * 
 * Depende del modelo de leds que estemos usando el orden de los colores
 * puede ser RGB o GRB. Para los leds WS2812 el orden es GRB y es el
 * funcionamiento predeterminado. Para leds RGB usar esto:
 * #define PIX_RGB
 * 
 * Entre cada trama de datos deben transcurrir 50uS. Podemos usar un delay,
 * que bloquea la ejecucion del programa o podemos usar el Timer 2 para poder
 * seguir ejecutando instrucciones. Es recomendable usar el Timer 2 para generar
 * el delay y no atascar el programa.
 * La libreria por defecto usa el delay. Si queremos que use el Timer 2
 * declarar la constante "PIX_DELAY_TIMER2":
 * #define PIX_DELAY_TIMER2
 * 
 * Hay que usar FAST_IO para el puerto del pin de datos, si se usa STANDARD_IO
 * se agrega una instruccion extra cada vez que se cambia el pin de estado
 * y esto descontrola los tiempos de envio -> es decir, NO FUNCIONA!
 * #use fast_io(a)
 * ---------------------------------------------------------------------------*/

/* --------------------------------- FUNCIONES --------------------------------
 * 
 * -InitPixels()
 * Inicializa el array de leds
 * 
 * -SetPixelColor(int n, int r, int g, int b)
 * Escribe en el LED (n) el color (r, g, b)
 * 
 * -SetPixelColor(int n, int32 c)
 * Escribe en el LED (n) el color (c)
 * 
 * -Color32(int r, int g, int b)
 * Se le pasan los colores separados y devuelve un color en formato int32
 * 
 * -GetPixelColor(int n)
 * Devuelve el color que hayamos escrito previamente a un LED
 * 
 * -CambiarBrillo(int b)
 * Permite ajustar el brillo de los LEDs sin cambiar su color
 * (esta funcion genera perdida, mirar los comentarios en la funcion)
 * 
 * -LlenarDeColor(int from, int to, int32 c)
 * Llena de un color (c) los pixels comprendidos entre (from) y (to)
 * 
 * -MostrarPixels()
 * Envia la trama de datos a los LEDs. Cualquier otra operacion realizada
 * solo modifica los datos de color >> en la RAM <<, pero hasta que no se utiliza
 * MostrarPixels() no se enviara a los LEDs.
 * CUIDADO se desactivan las interrupciones mientras se envian datos!
 * ---------------------------------------------------------------------------*/

/* --------------------------------- VERSIONES --------------------------------
 * 
 * v0.3.2 (24/Mayo/2017)
 * -Se incluye la funcion Wheel
 * 
 * v0.3.1 (15/Mayo/2017)
 * -se suprime el numero de version del nombre del archivo .c y .h, ahora
 * el control de versiones se hace con GIT
 * -se añade codigo de ejemplo en la carpeta demo
 * 
 * v0.3 (23/Enero/2017)
 * -Se divide la libreria en un archivo .c y otro .h
 * -Se documenta mejor y se estandariza la forma de presentar los datos
 * -Se corrige la funcion SetBrightness(CambiarBrillo) que no funcionaba ¿?
 * -Se cambian nombres a algunas funciones y variables
 * -Se crea la funcion LlenarDeColor
 * -Se cambia la forma de usar el pin de datos
 * 
 * v0.2 (21/Abril/2014)
 * -Mejor manejo de la memoria
 * -Se añade posibilidad de  enviar datos a 400Khz o 800Khz
 * 
 * v0.1 (14/Abril/2014)
 * -Primera version
 * -------------------------------------------------------------------------- */

#ifndef LED_PIXELS_H
#define	LED_PIXELS_H

/* COMPROBACIONES DE COMPATIBILIDAD */
#if getenv("VERSION")<5.064
	//Libreria creada con compilador CCS v5.064
	//Para versiones anteriores comprobar que el codigo
	//generado se adecua a los tiempos requeridos
   #warning "Compilador antiguo, comprueba que el codigo generado es valido"
#endif

//si no se ha definido PIX_400KHZ funciona a 800KHZ por defecto
#ifndef PIX_400KHZ
#define PIX_800KHZ
#endif

#if getenv("CLOCK") == 48000000
	//PIC corriendo a 48MHz (12MHz instruccion)
	//Soporta 800KHz y 400KHz
#elif getenv("CLOCK") == 32000000
	//PIC corriendo a 32MHz (8MHz instruccion)
	//Soporta 800KHz y 400KHz
#elif getenv("CLOCK") == 16000000
	//PIC corriendo a 16MHz (4MHz instruccion)
	#ifdef PIX_800KHZ
		#error "A 16Mhz solo puede enviar datos a 400Khz."
	#else
		#warning "Comprobar que los LEDs funcionan a 400Khz."
	#endif
#else
	#warning "Velocidad no probada. Velocidades soportadas: 16MHz, 32MHz, 48MHz"
#endif

#ifndef PIX_PIN
	#error "PIX_PIN no definido"
#endif

#ifndef PIX_NUM_LEDS
	#error "PIX_NUM_LEDS no definido"
#else
	#if PIX_NUM_LEDS > 85
		//Solo podemos usar un maximo de 85 leds.
		//Cada led usa 3 bytes, y nuestro contador de envio es un INT
		#error "PIX_NUM_LEDS tiene que ser menor a 85"
	#elif (PIX_NUM_LEDS*3) > getenv("RAM")
		#error "Tu PIC no tiene suficiente RAM para tantos LEDs"
	#endif
#endif

/* REGISTROS */
#byte INDF0	= getenv("SFR:INDF0")
#byte FSR0L	= getenv("SFR:FSR0L")
#byte FSR0H	= getenv("SFR:FSR0H")
#byte TMR2	= getenv("SFR:TMR2")
#bit GIE	= getenv("BIT:GIE")
#bit TMR2IF	= getenv("BIT:TMR2IF")

/* DEFINES */
#define PIX_NUM_BYTES	(PIX_NUM_LEDS * 3)

/* COLORES BASICOS */
//¡¡¡ Los colores cambian segun el voltaje que se aplique a los LEDs !!!
#define PIX_NEGRO		0x000000	//negro
#define PIX_ROJO		0xFF0000	//rojo
#define PIX_NARANJA		0xFF6A00	//naranja
#define PIX_AMARILLO	0x7D7D00	//amarillo
#define PIX_VERDE		0x00FF00	//verde
#define PIX_CELESTE		0x1284A5	//celeste
#define PIX_AZUL		0x0000FF	//azul
#define PIX_FUCSIA		0xFF005A	//fucsia
#define PIX_VIOLETA		0xAC00FF	//violeta
#define PIX_BLANCO		0xFFFFFF 	//blanco

/* VARIABLES */
int Pixels[PIX_NUM_BYTES];	//Array donde se guardan los valores de los leds
int Brillo = 0;				//Permite ajustar el brillo sin modificar el color

/* PROTOTIPOS */
void InitPixels(void);
void SetPixelColor(int n, int r, int g, int b);
void SetPixelColor(int n, int32 c);
int32 Color32(int r, int g, int b);
int32 Wheel(int WheelPos);
void CambiarBrillo(int b);
void LlenarDeColor(int from, int to, int32 c);
void MostrarPixels(void);

#endif	/* LED_PIXELS_H */

