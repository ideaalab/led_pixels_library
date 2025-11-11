#include <led_pixels.h>

/*
 * Inicializa la librería:
 * - Configura Timer2 si es necesario (para el reset de 50us)
 * - Pone el pin de datos en LOW
 * - Limpia el buffer de pixels (todos en negro)
 */
void InitPixels(void){
	//Configura timer para desbordar cada 50uS, pero no genera interrupcion
#ifdef PIX_DELAY_TIMER2
	#if getenv("CLOCK") == 48000000
		// Para 48 MHz: 48/4 = 12, divisor 4 → 3MHz, 3MHz*50us=150-1=149
		setup_timer_2(T2_DIV_BY_4, 149, 1);
	#elif getenv("CLOCK") == 32000000
		// Para 32 MHz: 32/4 = 8, divisor 4 → 2MHz, 2MHz*50us=100-1=99
		setup_timer_2(T2_DIV_BY_4, 99, 1);
	#elif getenv("CLOCK") == 24000000
		// Para 24 MHz: 24/4 = 6, divisor 4 → 1.5MHz, 1.5MHz*50us=75-1=74
		setup_timer_2(T2_DIV_BY_4, 74, 1);
	#elif getenv("CLOCK") == 16000000
		// Para 16 MHz: 16/4 = 4, divisor 4 → 1MHz, 1MHz*50us=50-1=49
		setup_timer_2(T2_DIV_BY_4, 49, 1);
	#endif
	disable_interrupts(INT_TIMER2);
#endif
	output_low(PIX_PIN);
	LlenarDeColor(0, PIX_NUM_LEDS-1, PIX_NEGRO); //pone todos los pixels en negro
}

/*
 * Escribe el color RGB en el LED n
 * Parámetros: n = índice, r/g/b = componentes de color
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
 * Escribe el color en el LED n usando un int32 (formato RGB)
 * Parámetros: n = índice, c = color empaquetado
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
 * Empaqueta los componentes R, G, B en un int32 (formato RGB)
 */
int32 Color32(int r, int g, int b){
	return ((int32)r << 16) | ((int16)g <<  8) | b;
}

/*
 * Devuelve un color de la rueda cromática (0-255)
 * Útil para efectos arcoiris y barridos de color
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
 * Lee el color actual del LED n (formato int32 RGB)
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
 * Rellena un rango de LEDs con un color
 * Parámetros: from = primer LED, to = último LED, c = color (int32 RGB)
 * No actualiza los LEDs hasta llamar a MostrarPixels()
 */
void LlenarDeColor(int from, int to, int32 c){
int i;
	
	for(i = from; i<=to; i++){
		SetPixelColor(i, c);
	}
}

