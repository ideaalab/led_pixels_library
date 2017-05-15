#include <12F1840.h>
#use delay(clock=32000000)    //clock de 32Mhz

#FUSES INTRC_IO,NOWDT,PUT,NOMCLR,NOPROTECT,NOCPD,BROWNOUT_NOSL,NOCLKOUT,NOIESO,NOFCMEN,NOWRT,PLL_SW,NOSTVREN,BORV25,NODEBUG,NOLVP

#use fast_io(a)               //se accede al puerto a como memoria

#byte PORTA	= getenv("SFR:PORTA")	

//Bits					543210
#define TRIS_A		0b00111110		//define cuales son entradas y cuales salidas
#define WPU_A		0b00001000		//define los weak pull up

#define PIX_PIN		PIN_A0		// O
#bit BTN			= PORTA.3	// I

/* GENERICOS */
#define PULSADO					0			//active low
#define NO_PULSADO				(!PULSADO)

/* CONSTANTES PARA PIXEL LED */
//#define PIX_DELAY_TIMER2			//usamos el timer2 para generar el delay de 50uS
//#define PIX_400KHZ
#define PIX_NUM_LEDS			5		//cuantos leds vamos a usar

/* INCLUDES */
#include "../led_pixels.c"

void main(void) {
int i = 0;

	setup_oscillator(OSC_8MHZ|OSC_PLL_ON);	//configura oscilador interno
	setup_wdt(WDT_OFF);						//configuracion wdt
	setup_timer_0(T0_INTERNAL|T0_DIV_256);	//configuracion timer0
	setup_timer_1(T1_DISABLED);				//configuracion timer1
	setup_timer_2(T2_DISABLED,255,1);		//configuracion timer2
	setup_dac(DAC_OFF);						//configura DAC
	setup_adc(ADC_OFF);						//configura ADC
	setup_ccp1(CCP_OFF);					//configura CCP
	setup_spi(SPI_DISABLED);				//configura SPI
	//setup_uart(FALSE);					//configura UART
	setup_comparator(NC_NC);				//comparador apagado
	setup_vref(VREF_OFF);					//no se usa voltaje de referencia
	set_tris_a(TRIS_A);						//configura I/O
	port_a_pullups(WPU_A);					//configura pullups
	
	InitPixels();	//inicializa pixels
	
	do{
		if(BTN == PULSADO){
			SetPixelColor(0, PIX_ROJO);
			SetPixelColor(1, PIX_NARANJA);
			SetPixelColor(2, PIX_AMARILLO);
			SetPixelColor(3, PIX_VERDE);
			SetPixelColor(4, PIX_CELESTE);
			
			i = i + 0x1F;		//modifico valor
			CambiarBrillo(i);	//actualizo brillo
			MostrarPixels();	//envio nuevos valores a la tira
			
			while(BTN == PULSADO){delay_ms(20);}
		}
	}while(true);
}