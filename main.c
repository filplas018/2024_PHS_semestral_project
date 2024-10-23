//**********************************************************************
//**********************************************************************
//***                                                                ***
//***                       K O P Y T O   V2.0                       ***
//***                                                                ***
//**********************************************************************
//**********************************************************************


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// !  Prekladat s parametrem --nogcse a --std-c99 !
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// testováno s SDCC 4.0.3

#include <8051.h>
#include <stdio.h>

#define TickPerSec 100
#define NPER ((word)-(20000000L/12/TickPerSec))


// Definice zakladnich typu
typedef unsigned char byte;
typedef unsigned int  word;
typedef unsigned long dword;

// Definice ukazatelu na zacatek programove, datove a externi pameti
__code   byte *CBYTE=0;
__idata  byte *DBYTE=0;
__xdata  byte *XBYTE=0;

// Makro pro prevod dvou osmibitovych hodnot na sestnactibitovou
#define MAKEWORD(a, b)      ((word)(((byte)(a)) | ((word)((byte)(b))) << 8))

// Definice vstupu a vystupu
#define _P0 0x80
#define _P1 0x90
#define _P2 0xa0
#define _P3 0xb0
#define _P4 0xc0

__sbit __at _P3+2 Tlac1;
__sbit __at _P3+3 Tlac2;
__sbit __at _P3+6 Buzzer;
__sbit __at _P4+2 LED1;
__sbit __at _P4+3 LED2;
__sbit __at _P4+4 LED3;

__sbit __at _P1+0 POT;
__sbit __at _P1+1 SMT;
__sbit __at _P1+2 ZAR;
__sbit __at _P1+3 LB_Data;
__sbit __at _P1+4 LB_SCK;
__sbit __at _P1+5 LB_SCL;
__sbit __at _P1+6 LB_RCK;
__sbit __at _P1+7 LB_OE;

// TIMER se odecita v casovem preruseni
volatile word TIMER;
byte LCD_Pos;

/*************************************************************************\
*                  LCD - Liquid Crystal Display                           *
*-------------------------------------------------------------------------*
* Description : Header file for programming of LCD display based on       *
*               HITACHI controller HD44780.                               *
*-------------------------------------------------------------------------*
* Author      : Ceba & www.HW.cz          Email       : ceba@hw.cz        *
* Developed   : 06.02.2002                Last Update : 12.04.2003        *
* Version     : 0.1.0                                                     *
*-------------------------------------------------------------------------*
* Compiler     : ANY                      Version: ANY                    *
* Source file  : LCD.H                                                    *
*-------------------------------------------------------------------------*
* Target system : Charon II. - ATmega128, Quartz 14.7456 MHz              *
*                 LCD SC1602A - 4.bit mode                                *
*-------------------------------------------------------------------------*
*   Instruction     D7  D6  D5  D4  D3  D2  D1  D0                        *
*   ==============================================                        *
*   Display clear   0   0   0   0   0   0   0   1                         *
*   Cursor home     0   0   0   0   0   0   1   *                         *
*   Entry Mode Set  0   0   0   0   0   1  I/D  S                         *
*   Display On/Off  0   0   0   0   1   D   C   B                         *
*   Curs/Disp shift 0   0   0   1  S/C R/L  *   *                         *
*   Function Set    0   0   1   DL  N   F   *   *                         *
*   CG RAM addr set 0   1   ---------Acg---------                         *
*   DD RAM addr set 1   -------------Add---------                         *
*                                                                         *
*   Meaning:                                                              *
*   *     - nonvalid bit                                                  *
*   Acg   - CG RAM address (CHARACTER GENERATOR)                          *
*   Add   - DD RAM address (DATA DISPLAY)                                 *
*   AC    - adress counter                                                *
*                                                                         *
*   I/D   - 1-increment, 0-decrement                                      *
*   S     - 1-display shift, 0-no display shift                           *
*   D     - 1-display ON, 0-display OFF                                   *
*   C     - 1-cursor ON, 0-cursor OFF                                     *
*   B     - 1-blink ON, 0-blink OFF                                       *
*   S/C   - 1-display shift, 0-cursor movement                            *
*   R/L   - 1-right shift, 0-left shift                                   *
*   DL    - 1-8 bits data transfer, 0-4 bits data transfer                *
*   N     - 1-1/16 duty, 0-1/8 or 1/11 duty                               *
*   F     - 1-5x10 dot matrix, 0-5x7 dot matrix                           *
*   BF    - 1-internal operation in progress, 0-display ready             *
*                                                                         *
\*************************************************************************/


#define LCD_RS  0x10                   /* rgister select */
#define LCD_RW  0x20                   /* read/write */
#define LCD_EN  0x40                   /* chip enable */

/* --- LCD commands --- */

#define cmd_lcd_init            0x38

#define cmd_lcd_clear           0x01
#define cmd_lcd_home            0x02

#define cmd_cur_shift_on        0x06
#define cmd_cur_shift_off       0x07

#define cmd_lcd_on              0x0E
#define cmd_lcd_off             0x0A