/*
 * Envía el buffer de colores por el pin de datos
 * Deshabilita interrupciones durante la transmisión
 * Rutinas de transmisión:
 *   - 400KHz disponible en 16, 24, 32 y 48MHz (principalmente para WS2811)
 *   - 800KHz solo en 32/48MHz (recomendado para WS2812/WS2812B)
 *   - WS2812/WS2812B pueden no funcionar correctamente a 400KHz
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

#if getenv("CLOCK") == 48000000
// ==================== ENVIO DE DATOS A 48MHZ ====================

#ifdef PIX_800KHZ
//Envio de datos a 800Khz (48Mhz clock)
//15 instrucciones por cada bit: HHHHHxxxxxLLLLL
//OUT instructions:              ^    ^    ^      (T=0,5,10)
//Para bit 0: T0H=5*83.33ns=416ns, T0L=10*83.33ns=833ns
//Para bit 1: T1H=10*83.33ns=833ns, T1L=5*83.33ns=416ns


// ===== VERSION EN C (COMENTADA) =====
SendByte48_800:				//Clk	Instr   Tiempo
	//bit7 ---
	output_high(PIX_PIN);	//0		1       83.33ns
	delay_cycles(2);		//1-3	3       250ns
	if(!bit_test(INDF0, 7))	//4		1       83.33ns
		output_low(PIX_PIN);//5		1       83.33ns
	delay_cycles(5);		//6-9	4       333.33ns
	output_low(PIX_PIN);	//10	1       83.33ns
	delay_cycles(4);		//11-14	4       333.33ns
	
	//bit6 ---
	output_high(PIX_PIN);	//0		1       83.33ns
	delay_cycles(2);		//1-3	3       250ns
	if(!bit_test(INDF0, 6))	//4		1       83.33ns
		output_low(PIX_PIN);//5		1       83.33ns
	delay_cycles(5);		//6-9	4       333.33ns
	output_low(PIX_PIN);	//10	1       83.33ns
	delay_cycles(4);		//11-14	4       333.33ns
	
	//bit5 ---
	output_high(PIX_PIN);	//0		1       83.33ns
	delay_cycles(2);		//1-3	3       250ns
	if(!bit_test(INDF0, 5))	//4		1       83.33ns
		output_low(PIX_PIN);//5		1       83.33ns
	delay_cycles(5);		//6-9	4       333.33ns
	output_low(PIX_PIN);	//10	1       83.33ns
	delay_cycles(4);		//11-14	4       333.33ns
	
	//bit4 ---
	output_high(PIX_PIN);	//0		1       83.33ns
	delay_cycles(2);		//1-3	3       250ns
	if(!bit_test(INDF0, 4))	//4		1       83.33ns
		output_low(PIX_PIN);//5		1       83.33ns
	delay_cycles(5);		//6-9	4       333.33ns
	output_low(PIX_PIN);	//10	1       83.33ns
	delay_cycles(4);		//11-14	4       333.33ns
	
	//bit3 ---
	output_high(PIX_PIN);	//0		1       83.33ns
	delay_cycles(2);		//1-3	3       250ns
	if(!bit_test(INDF0, 3))	//4		1       83.33ns
		output_low(PIX_PIN);//5		1       83.33ns
	delay_cycles(5);		//6-9	4       333.33ns
	output_low(PIX_PIN);	//10	1       83.33ns
	delay_cycles(4);		//11-14	4       333.33ns
	
	//bit2 ---
	output_high(PIX_PIN);	//0		1       83.33ns
	delay_cycles(2);		//1-3	3       250ns
	if(!bit_test(INDF0, 2))	//4		1       83.33ns
		output_low(PIX_PIN);//5		1       83.33ns
	delay_cycles(5);		//6-9	4       333.33ns
	output_low(PIX_PIN);	//10	1       83.33ns
	delay_cycles(4);		//11-14	4       333.33ns
	
	//bit1 ---
	output_high(PIX_PIN);	//0		1       83.33ns
	delay_cycles(2);		//1-3	3       250ns
	if(!bit_test(INDF0, 1))	//4		1       83.33ns
		output_low(PIX_PIN);//5		1       83.33ns
	delay_cycles(5);		//6-9	4       333.33ns
	output_low(PIX_PIN);	//10	1       83.33ns
	delay_cycles(4);		//11-14	4       333.33ns
	
	//bit0 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 0))	//3		1
		output_low(PIX_PIN);//4		1
	
#asm
	DECFSZ	i, F			//5		1	decrementar contador de bytes enviados, si es cero salta 1 -> listo.
	GOTO	Salto48_800		//6-7	2	salta 1 instruccion
	GOTO	Listo48_800		//6-7	2	todo enviado. Salir
#endasm

Salto48_800:
	FSR0L++;				//8		1	incrementar puntero
	output_low(PIX_PIN);	//9		1
	goto SendByte48_800;	//10-11	2	vuelve al principio

Listo48_800:
	output_low(PIX_PIN);	//8
	delay_cycles(4);		//6		3
	//Fin de transmisio

// ===== VERSION EN ASM (ACTIVA) =====
// 15 instrucciones por cada bit: HHHHHxxxxxLLLLL
// Para bit 0: T0H=5*83.33ns=416ns, T0L=10*83.33ns=833ns
// Para bit 1: T1H=10*83.33ns=833ns, T1L=5*83.33ns=416ns
/*#asm asis
SendByte48_800:
	; ===== BIT 7 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1 (equivalente: output_high(PIX_PIN))
	NOP						; [1] delay 1 ciclo
	NOP						; [2] delay 1 ciclo
	NOP						; [3] delay 1 ciclo (equivalente: delay_cycles(3))
	BTFSC	INDF0, 7		; [4] test bit 7, skip if clear (equivalente: if(!bit_test(INDF0, 7)))
	GOTO	Skip_B7			; [5] si bit=1, salta clear del pin
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [5] PIX = 0 si bit=0 (equivalente: output_low(PIX_PIN))
Skip_B7:
	NOP						; [6] delay 1 ciclo
	NOP						; [7] delay 1 ciclo
	NOP						; [8] delay 1 ciclo
	NOP						; [9] delay 1 ciclo (equivalente: delay_cycles(4))
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [10] PIX = 0 (equivalente: output_low(PIX_PIN))
	NOP						; [11] delay 1 ciclo
	NOP						; [12] delay 1 ciclo
	NOP						; [13] delay 1 ciclo
	NOP						; [14] delay 1 ciclo (equivalente: delay_cycles(4))
	
	; ===== BIT 6 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	BTFSC	INDF0, 6		; [4] test bit 6
	GOTO	Skip_B6			; [5] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [5] PIX = 0 si bit=0
Skip_B6:
	NOP						; [6] delay
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [10] PIX = 0
	NOP						; [11] delay
	NOP						; [12] delay
	NOP						; [13] delay
	NOP						; [14] delay
	
	; ===== BIT 5 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	BTFSC	INDF0, 5		; [4] test bit 5
	GOTO	Skip_B5			; [5] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [5] PIX = 0 si bit=0
Skip_B5:
	NOP						; [6] delay
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [10] PIX = 0
	NOP						; [11] delay
	NOP						; [12] delay
	NOP						; [13] delay
	NOP						; [14] delay
	
	; ===== BIT 4 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	BTFSC	INDF0, 4		; [4] test bit 4
	GOTO	Skip_B4			; [5] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [5] PIX = 0 si bit=0
Skip_B4:
	NOP						; [6] delay
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [10] PIX = 0
	NOP						; [11] delay
	NOP						; [12] delay
	NOP						; [13] delay
	NOP						; [14] delay
	
	; ===== BIT 3 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	BTFSC	INDF0, 3		; [4] test bit 3
	GOTO	Skip_B3			; [5] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [5] PIX = 0 si bit=0
Skip_B3:
	NOP						; [6] delay
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [10] PIX = 0
	NOP						; [11] delay
	NOP						; [12] delay
	NOP						; [13] delay
	NOP						; [14] delay
	
	; ===== BIT 2 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	BTFSC	INDF0, 2		; [4] test bit 2
	GOTO	Skip_B2			; [5] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [5] PIX = 0 si bit=0
Skip_B2:
	NOP						; [6] delay
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [10] PIX = 0
	NOP						; [11] delay
	NOP						; [12] delay
	NOP						; [13] delay
	NOP						; [14] delay
	
	; ===== BIT 1 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	BTFSC	INDF0, 1		; [4] test bit 1
	GOTO	Skip_B1			; [5] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [5] PIX = 0 si bit=0
Skip_B1:
	NOP						; [6] delay
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [10] PIX = 0
	NOP						; [11] delay
	NOP						; [12] delay
	NOP						; [13] delay
	NOP						; [14] delay
	
	; ===== BIT 0 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	BTFSC	INDF0, 0		; [4] test bit 0
	GOTO	Skip_B0			; [5] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [5] PIX = 0 si bit=0
Skip_B0:
	NOP						; [6] delay
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [10] PIX = 0
	
	; ===== CONTROL DE LOOP =====
	DECFSZ	i, F			; [11] decrementar contador (equivalente: decrementar i)
	GOTO	Salto48_800		; [12] si no es cero, continuar
	GOTO	Listo48_800		; [13] si es cero, terminar

Salto48_800:
	INCF	FSR0L, F		; [14] incrementar puntero (equivalente: FSR0L++)
	GOTO	SendByte48_800	; [0-1] volver al inicio

Listo48_800:
	NOP						; [14] delay
	NOP						; [15] delay
	NOP						; [16] delay (equivalente: delay_cycles(3))
	; Fin de transmision
#endasm*/

