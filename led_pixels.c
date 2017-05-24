#include <led_pixels.h>

/*
 * Inicializa la libreria:
 *	-configura timer2 si es necesario
 *	-Pone el pin de datos en LOW
 *	-Limpia los pixels
 */
void InitPixels(void){
#ifdef PIX_DELAY_TIMER2
	//Configura timer para desbordar cada 50uS, pero no genera interrupcion
	setup_timer_2(T2_DIV_BY_4, 99, 1);
	disable_interrupts(INT_TIMER2);
#endif
	output_low(PIX_PIN);
	LlenarDeColor(0, PIX_NUM_LEDS-1, PIX_NEGRO); //pone todos los pixels en negro
}

/*
 * Pone un led en el color especificado
 * El color debe ser pasado como una variable INT por cada color
 */
void SetPixelColor(int n, int r, int g, int b){
	if(n < PIX_NUM_LEDS){
		if(Brillo != 0){ //Mirar notas en SetBrightness()
			r = ((long)r * Brillo) >> 8;
			g = ((long)g * Brillo) >> 8;
			b = ((long)b * Brillo) >> 8;
		}
		
		int * p;
		p = &Pixels[n * 3];
		
#ifdef PIX_RGB
		//para led RGB
		*p++ = r;
		*p++ = g;
		*p = b;
#else
		//para leds GRB
		*p++ = g;
		*p++ = r;
		*p = b;
#endif
	}
}

/*
 * Pone un led en el color especificado
 * El color debe ser pasado como variable de 32bits
 */
void SetPixelColor(int n, int32 c){
	if(n < PIX_NUM_LEDS){
		int r = (int)(c >> 16);
		int g = (int)(c >>  8);
		int b = (int)c;
		
		if(Brillo != 0){ //Mirar notas en SetBrightness()
			r = ((long)r * Brillo) >> 8;
			g = ((long)g * Brillo) >> 8;
			b = ((long)b * Brillo) >> 8;
		}
		
		int * p;
		p = &Pixels[n * 3];
		
#ifdef PIX_RGB
		//para led RGB
		*p++ = r;
		*p++ = g;
		*p = b;
#else
		//para leds GRB
		*p++ = g;
		*p++ = r;
		*p = b;
#endif
	}
}

/*
 * Convierte los colores separados R,G,B en un paquete de 32 bits con el color
 */
int32 Color32(int r, int g, int b){
	return ((int32)r << 16) | ((int16)g <<  8) | b;
}

/*
 * Crea una "rueda de color" y segun el valor introducido (0-255)
 * devuelve un color completo. El orden es R-G-B-R
 */
int32 Wheel(int WheelPos){
	if(WheelPos < 85){
		return Color32(255-WheelPos*3,WheelPos*3,0);
	}else if(WheelPos < 170){
		WheelPos -= 85;
		return Color32(0,255-WheelPos*3,WheelPos*3);
	}else{
		WheelPos -= 170;
		return Color32(WheelPos*3,0,255-WheelPos*3);
	}
}

/*
 * Devuelve el color del pixel "n" en formato de 32bits y en order RGB
 */
int32 GetPixelColor(int n){
	if(n < PIX_NUM_LEDS) {
		int ofs = n * 3;
		
#ifdef PIX_RGB
		//para led RGB
		return (int32)((Pixels[ofs] << 16) | (Pixels[ofs + 1] <<  8) | (Pixels[ofs + 2]));
#else
		//para leds GRB
		return (int32)((Pixels[ofs + 1] <<  8) | (Pixels[ofs] << 16) | (Pixels[ofs + 2]));
#endif
		
	}

  return 0; //pixel fuera de limites
}

/*
 * Ajusta el brillo de TODOS los leds
 * 0 = completamente apagado, 255 = completamente encendido
 * El brillo no se ve reflejado automaticamente en los leds
 * La proxima llamada a MostrarPixels() actualizara el brillo
 * 
 * Esta funcion genera perdidas de resolucion al aumentar el brillo
 * 
 * No se puede actualizar el brillo al vuelo (mientras se envian los datos)
 * por eso hay que hacerlo previamente diractamente sobre la RAM, lo que
 * modifica permanentemente los datos de color
 * 
 * Si quisieramos modificar el brillo sin perdida de datos necesitariamos
 * otro array con la informacion de todos los leds. En este array estaria
 * la informacion de color sin modificar, y el array que se envia es el que
 * tiene aplicados los ajustes de brillo. De esta manera los ajustes de brillo
 * se harian teniendo en cuenta el array con los colores, pero no se
 * sobreescribirian los valores reales
 * 
 * El valor de brillo almacenado es diferente al valor asignado. De esta manera
 * se optimizan las instrucciones de escalado. La variable "brightness" es un INT.
 * 0 = maximo brillo, 1 = minimo brillo (off), 255 = uno menos a maximo brillo.
 */