#define cmd_cur_on              0x0E
#define cmd_cur_off             0x0C

#define cmd_cur_blink_on        0x0F
#define cmd_cur_blink_off       0x0E

#define cmd_cur_left            0x10
#define cmd_cur_right           0x14

#define cmd_scr_left            0x18
#define cmd_scr_right           0x1C

#define cmd_set_cgram_addr      0x40
#define cmd_set_ddram_addr      0x80

void Delay(int pause)
  {
	TIMER=1+pause;
	while(TIMER);
  }

void Pause()
  {
	int i=30;
	while(i--);
  }

byte LCD_State()
{
	byte temp;

	Pause();
	P2=(LCD_RW | 0x0F);
	Pause();
	P2=(LCD_RW |LCD_EN | 0x0F);
	Pause();
	temp=16*(P2&0x0F);
	P2=(LCD_RW | 0x0F);

	Pause();
	P2=(LCD_RW | 0x0F);
	Pause();
	P2=(LCD_RW |LCD_EN | 0x0F);
	Pause();
	temp=temp+P2&0x0F;
	P2=(LCD_RW | 0x0F);

	return (temp);
}

void LCD_SendCmd( byte val )
{
	Pause();
	P2=((val>>4)&0x0F);
	Pause();
	P2= LCD_EN | ((val>>4)&0x0F);
	Pause();
	P2=((val>>4)&0x0F);
	Pause();
	P2=( (val & 0x0F) );
	Pause();
	P2=( LCD_EN | (val & 0x0F ));
	Pause();
	P2=( (val & 0x0F) );

	while(LCD_State()&0x80);
}

void LCD_SendData( byte val )
{
	Pause();
	P2=( LCD_RS | ((val>>4)&0x0F) );
	Pause();
	P2= LCD_RS | LCD_EN | ((val>>4)&0x0F);
	Pause();
	P2=( LCD_RS | ((val>>4)&0x0F) );
	Pause();
	P2=( LCD_RS | (val & 0x0F) );
	Pause();
	P2=( LCD_RS | LCD_EN | (val & 0x0F) );
	Pause();
	P2=( LCD_RS | (val & 0x0F) );

	while(LCD_State()&0x80);
}

/* --- Initializing by Instruction - 4-bit initialization --- */
void LCD_Init(void)
{
  P2=( 0 );      /* set RS, RW and EN low */
					 /* all delays are vital */
  Delay(50);        /* power on delay - wait more than 15 ms */

  P2=( 0x03 );           /* lce enable low */
  Delay(1);                /* wait more than 100us */
  P2=( LCD_EN | 0x03 );  /* lcd enable high */
  Delay(1);                /* wait more than 100us */
  P2=( 0x03 );           /* lce enable low */
  Delay(5);                  /* wait more than 4.1 ms */

  P2=( 0x03 );           /* lcd enable low */
  Delay(1);                /* wait more than 100us */
  P2=( LCD_EN | 0x03 );  /* lcd enable high */
  Delay(1);                /* wait more than 100us */
  P2=( 0x03 );           /* lcd enable low */
  Delay(5);                /* wait more than 100us */

  P2=( 0x03 );           /* lcd enable low */
  Delay(1);                /* wait more than 100us */
  P2=( LCD_EN | 0x03 );  /* lcd enable high */
  Delay(1);                /* wait more than 100us */
  P2=( 0x03 );           /* lcd enable low */
  Delay(5);                /* wait more than 100us */

  P2=( 0x02 );           /* lcd enable low */
  Delay(1);                /* wait more than 100us */
  P2=( LCD_EN | 0x02 );  /* lcd enable high */
  Delay(1);                /* wait more than 100us */
  P2=( 0x02 );           /* lcd enable low */
  Delay(5);                /* wait more than 100us */

  LCD_SendCmd(0x28);   /* 4 bit mode, 1/16 duty, 5x8 font */
  LCD_SendCmd(0x08);   /* display off */
  LCD_SendCmd(0x01);   /* display clear */
  LCD_SendCmd(0x06);   /* entry mode */
  LCD_SendCmd(0x0C);   /* display on, blinking cursor off */
  LCD_Pos=0;
}



int putchar(int c)
  {
   if(c==10)
	 {
	   while((LCD_Pos!=0)&&(LCD_Pos!=40))
		 {
		   LCD_SendData(32);
		   LCD_Pos++;
		   if(LCD_Pos==80) LCD_Pos=0;
		 }
	 }
   else
	 {
	   LCD_SendData(c);
	   LCD_Pos++;
	   if(LCD_Pos==80) LCD_Pos=0;
	 }
	 return 0;
  }



/*****************************************
 **          Nastaveni preruseni        **
 *****************************************/

/* Seriove preruseni a timer1 jsou nastaveny
   na 115200bps, timer0 se nastavuje pomoci TH0.*/
void init_interrupts(void)
  {
	TMOD=0x21;
	TCON=0x50;
	PCON=0x80;
	TL0=(byte)NPER;
	TH0=(byte)(NPER >> 8);
	IE=0x82;
  }


