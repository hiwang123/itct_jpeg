#include "libs.h"

void encode(){
	//SOI
	write_uint16_t(0xFFD8);
	//APP0
	write_uint16_t(0xFFE0);
	write_app0();
	//DQT * 2
	write_dqt();
	//SOF0
	write_uint16_t(0xFFC0);
	write_sof0();
	//DHT * 4
	write_dht();
	//SOS
	write_uint16_t(0xFFDA);
	write_sos();
	//write data
	for(int i=bmp.V-1; i>=0; i-=8)
		for(int j=0; j<bmp.H; j+=8){
			// a MCU
			gen_mcu(i, j);
		}
	//EOF
	padding_bits();
	write_uint16_t(0xFFD9);
}
int main(int argc, char* argv[]){
	assert(argc==3);
	init(argv[1],argv[2]);
	encode();
}