#else
//Envio de datos a 400Khz (48Mhz clock)
//30 instrucciones por cada bit: HHHHHHxxxxxxLLLLLLLLLLLLLLLL
//OUT instructions:              ^     ^     ^               (T=0,6,12)

/*
// ===== VERSION EN C (COMENTADA) =====
SendByte48_400:				//Clk	Instr   Tiempo
	//bit7 ---
	output_high(PIX_PIN);	//0		1       83.33ns
	delay_cycles(4);		//1-4	4       333.33ns
	if(!bit_test(INDF0, 7))	//5		1       83.33ns
		output_low(PIX_PIN);//6		1       83.33ns
	delay_cycles(5);		//7-11	5       416.67ns
	output_low(PIX_PIN);	//12		1       83.33ns
	delay_cycles(17);		//13-29	17      1.41us
	
	//bit6 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(4);		//1-4	4
	if(!bit_test(INDF0, 6))	//5		1
		output_low(PIX_PIN);//6		1
	delay_cycles(5);		//7-11	5
	output_low(PIX_PIN);	//12		1
	delay_cycles(17);		//13-29	17
	
	//bit5 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(4);		//1-4	4
	if(!bit_test(INDF0, 5))	//5		1
		output_low(PIX_PIN);//6		1
	delay_cycles(5);		//7-11	5
	output_low(PIX_PIN);	//12		1
	delay_cycles(17);		//13-29	17
	
	//bit4 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(4);		//1-4	4
	if(!bit_test(INDF0, 4))	//5		1
		output_low(PIX_PIN);//6		1
	delay_cycles(5);		//7-11	5
	output_low(PIX_PIN);	//12		1
	delay_cycles(17);		//13-29	17
	
	//bit3 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(4);		//1-4	4
	if(!bit_test(INDF0, 3))	//5		1
		output_low(PIX_PIN);//6		1
	delay_cycles(5);		//7-11	5
	output_low(PIX_PIN);	//12		1
	delay_cycles(17);		//13-29	17
	
	//bit2 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(4);		//1-4	4
	if(!bit_test(INDF0, 2))	//5		1
		output_low(PIX_PIN);//6		1
	delay_cycles(5);		//7-11	5
	output_low(PIX_PIN);	//12		1
	delay_cycles(17);		//13-29	17
	
	//bit1 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(4);		//1-4	4
	if(!bit_test(INDF0, 1))	//5		1
		output_low(PIX_PIN);//6		1
	delay_cycles(5);		//7-11	5
	output_low(PIX_PIN);	//12		1
	delay_cycles(17);		//13-29	17
	
	//bit0 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(4);		//1-4	4
	if(!bit_test(INDF0, 0))	//5		1
		output_low(PIX_PIN);	//6		1
	delay_cycles(5);		//7-11	5
	output_low(PIX_PIN);	//12		1
	
#asm
	DECFSZ	i, F			//13	1	decrementar contador de bytes enviados, si es cero salta 1 -> listo.
	GOTO	Salto48_400		//14	2	salta 1 instruccion
	GOTO	Listo48_400		//15	2	todo enviado. Salir
#endasm
Salto48_400:
	FSR0L++;				//16	1	incrementar puntero
	delay_cycles(12);		//17-28	12
	goto SendByte48_400;	//29		2	vuelve al principio

Listo48_400:	
	delay_cycles(16);		//14-29	16
*/