/*****************************************
 **         Obsluha LED baru            **
 *****************************************/
/*
__sbit __at _P1+3 LB_Data;
__sbit __at _P1+4 LB_SCK;
__sbit __at _P1+5 LB_SCL;     RESET
__sbit __at _P1+6 LB_RCK;     PREPIS
__sbit __at _P1+7 LB_OE;
*/

void SetLedBar (word n)
{
	byte i;
	LB_RCK = 0;
	LB_SCL = 0;
	LB_SCL = 1;
	LB_SCK = 0;

	for (i=0; i<10; i++)
	{
		LB_Data = !(n & 1);
		n >>= 1;		// n = n >> 1
		LB_SCK = 1;
		LB_SCK = 0;
	}
	LB_RCK = 1;
	LB_OE = 0;
}


/*****************************************
 **         Obsluha Klavesnice          **
 *****************************************/

__sbit __at _P0+0 KEY_C_1;
__sbit __at _P0+1 KEY_C_2;
__sbit __at _P0+2 KEY_C_3;
__sbit __at _P0+3 KEY_C_4;

__sbit __at _P0+4 KEY_R_1;
__sbit __at _P0+5 KEY_R_2;
__sbit __at _P0+6 KEY_R_3;
__sbit __at _P0+7 KEY_R_4;


char GetKeyboard(void)
{

	P0 = 0xFF;
	KEY_R_1 = 0;
	Pause();
	if (!KEY_C_1) return '1';
	if (!KEY_C_2) return '2';
	if (!KEY_C_3) return '3';
	if (!KEY_C_4) return 'A';

	P0 = 0xFF;
	KEY_R_2 = 0;
	Pause();
	if (!KEY_C_1) return '4';
	if (!KEY_C_2) return '5';
	if (!KEY_C_3) return '6';
	if (!KEY_C_4) return 'B';

	P0 = 0xFF;
	KEY_R_3 = 0;
	Pause();
	if (!KEY_C_1) return '7';
	if (!KEY_C_2) return '8';
	if (!KEY_C_3) return '9';
	if (!KEY_C_4) return 'C';

	P0 = 0xFF;
	KEY_R_4 = 0;
	Pause();
	if (!KEY_C_1) return '*';
	if (!KEY_C_2) return '0';
	if (!KEY_C_3) return '#';
	if (!KEY_C_4) return 'D';

	return -1;
}



/*****************************************
 **           Hlavni program            **
 *****************************************/

byte led_position = 9;  // Dioda na spodní pozici
byte state = 0;         // Stav: 0 - stop, 1 - rotace

// Hlavní program
void main(void)
{
    //init_interrupts();
    LCD_Init();

    //SetLedBar(1 << led_position);  // Rozsvítíme spodní diodu (dioda 9)
    
    // Na displeji zobrazíme aktuální stav
    //LCD_SendCmd(cmd_lcd_clear);    
    printf(" 0 - STOP\n");
    printf(" 1 - START\n");

    // Hlavní smyčka
    while (1)
    {
        // Reakce na tlačítko Tlac1 (START)
        /*if (!Tlac1)
        {
            state = 1;  // Spustíme rotaci
            Buzzer = 1; // Zapneme bzučák
            Delay(100); // Krátká pauza
            Buzzer = 0; // Vypneme bzučák
            
            // Zobrazíme informaci o rotaci
            LCD_SendCmd(cmd_lcd_clear);  
            printf("Rotace diod\n");
            printf(" 1 - START\n");

            Delay(300);  // Odskok pro debounce tlačítka
        }

        // Reakce na tlačítko Tlac2 (STOP)
        if (!Tlac2)
        {
            state = 0;  // Zastavíme rotaci
            Buzzer = 1; // Zapneme bzučák
            Delay(100); // Krátká pauza
            Buzzer = 0; // Vypneme bzučák
            
            // Zobrazení čísla aktuální diody
            LCD_SendCmd(cmd_lcd_clear);
            printf("Dioda c. %d\n", led_position);  
            printf(" 0 - STOP\n");

            Delay(300);  // Odskok pro debounce tlačítka
        }

        // Aktualizace LED bar, pokud je stav rotace
        if (state == 1)
        {
            Delay(390);  // Zpoždění 390 ms (ekvivalent T0 časovače)

            // Posun na další LED
            led_position = (led_position + 1) % 10;  
            SetLedBar(1 << led_position);  // Aktualizace LED
        }*/
    }
}

// Přerušení časovače T0 (každých 390 ms)
void timer0() __interrupt 1
  {
	// Reload, 10ms
	TL0=(byte)NPER;
	TH0=(byte)(NPER >> 8);
	if(TIMER!=0) TIMER--;
  }



/*****************************************
 **         Obsluha casovace 0          **
 *****************************************/


/*void timer0() __interrupt 1
  {
	// Reload, 10ms
	TL0=(byte)NPER;
	TH0=(byte)(NPER >> 8);
	if(TIMER!=0) TIMER--;
  }*/
