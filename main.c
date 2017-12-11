
/*////////////////////////////////////////////////////////////////////
///                                                                                                                        \\\
///                                                                                                                            ///
///                 ATtiny13A VU meter by Rafał T.                 \\\
///                                                                ///
/////////////////////////////////////////////////////////////////////
*/
 
//inkludowanie bibliotek
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
 
// makra dla większej przejrzystoci kodu
#define MOSI PB2
#define SCK  PB1
#define CS   PB0
#define SCK_0 (PORTB &=~(1<<SCK))
#define SCK_1 (PORTB |= (1<<SCK))
 
#define MOSI_0 (PORTB &= ~(1<<MOSI))
#define MOSI_1 (PORTB |= (1<<MOSI))
 
#define LATCH_ON (PORTB |= (1<<CS))
#define LATCH_OFF (PORTB &=~(1<<CS))
 
//deklaracje funkcji
// deklaracja fukcji do odczytu z ADC
uint16_t analogRead (uint8_t pin);
 
// deklaracja funkcji do wysłania bajtu po SPI
void SPI_putBYTE(uint8_t bajt);
 
//definicje stałych
 
//ilosc liczb z których liczona jest srednia
#define TIME_BASE 60
 
//definicje zmiennych
 
 
//zmienne do wysłania na rejsetry
int16_t z2;
int16_t z1;
 
//timery
uint8_t counter;
uint8_t counter1;
uint8_t counter2;
uint8_t counter3;
 
//zmienna licznikowa
uint8_t i;
 
//zmienne "doganiające" obliczoną iloć diod
uint8_t doganianieL;
uint8_t doganianieR;
 
//zmienne z którymi porówywane są zmienne doganiające
uint8_t ilosc_diod_L;
uint8_t ilosc_diod_R;
 
//zmienne potrzebne do wyciągnięcia sredniej
uint16_t srednia1;
uint16_t srednia2;
 
//zmienne przechowujące wartoć szczytową
uint8_t peak_L;
uint8_t peak_R;
 
