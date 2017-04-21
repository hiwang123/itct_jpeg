#include "libs.h"

void read_frame(void){
	uint16_t tag;
	//SOI
	read_uint16_t(&tag);
	assert(tag == 0xFFD8);
	//APP0
	read_uint16_t(&tag);
	assert(tag == 0xFFE0);
	read_app0();
	//skip APPn and COM
	read_uint16_t(&tag);
	while((tag>>8)==0xFF && (((tag&0xFF)>=0xE1 && (tag&0xFF)<=0xEF) || ((tag&0xFF)==0xFE)) ){
		uint16_t len;
		read_uint16_t(&len);
		fseek(fp, len-2, SEEK_CUR);
		read_uint16_t(&tag);
	}
	//DQT
	while(tag == 0xFFDB){
		read_dqt();
		read_uint16_t(&tag);
	}
	//SOF0
	assert(tag == 0xFFC0);
	read_sof0();
	//DHT
	read_uint16_t(&tag);
	assert(tag == 0xFFC4);
	while(tag == 0xFFC4){
		read_dht();
		read_uint16_t(&tag);
	}
	//SOS
	assert(tag == 0xFFDA);
	read_sos();
	assert(sof0.Nf == 3 && sos.Ns == 3); 
	//init bmp
	init_bmp(sof0.Y, sof0.X);
	//read data
	int Y = (sof0.Y-1)/(sof0.Vmax*8)+1;
	int X = (sof0.X-1)/(sof0.Hmax*8)+1;
	for(int i=0; i<Y; i++)
		for(int j=0; j<X; j++){
			//a MCU
			read_mcu(i,j);
		}
	//EOI
	read_uint16_t(&tag);
	assert(tag == 0xFFD9);
}

int main(void){
	const char *s="JPEG/teatime.jpg";
	const char *t="test.bmp";
	init(s);
	read_frame();
	//output bmp
	output_bmp(t);
}