void CambiarBrillo(int b){
	int NuevoBrillo = b + 1;
	
	//solo cambiamos el brillo si el nuevo valor es diferente al antiguo
	if(NuevoBrillo != Brillo){
		int Val;							//valor de color, solo es una parte del R/G/B
		int *ptr = Pixels;					//puntero a el array de pixels
		int BrilloAnterior = Brillo - 1;	//brillo anterior (real, de 0 a 255)
		long Escala;						//escala, se multiplicn todos los valores por esta escala
		
		if(BrilloAnterior == 0)
			Escala = 0;						//evitamos division entre 0
		else if (b == 255)
			Escala = 65535 / BrilloAnterior;
		else 
			Escala = (((long)NuevoBrillo << 8) - 1) / BrilloAnterior;
		
		for(int i=0; i<PIX_NUM_BYTES; i++) {
			Val = *ptr;
			*ptr = (Val * Escala) >> 8;
			*ptr++;
		}
		
		Brillo = NuevoBrillo;
	}
}

/*
 * Llena pixels de un mismo color, NO MUESTRA LOS CAMBIOS, hay que llamar a MostrarPixels
 * -from: desde que pixel empezar (empezando desde 0)
 * -to: hasta que pixel (empezando desde 0)
 * -color: color en formato int32 RGB
 */
void LlenarDeColor(int from, int to, int32 c){
int i;
	
	for(i = from; i<=to; i++){
		SetPixelColor(i, c);
	}
}

/*
 * Envia el array de colores por el pin de datos
 */
