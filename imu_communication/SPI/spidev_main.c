#include "./spidev_read.h"

int main(int argc, char *argv[]) {
	
    init_spi(argc, argv);
    
	uint8_t rx[MAX_BUFFER_SIZE] = {0, };
	
    receive(rx, 1); 
    
    int i;
	for (i = 0; i < 10; i++) {
        printf("%i\n", rx[i]);
	}

    close_spi();
	return 0;
}