// ===== VERSION EN ASM (ACTIVA) =====
// 30 instrucciones por cada bit: HHHHHHxxxxxxLLLLLLLLLLLLLLLL
// Para bit 0: T0H=6*83.33ns=500ns, T0L=24*83.33ns=2us
// Para bit 1: T1H=18*83.33ns=1.5us, T1L=12*83.33ns=1us
#asm asis
SendByte48_400:
	; ===== BIT 7 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	NOP						; [4] delay
	BTFSC	INDF0, 7		; [5] test bit 7
	GOTO	Skip_B7_400		; [6] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [6] PIX = 0 si bit=0
Skip_B7_400:
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	NOP						; [10] delay
	NOP						; [11] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [12] PIX = 0
	NOP						; [13] delay
	NOP						; [14] delay
	NOP						; [15] delay
	NOP						; [16] delay
	NOP						; [17] delay
	NOP						; [18] delay
	NOP						; [19] delay
	NOP						; [20] delay
	NOP						; [21] delay
	NOP						; [22] delay
	NOP						; [23] delay
	NOP						; [24] delay
	NOP						; [25] delay
	NOP						; [26] delay
	NOP						; [27] delay
	NOP						; [28] delay
	NOP						; [29] delay
	
	; ===== BIT 6 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	NOP						; [4] delay
	BTFSC	INDF0, 6		; [5] test bit 6
	GOTO	Skip_B6_400		; [6] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [6] PIX = 0 si bit=0