void MostrarPixels(void){
short GIEval;		//Valor de GIE
int i;				//Loop

	GIEVal = GIE;	//Guardo valor de global interrupt enable
	GIE = 0;		//Deshabilito interrupciones
	
	i = PIX_NUM_BYTES;	//Numero de bytes a enviar
	
#ifdef PIX_DELAY_TIMER2
	//Espero a que hayan transcurrido 50uS antes de volver a enviar
	while(TMR2IF == FALSE){delay_cycles(1);}
#endif

	//Apunto FSR0 al inicio de mis bytes
	FSR0L = Pixels;
	FSR0H = Pixels >> 8;

	//esta instruccion es para que el compilador se mueva al banco correcto antes de
	//empezar con el envio de datos
	output_low(PIX_PIN);

#ifndef PIX_400KHZ
//Envio de datos a 800Khz
//10 instrucciones por cada bit: HHxxxxxLLL
//OUT instructions:              ^ ^    ^   (T=0,2,7)

SendByte1:					//Clk	Instr
	//bit7 ---
	output_high(PIX_PIN);	//0		1
	if(!bit_test(INDF0, 7))	//1		1
		output_low(PIX_PIN);//2		1
	delay_cycles(4);		//3-6	4
	output_low(PIX_PIN);	//7		1
	delay_cycles(2);		//8-9	2
	
	//bit6 ---
	output_high(PIX_PIN);	//0		1
	if(!bit_test(INDF0, 6))	//1		1
		output_low(PIX_PIN);//2		1
	delay_cycles(4);		//3-6	4
	output_low(PIX_PIN);	//7		1
	delay_cycles(2);		//8-9	2
	
	//bit5 ---
	output_high(PIX_PIN);	//0		1
	if(!bit_test(INDF0, 5))	//1		1
		output_low(PIX_PIN);//2		1
	delay_cycles(4);		//3-6	4
	output_low(PIX_PIN);	//7		1
	delay_cycles(2);		//8-9	2
	
	//bit4 ---
	output_high(PIX_PIN);	//0		1
	if(!bit_test(INDF0, 4))	//1		1
		output_low(PIX_PIN);//2		1
	delay_cycles(4);		//3-6	4
	output_low(PIX_PIN);	//7		1
	delay_cycles(2);		//8-9	2
	
	//bit3 ---
	output_high(PIX_PIN);	//0		1
	if(!bit_test(INDF0, 3))	//1		1
		output_low(PIX_PIN);//2		1
	delay_cycles(4);		//3-6	4
	output_low(PIX_PIN);	//7		1
	delay_cycles(2);		//8-9	2
	
	//bit2 ---
	output_high(PIX_PIN);	//0		1
	if(!bit_test(INDF0, 2))	//1		1
		output_low(PIX_PIN);//2		1
	delay_cycles(4);		//3-6	4
	output_low(PIX_PIN);	//7		1
	delay_cycles(2);		//8-9	2
	
	//bit1 ---
	output_high(PIX_PIN);	//0		1
	if(!bit_test(INDF0, 1))	//1		1
		output_low(PIX_PIN);//2		1
	delay_cycles(4);		//3-6	4
	output_low(PIX_PIN);	//7		1
	delay_cycles(2);		//8-9	2
	
	//bit0 ---
	output_high(PIX_PIN);	//0		1
	if(!bit_test(INDF0, 0))	//1		1
		output_low(PIX_PIN);//2		1
#asm
	DECFSZ	i, F			//3		1	decrementar contador de bytes enviados, si es cero salta 1 -> listo.
	GOTO	Salto1			//4		2	salta 1 instruccion (no me deja usar GOTO $+2 ni BRA 2 ¿?)
	GOTO	Listo1			//5		2	todo enviado. Salir
#endasm
Salto1:
	FSR0L++;				//6		1	incremento puntero
	output_low(PIX_PIN);	//7		1	PIN = LOW
	goto SendByte1;			//8		2	vuelve al principio

Listo1:	
	output_low(PIX_PIN);	//7		1
	delay_cycles(2);		//8		2
	//Fin de transmision
	
#else
//Envio de datos a 400Khz
//20 instrucciones por cada bit: HHHHxxxxxxLLLLLLLLLL
//OUT instructions:              ^   ^     ^          (T=0,4,10)

SendByte2:					//Clk	Instr
	//bit7 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 7))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(5);		//5-9	5
	output_low(PIX_PIN);	//10	1
	delay_cycles(9);		//11-19	9
	
	//bit6 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 6))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(5);		//5-9	5
	output_low(PIX_PIN);	//10	1
	delay_cycles(9);		//11-19	9
	
	//bit5 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 5))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(5);		//5-9	5
	output_low(PIX_PIN);	//10	1
	delay_cycles(9);		//11-19	9
	
	//bit4 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 4))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(5);		//5-9	5
	output_low(PIX_PIN);	//10	1
	delay_cycles(9);		//11-19	9
	
	//bit3 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 3))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(5);		//5-9	5
	output_low(PIX_PIN);	//10	1
	delay_cycles(9);		//11-19	9
	
	//bit2 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 2))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(5);		//5-9	5
	output_low(PIX_PIN);	//10	1
	delay_cycles(9);		//11-19	9
	
	//bit1 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 1))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(5);		//5-9	5
	output_low(PIX_PIN);	//10	1
	delay_cycles(9);		//11-19	9
	
	//bit0 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 0))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(5);		//5-9	5
	output_low(PIX_PIN);	//10	1
	
	//delay_cycles(9);		//11-19	9
#asm
	DECFSZ	i, F			//11	1	decrementar contador de bytes enviados, si es cero salta 1 -> listo.
	GOTO	Salto2			//12	2	salta 1 instruccion (no me deja usar GOTO $+2 ni BRA 2 ¿?)
	GOTO	Listo2			//13	2	todo enviado. Salir
#endasm
Salto2:
	FSR0L++;				//14	1	incremento puntero
	delay_cycles(3);		//15-17	3
	goto SendByte2;			//18	2	vuelve al principio

Listo2:	
	delay_cycles(5);		//15-19	2
	//Fin de transmision

#endif	//Fin de envio de datos a 400Khz

#ifdef PIX_DELAY_TIMER2
	//La demora de 50uS se genera con el Timer2
	TMR2 = 0;			//Reinicio contador de Timer2
	TMR2IF = FALSE;		//Quito flag de interrupcion
#else
	//La demora de 50uS se genera con un delay
	delay_us(50);		//espero 50uS para volver a enviar
#endif

	GIE = GIEval;		//restauro valor de GIE
}