/*********************************************************************
 * dht22.c : Direct GPIO access reading DHT22 humidity and temp sensor.
 *********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <setjmp.h>
#include <sys/mman.h>
#include <signal.h>

#include "gpio_io.c"			/* GPIO routines */
#include "timed_wait.c"			/* timed_wait() */

static const int gpio_dht11 = 22;	/* GPIO pin */
static jmp_buf timeout_exit;		/* longjmp on timeout */


/*
 * Read the GPIO line status :
 */
static inline unsigned
gread(void) {
	return gpio_read(gpio_dht11);
}

/*
 * Wait until the GPIO line goes low :
 */
static inline unsigned
wait_until_low(void) {
	const unsigned maxcount = 12000;
	unsigned count = 0;

	while ( gread() )
		if ( ++count >= maxcount )
			longjmp(timeout_exit,1);
	return count;
}

/*
 * Wait until the GPIO line goes high :
 */
static inline unsigned
wait_until_high(void) {
	unsigned count = 0;

	while ( !gread() )
		++count;
	return count;
}

/*
 * Read 1 bit from the DHT11 sensor :
 */
static unsigned
rbit(void) {
	unsigned bias;
	unsigned lo_count, hi_count;

	wait_until_low();
	lo_count = wait_until_high();
	hi_count = wait_until_low();

	bias = lo_count / 3;

	return hi_count + bias > lo_count ? 1 : 0;
}	

/*
 * Read 1 byte from the DHT11 sensor :
 */
static unsigned
rbyte(void) {
	unsigned x, u = 0;

	for ( x=0; x<8; ++x )
		u = (u << 1) | rbit();
	return u;
}

/*
 * Read 32 bits of data + 8 bit checksum from the
 * DHT sensor. Returns relative humidity and
 * temperature in Celcius when successful. The
 * function returns zero if there was a checksum
 * error.
 */
static int
rsensor(float *relhumidity,float *celsius) {
	unsigned char u[5], cs = 0, x;

	for ( x=0; x<5; ++x ) {
		u[x] = rbyte();
		if ( x < 4 )		/* Only checksum data.. */
			cs += u[x];	/* Checksum */
	}

	if ( (cs & 0xFF)  == u[4] ) {

                *relhumidity =  (float)((u[0] << 8) + u[1]) / 10;
                if ( *relhumidity > 100 ) {
		        *relhumidity = (int)u[0];
                }

                *celsius = (float)(((u[2] & 0x7F) << 8) + u[3]) / 10;
                if ( *celsius > 125 ) {
		        *celsius   = (int)u[2];
                }

                if ( u[2] & 0x80 ) {
                        *celsius = - *celsius;
                }

		return 1;
	}
	return 0;
}

/*
 * Main program :
 */
int
main(int argc,char **argv) {
	float relhumidity = 0, celsius = 0;
	int errors = 0, timeouts = 0;
	unsigned wait;

	gpio_init();    			/* Initialize GPIO access */
	gpio_config(gpio_dht11,Input);		/* Set GPIO pin as Input */

	for (;;) {
		if ( setjmp(timeout_exit) ) {	/* Timeouts go here */
			fprintf(stderr,"(Timeout # %d)\n",++timeouts);
			wait = 5;
		} else	wait = 2;

		wait_until_high();		/* Wait GPIO line to go high */
		timed_wait(wait,0,0);		/* Pause for sensor ready */

		gpio_config(gpio_dht11,Output);	/* Output mode */
		gpio_write(gpio_dht11,0);   	/* Bring line low */
		timed_wait(0,30000,0);		/* Hold low min of 18ms */
		gpio_write(gpio_dht11,1);   	/* Bring line high */

		gpio_config(gpio_dht11,Input);	/* Input mode */
		wait_until_low();		/* Wait for low signal */
		wait_until_high();		/* Wait for return to high */

		if ( rsensor(&relhumidity,&celsius) ) {
			printf("RH %.02f%% Temp %.02f C\n",relhumidity,celsius);
                        break;
                } else {
                        fprintf(stderr,"(Error # %d)\n",++errors);
                }
	}

	gpio_config(gpio_dht11,Input);		/* Set pin to input mode */

	return 0;
}

/*********************************************************************
 * End dht11.c
 * Mastering the Raspberry Pi - ISBN13: 978-1-484201-82-4
 * This source code is placed into the public domain.
 *********************************************************************/