Skip_B6_400:
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	NOP						; [10] delay
	NOP						; [11] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [12] PIX = 0
	NOP						; [13] delay
	NOP						; [14] delay
	NOP						; [15] delay
	NOP						; [16] delay
	NOP						; [17] delay
	NOP						; [18] delay
	NOP						; [19] delay
	NOP						; [20] delay
	NOP						; [21] delay
	NOP						; [22] delay
	NOP						; [23] delay
	NOP						; [24] delay
	NOP						; [25] delay
	NOP						; [26] delay
	NOP						; [27] delay
	NOP						; [28] delay
	NOP						; [29] delay
	
	; ===== BIT 5 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	NOP						; [4] delay
	BTFSC	INDF0, 5		; [5] test bit 5
	GOTO	Skip_B5_400		; [6] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [6] PIX = 0 si bit=0
Skip_B5_400:
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	NOP						; [10] delay
	NOP						; [11] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [12] PIX = 0
	NOP						; [13] delay
	NOP						; [14] delay
	NOP						; [15] delay
	NOP						; [16] delay
	NOP						; [17] delay
	NOP						; [18] delay
	NOP						; [19] delay
	NOP						; [20] delay
	NOP						; [21] delay
	NOP						; [22] delay
	NOP						; [23] delay
	NOP						; [24] delay
	NOP						; [25] delay
	NOP						; [26] delay
	NOP						; [27] delay
	NOP						; [28] delay
	NOP						; [29] delay
	
	; ===== BIT 4 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	NOP						; [4] delay
	BTFSC	INDF0, 4		; [5] test bit 4
	GOTO	Skip_B4_400		; [6] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [6] PIX = 0 si bit=0
Skip_B4_400:
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	NOP						; [10] delay
	NOP						; [11] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [12] PIX = 0
	NOP						; [13] delay
	NOP						; [14] delay
	NOP						; [15] delay
	NOP						; [16] delay
	NOP						; [17] delay
	NOP						; [18] delay
	NOP						; [19] delay
	NOP						; [20] delay
	NOP						; [21] delay
	NOP						; [22] delay
	NOP						; [23] delay
	NOP						; [24] delay
	NOP						; [25] delay
	NOP						; [26] delay
	NOP						; [27] delay
	NOP						; [28] delay
	NOP						; [29] delay
	
	; ===== BIT 3 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	NOP						; [4] delay
	BTFSC	INDF0, 3		; [5] test bit 3
	GOTO	Skip_B3_400		; [6] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [6] PIX = 0 si bit=0
