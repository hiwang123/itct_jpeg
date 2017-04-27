#include "libs.h"

Bits bits;
FILE *fs, *fd;
Bmp bmp;
Comp comp[4];
Mcu mcu;

void init(char *src, char *dst){
	fs=fopen(src, "rb");
	fd=fopen(dst, "wb+");
	//read bmp height(V), width(H)
	fseek(fs, 18, SEEK_CUR);
	fread(&bmp.H, 1, sizeof(uint32_t), fs);
	fread(&bmp.V, 1, sizeof(uint32_t), fs);
	bmp.p = (Pixel **)malloc((bmp.V+1)*sizeof(Pixel *));
	for(int i=0;i<bmp.V;i++)
		bmp.p[i] = (Pixel *) malloc((bmp.H+1)*sizeof(Pixel));
	fseek(fs, 28, SEEK_CUR);
	// read bmp pixel
	uint8_t R, G, B;
	unsigned char bmppad[3] = {0,0,0};
	for(int i=0;i<bmp.V;i++){
		for(int j=0;j<bmp.H;j++){
			fread(&R, 1, 1, fs);
			fread(&G, 1, 1, fs);
			fread(&B, 1, 1, fs);
			/* bad
			bmp.p[i][j].YCrCb[1] = clip(round(0.299*R + 0.587*G + 0.114*B));
			bmp.p[i][j].YCrCb[2] = clip(round(0.5*R -0.419*G -0.081*B + 128));
			bmp.p[i][j].YCrCb[3] = clip(round(-0.169*R -0.331*G + 0.5*B + 128));
			*/
			bmp.p[i][j].YCrCb[1] = clip(round(0.2126*R + 0.7152*G + 0.0722*B));
			bmp.p[i][j].YCrCb[2] = clip(round(0.615*R -0.55861*G -0.05639*B + 128));
			bmp.p[i][j].YCrCb[3] = clip(round(-0.09991*R -0.33609*G + 0.436*B + 128));
		}
		fread(bmppad,1,(4-(bmp.H*3)%4)%4,fs);
	}

	comp[1].H = comp[1].V = 1;
	comp[2].H = comp[2].V = 1;
	comp[3].H = comp[3].V = 1;

	comp[1].Tq=0; comp[1].Td=0; comp[1].Ta=0;
	comp[2].Tq=1; comp[2].Td=1; comp[2].Ta=1;
	comp[3].Tq=1; comp[3].Td=1; comp[3].Ta=1;
}

void write_uint16_t(uint16_t num){
	uint16_t out;
	out = (num<<8) | (num >>8);
	fwrite(&out, 1, sizeof(uint16_t), fd); 
}
void write_uint8_t(uint8_t num){
	fwrite(&num, 1, sizeof(uint8_t), fd); 
}
void write_uints(uint8_t *ptr, int len){
	fwrite(ptr, 1, len, fd);
}
void write_next_bit(uint8_t b){
	bits.now = (bits.now<<1) | b;
	bits.cnt++;
	if(bits.cnt==8){
		write_uint8_t(bits.now);
		if(bits.now == 0xFF) write_uint8_t(0x00);
		bits.now = 0;
		bits.cnt = 0;
	}
}
void padding_bits(void){
	while(bits.cnt!=0) write_next_bit(0);
}