//tablica z wartosciami logarytmów dla 0dB = 710 = 0.77V, skala 0d -27db d0 +3dB
uint16_t log_tab[] =
{
                32,   // -27 dB
                40,   // -25 dB
                50,   // -23 dB
                63,   // -21 dB
                80,   // -19 dB
                100,  // -17 dB
                126,  // -15 dB
                159,  // -13 dB
                200,  // -11 dB
                252,  // -9  dB
                317,  // -7  dB
                399,  // -5  dB
                503,  // -3  dB
                633,  // -1  dB
                710,  //  0  db
                797,  //  1  dB
                1003, //  3  db
};
 
 
 
 
//funkcja główna
int main (void)
{
        ////////////////////////////////////////
        //    inicjalizacja programowego SPI
        ////////////////////////////////////////
        DDRB |= ((1<<MOSI) | (1<<CS) | (1<<SCK));               //Ustaw MOSI, SCK i LATCH jako wyjcia na porcie B
 
 
        ///////////////////////////////////////
        //    inicjalizacja ADC
        ///////////////////////////////////////
        ADMUX  |= (1<<REFS0);                                                   //Wybór napięcia odniesienia - w tym wypadku wewnętrzne = 1,1V
        ADCSRA |= (1<<ADEN);                                                    //włącznie ADC
        ADCSRA |= (1<<ADPS0) | (1<<ADPS1);                              //preskaler na 9600000/2
 
        ///////////////////////////////////////
        //    konfiguracja timera
        ///////////////////////////////////////
 
 
        TCCR0A |= (1<<WGM01);
        TCCR0B |= (1<<CS01) | (1<<CS00);
        TIMSK0 |= (1<<OCIE0A);
        OCR0A = 74;
 
 
        //komandor programista zezwala na przerwania ;p
        sei();
 
 
        while(1)
 
        {
 
 
//              usrednianie pomiaru
 
                srednia1 = srednia1 * TIME_BASE;
                srednia2 = srednia2 * TIME_BASE;
 
                srednia1 = srednia1 + analogRead(0b00000010);
                srednia2 = srednia2 + analogRead(0b00000011);
 
                srednia1 = srednia1 / (TIME_BASE + 1);
                srednia2 = srednia2 / (TIME_BASE + 1);
 
 
 
//wyliczenie ile diod z danego pomiaru ma się zapalić
for ( i = 0; i <= 16; i++ )
{
        if ( srednia1 > log_tab[i] ) ilosc_diod_L = i;
        if ( srednia2 > log_tab[i] ) ilosc_diod_R = i;
}
 
                //jesli wybije czas == counter zmniejsz o 1 ilosc diod do zapalenia
                if (!counter)
                {
 
                        counter = 35;           //czas opadania
 
                        if ( doganianieL > ilosc_diod_L ) doganianieL--;
                        if ( doganianieR > ilosc_diod_R ) doganianieR--;
 
                }
 
 
 
                //jesli wybije czas == counter1 zwiększ o 1 ilosc diod do zapalenia
                if (!counter1)
                {
                        counter1 =      20;  //czas wznoszenia
 
                        if ( doganianieL < ilosc_diod_L ) doganianieL++;
                        if ( doganianieR < ilosc_diod_R ) doganianieR++;
 
                        if (peak_L < doganianieL)                                               //jeżeli wartoć szczytowa jest mniejsza od
                                                                                                                        //ilosci diod do zapalenia to
                        {                                                                                               //wartosc szczytowa = ilosc diod do zapalenia
                                peak_L = doganianieL;
                        }
 
                        if (peak_R < doganianieR )
                        {
                                peak_R = doganianieR;
                        }
                }
 
 
                if (peak_L > doganianieL)                                                  //powolne opadanie wartoci szczytowej
                {
                                if (!counter2)
                                {
                                        counter2 = 200;
                                        peak_L--;
                                }
                }
 
                if (peak_R > doganianieR)
                {
                        if (!counter3)
                        {
                                counter3 = 200;
                                peak_R--;
                        }
                }
 
                                //tu ustawiamy ilosc diod obliczoną wczesniej
                                //poprzez ustawienie odpowiedniej ilosci jedynek w bajtach z1.z2
                                if ( peak_L == 0 ) z1 = ((1<<doganianieL)-1);
                                else z1 = ((1<<doganianieL)-1) | (1<<peak_L);
 
                                if ( peak_R == 0 ) z2 = ((1<<doganianieR)-1);
                                else z2 = ((1<<doganianieR)-1) | (1<<peak_R);
 
 
 
 
                //wyslij wartosc na rejestry przesuwne
                SPI_putBYTE( z1 >> 8); //Wysłanie bajtów danych
                SPI_putBYTE( z1);
                SPI_putBYTE( z2 >> 8);
                SPI_putBYTE( z2);
 
                LATCH_OFF;                                                // latchowanie
                LATCH_ON;
                LATCH_OFF;
 
 
 
}
 
 
        }
 
 
 
 
//wysyłanie bajtu na rejsetry
void SPI_putBYTE(uint8_t bajt)
{
        uint8_t cnt = 0x80;
 
        SCK_0;
 
        while (cnt)
        {
                if ( bajt & cnt ) MOSI_1;
                else MOSI_0;
 
                SCK_1;
 
                SCK_0;
 
                cnt>>= 1;
 
        }
 
 
}
 
//obsługa przerwania
ISR(TIM0_COMPA_vect)
{
        if ( counter  ) counter--;
        if ( counter1 ) counter1--;
        if ( counter2 ) counter2--;
        if ( counter3 ) counter3--;
}
 
 
 
uint16_t analogRead (uint8_t pin)
{
        ADMUX = (0b11111100 & ADMUX) | pin; // WYBÓR PINU Z KTÓREGO CHCEMY MIEĆ POMIAR
 
        ADCSRA |= (1<<ADSC);                            // ZACZNIJ KONWERSJE!
 
        while (ADCSRA & (1<<ADSC));             // czekaj
 
 
        return ADCW;                                            //zwróć ADCW czyli ADCH i ADCL razem;
}