Skip_B3_400:
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	NOP						; [10] delay
	NOP						; [11] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [12] PIX = 0
	NOP						; [13] delay
	NOP						; [14] delay
	NOP						; [15] delay
	NOP						; [16] delay
	NOP						; [17] delay
	NOP						; [18] delay
	NOP						; [19] delay
	NOP						; [20] delay
	NOP						; [21] delay
	NOP						; [22] delay
	NOP						; [23] delay
	NOP						; [24] delay
	NOP						; [25] delay
	NOP						; [26] delay
	NOP						; [27] delay
	NOP						; [28] delay
	NOP						; [29] delay
	
	; ===== BIT 2 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	NOP						; [4] delay
	BTFSC	INDF0, 2		; [5] test bit 2
	GOTO	Skip_B2_400		; [6] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [6] PIX = 0 si bit=0
Skip_B2_400:
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	NOP						; [10] delay
	NOP						; [11] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [12] PIX = 0
	NOP						; [13] delay
	NOP						; [14] delay
	NOP						; [15] delay
	NOP						; [16] delay
	NOP						; [17] delay
	NOP						; [18] delay
	NOP						; [19] delay
	NOP						; [20] delay
	NOP						; [21] delay
	NOP						; [22] delay
	NOP						; [23] delay
	NOP						; [24] delay
	NOP						; [25] delay
	NOP						; [26] delay
	NOP						; [27] delay
	NOP						; [28] delay
	NOP						; [29] delay
	
	; ===== BIT 1 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	NOP						; [4] delay
	BTFSC	INDF0, 1		; [5] test bit 1
	GOTO	Skip_B1_400		; [6] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [6] PIX = 0 si bit=0
Skip_B1_400:
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	NOP						; [10] delay
	NOP						; [11] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [12] PIX = 0
	NOP						; [13] delay
	NOP						; [14] delay
	NOP						; [15] delay
	NOP						; [16] delay
	NOP						; [17] delay
	NOP						; [18] delay
	NOP						; [19] delay
	NOP						; [20] delay
	NOP						; [21] delay
	NOP						; [22] delay
	NOP						; [23] delay
	NOP						; [24] delay
	NOP						; [25] delay
	NOP						; [26] delay
	NOP						; [27] delay
	NOP						; [28] delay
	NOP						; [29] delay
	
	; ===== BIT 0 =====
	BSF		PIX_ASM_PORT, PIX_ASM_BIT		; [0] PIX = 1
	NOP						; [1] delay
	NOP						; [2] delay
	NOP						; [3] delay
	NOP						; [4] delay
	BTFSC	INDF0, 0		; [5] test bit 0
	GOTO	Skip_B0_400		; [6] skip if bit=1
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [6] PIX = 0 si bit=0
Skip_B0_400:
	NOP						; [7] delay
	NOP						; [8] delay
	NOP						; [9] delay
	NOP						; [10] delay
	NOP						; [11] delay
	BCF		PIX_ASM_PORT, PIX_ASM_BIT		; [12] PIX = 0
	NOP						; [13] delay
	NOP						; [14] delay
	NOP						; [15] delay
	NOP						; [16] delay
	NOP						; [17] delay
	NOP						; [18] delay
	NOP						; [19] delay
	NOP						; [20] delay
	NOP						; [21] delay
	NOP						; [22] delay
	NOP						; [23] delay
	NOP						; [24] delay
	NOP						; [25] delay
	NOP						; [26] delay
	NOP						; [27] delay
	NOP						; [28] delay
	
	; ===== CONTROL DE LOOP =====
	DECFSZ	@i, F			; [29] decrementar contador
	GOTO	Salto48_400		; [0-1] si no es cero, continuar
	GOTO	Listo48_400		; [0-1] si es cero, terminar

Salto48_400:
	INCF	FSR0L, F		; [2] incrementar puntero
	GOTO	SendByte48_400	; [3-4] volver al inicio

Listo48_400:
	NOP						; [2] delay
	NOP						; [3] delay
	NOP						; [4] delay
	; Fin de transmision
#endasm

#endif	//Fin de envio de datos a 48Mhz

#elif getenv("CLOCK") == 24000000
// ==================== ENVIO DE DATOS A 24MHZ (400KHZ) ====================
//15 instrucciones por cada bit: HHHHxxxxxxLLLLLL
//OUT instructions:              ^   ^     ^      (T=0,4,9)
//1 ciclo = 166.667ns, por lo que:
//T0H = 4 ciclos = 666.67ns (WS2811 spec: 500ns±150ns)
//T1H = 9 ciclos = 1.5us (WS2811 spec: 1.2us±150ns)
//T0L = 11 ciclos = 1.833us
//T1L = 6 ciclos = 1us
//Total = 2.5us por bit = 400KHz

SendByte24_400:				//Clk	Instr
	//bit7 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 7))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(4);		//5-8	4
	output_low(PIX_PIN);	//9		1
	delay_cycles(5);		//10-14	5
	
	//bit6 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 6))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(4);		//5-8	4
	output_low(PIX_PIN);	//9		1
	delay_cycles(5);		//10-14	5
	
	//bit5 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 5))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(4);		//5-8	4
	output_low(PIX_PIN);	//9		1
	delay_cycles(5);		//10-14	5
	
	//bit4 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 4))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(4);		//5-8	4
	output_low(PIX_PIN);	//9		1
	delay_cycles(5);		//10-14	5
	
	//bit3 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 3))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(4);		//5-8	4
	output_low(PIX_PIN);	//9		1
	delay_cycles(5);		//10-14	5
	
	//bit2 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 2))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(4);		//5-8	4
	output_low(PIX_PIN);	//9		1
	delay_cycles(5);		//10-14	5
	
	//bit1 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 1))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(4);		//5-8	4
	output_low(PIX_PIN);	//9		1
	delay_cycles(5);		//10-14	5
	
	//bit0 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(2);		//1-2	2
	if(!bit_test(INDF0, 0))	//3		1
		output_low(PIX_PIN);//4		1
	delay_cycles(4);		//5-8	4
	output_low(PIX_PIN);	//9		1
	
#asm
	DECFSZ	i, F			//10	1	decrementar contador de bytes enviados
	GOTO	Salto24_400		//11	2	salta 1 instruccion
	GOTO	Listo24_400		//12	2	todo enviado. Salir
#endasm
Salto24_400:
	FSR0L++;				//13	1	incrementar puntero
	delay_cycles(1);		//14	1
	goto SendByte24_400;	//0		2	vuelve al principio

Listo24_400:	
	delay_cycles(4);		//13-14	2
	//Fin de transmision

#elif (defined(PIX_800KHZ) || (getenv("CLOCK") == 16000000))
// ==================== ENVIO DE DATOS A 32MHZ/16MHZ ====================
//Envio de datos a 800KHz (32MHz clock) o a 400KHz (16MHz clock)
//La secuencia de instrucciones es la misma en ambos casos
//10 instrucciones por cada bit: HHxxxxxLLL
//OUT instructions:              ^ ^    ^   (T=0,2,7)
//Para 32MHz (800KHz):
//1 ciclo = 125ns, por lo que:
//T0H = 3 ciclos = 375ns
//T0L = 7 ciclos = 875ns
//T1H = 7 ciclos = 875ns 
//T1L = 3 ciclos = 375ns
//Total = 1.25us por bit = 800KHz

//Para 16MHz (400KHz):
//1 ciclo = 250ns, por lo que:
//T0H = 2 ciclos = 500ns (WS2811 spec: 500ns±150ns)
//T1H = 7 ciclos = 1.75us (WS2811 spec: 1.2us±150ns)
//Total = 2.5us por bit = 400KHz

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
//Envio de datos a 400Khz (32Mhz clock)
//20 instrucciones por cada bit: HHHHHHxxxxxxLLLLLLLLLL
//OUT instructions:              ^     ^     ^          (T=0,6,14)
//270/1.25  900/1.25
//540 -1800

#warning crear funcion en asm con bucle, ya que los tiempos son menos criticos
SendByte2:					//Clk	Instr
	//bit7 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(1);		//1-4	4
	if(!bit_test(INDF0, 7))	//5		1
		output_low(PIX_PIN);//6		1
	delay_cycles(7);		//7-13	7
	output_low(PIX_PIN);	//14	1
	delay_cycles(5);		//15-19	5
	
	//bit6 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(1);		//1-4	4
	if(!bit_test(INDF0, 6))	//5		1
		output_low(PIX_PIN);//6		1
	delay_cycles(7);		//7-13	7
	output_low(PIX_PIN);	//14	1
	delay_cycles(5);		//15-19	5
	
	//bit5 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(1);		//1-4	4
	if(!bit_test(INDF0, 5))	//5		1
		output_low(PIX_PIN);//6		1
	delay_cycles(7);		//7-13	7
	output_low(PIX_PIN);	//14	1
	delay_cycles(5);		//15-19	5
	
	//bit4 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(1);		//1-4	4
	if(!bit_test(INDF0, 4))	//5		1
		output_low(PIX_PIN);//6		1
	delay_cycles(7);		//7-13	7
	output_low(PIX_PIN);	//14	1
	delay_cycles(5);		//15-19	5
	
	//bit3 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(1);		//1-4	4
	if(!bit_test(INDF0, 3))	//5		1
		output_low(PIX_PIN);//6		1
	delay_cycles(7);		//7-13	7
	output_low(PIX_PIN);	//14	1
	delay_cycles(5);		//15-19	5
	
	//bit2 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(1);		//1-4	4
	if(!bit_test(INDF0, 2))	//5		1
		output_low(PIX_PIN);//6		1
	delay_cycles(7);		//7-13	7
	output_low(PIX_PIN);	//14	1
	delay_cycles(5);		//15-19	5
	
	//bit1 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(1);		//1-4	4
	if(!bit_test(INDF0, 1))	//5		1
		output_low(PIX_PIN);//6		1
	delay_cycles(7);		//7-13	7
	output_low(PIX_PIN);	//14	1
	delay_cycles(5);		//15-19	5
	
	//bit0 ---
	output_high(PIX_PIN);	//0		1
	delay_cycles(1);		//1-4	4
	if(!bit_test(INDF0, 7))	//5		1
		output_low(PIX_PIN);//6		1

#asm
	DECFSZ	i, F			//7		1	decrementar contador de bytes enviados, si es cero salta 1 -> listo.
	GOTO	Salto2			//8-9	2	salta 1 instruccion (no me deja usar GOTO $+2 ni BRA 2 ¿?)
	GOTO	Listo2			//8-9	2	todo enviado. Salir
#endasm
Salto2:
	FSR0L++;				//10	1	incremento puntero
	delay_cycles(1);		//11-13	3
	output_low(PIX_PIN);	//14	1
	delay_cycles(2);		//15-17	3
	goto SendByte2;			//18-19	2	vuelve al principio

Listo2:	
	delay_cycles(2);		//15-19	2
	//Fin de transmision

#endif	//Fin de envio de datos a 400Khz	//Fin de envio de datos a 24MHz

#ifdef PIX_DELAY_TIMER2
	//La demora de 50uS se genera con el Timer2
	TMR2 = 0;			//Reinicio contador de Timer2
	TMR2IF = FALSE;		//Quito flag de interrupcion
#else
	//La demora de 50uS se genera con un delay
	delay_us(50);		//espero 50uS para volver a enviar
#endif	//Fin de la estructura principal de seleccion de frecuencia

	GIE = GIEval;		//restauro valor de GIE
}
