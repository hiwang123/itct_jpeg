#include "libs.h"

App0 app0;
Dqt dqt;
Sof0 sof0;
Dht dht[4];
Sos sos;
Mcu mcu;
Bits bits;
FILE* fp;
Bmp bmp;
double c[8];

void init_bmp(int V,int H){ //vertical, horizontal
	bmp.V = V; bmp.H = H;
	bmp.p = (Pixel **)malloc((V+1)*sizeof(Pixel *));
	for(int i=0;i<V;i++)
		bmp.p[i] = (Pixel *) malloc((H+1)*sizeof(Pixel));
}
void init_idct(double c[8]){
	double PI = acos(-1.0);
	for(int i=1;i<=7;i++)
		c[i] = cos((PI*i)/16);
}
void init(char *filename){
	fp = fopen(filename, "rb");
	memset(&app0, 0, sizeof(App0));
	memset(&dqt, 0, sizeof(Dqt));
	memset(&sof0, 0, sizeof(Sof0));
	memset(&dht[0], 0, sizeof(Dht));
	memset(&dht[1], 0, sizeof(Dht));
	memset(&dht[2], 0, sizeof(Dht));
	memset(&dht[3], 0, sizeof(Dht));
	memset(&sos, 0, sizeof(Sos));
	memset(&mcu, 0, sizeof(Mcu));
	memset(&bits, 0, sizeof(Bits));
	init_idct(c);
}
void read_uint8_t(uint8_t* ptr){
	fread(ptr, 1, 1, fp);
}
void read_uint16_t(uint16_t* ptr){
	uint16_t num;
	fread(&num, 1, 2, fp);  
	*ptr = (num<<8) | (num>>8);
}
void read_uints(uint8_t **ptr, int size){
	*ptr = (uint8_t *)malloc(size+1);
	fread(*ptr, 1, size, fp);
}
uint8_t get_next_bit(){
	if(bits.cnt==0){
		read_uint8_t(&bits.now);
		if(bits.now==0xFF){
			uint8_t nxt;
			read_uint8_t(&nxt);
		}
		bits.cnt=8;
	}
	return (bits.now>>(--bits.cnt))&1;
}
void fill_pixel(int st_v, int st_h, int scale_v, int scale_h, int chnl,int val){
	st_v*=scale_v;
	st_h*=scale_h;
	val += 128;     // left shift(+128)

	for(int i=0;i<scale_v;i++)
		for(int j=0;j<scale_h;j++){
			if(st_v+i>=bmp.V || st_h+j>=bmp.H) continue;
			bmp.p[st_v+i][st_h+j].YCrCb[chnl] = val;
		}
}
void output_bmp(char *filename){
	//ref: http://stackoverflow.com/questions/2654480/writing-bmp-image-in-pure-c-c-without-other-libraries
	FILE *fout;
	fout = fopen(filename, "wb+");
	fseek(fout, 0, 0);
	int filesize = 54 + 3*bmp.V*bmp.H;
	unsigned char bmpfileheader[14] = {'B','M', 0,0,0,0, 0,0, 0,0, 54,0,0,0};
	unsigned char bmpinfoheader[40] = {40,0,0,0, 0,0,0,0, 0,0,0,0, 1,0, 24,0};
	unsigned char bmppad[3] = {0,0,0};
	bmpfileheader[ 2] = (unsigned char)(filesize);
	bmpfileheader[ 3] = (unsigned char)(filesize>> 8);
	bmpfileheader[ 4] = (unsigned char)(filesize>>16);
	bmpfileheader[ 5] = (unsigned char)(filesize>>24);

	bmpinfoheader[ 4] = (unsigned char)(       bmp.H    );
	bmpinfoheader[ 5] = (unsigned char)(       bmp.H>> 8);
	bmpinfoheader[ 6] = (unsigned char)(       bmp.H>>16);
	bmpinfoheader[ 7] = (unsigned char)(       bmp.H>>24);
	bmpinfoheader[ 8] = (unsigned char)(       bmp.V    );
	bmpinfoheader[ 9] = (unsigned char)(       bmp.V>> 8);
	bmpinfoheader[10] = (unsigned char)(       bmp.V>>16);
	bmpinfoheader[11] = (unsigned char)(       bmp.V>>24);
	
	fwrite(bmpfileheader,1,14,fout);
	fwrite(bmpinfoheader,1,40,fout);
	
	for(int i=bmp.V-1;i>=0; i--){
		uint8_t *out;
		out = (uint8_t*) malloc(3*bmp.H);
		for(int j=0;j<bmp.H; j++){
			 double Y = bmp.p[i][j].YCrCb[1],Cr = bmp.p[i][j].YCrCb[2], Cb = bmp.p[i][j].YCrCb[3];

			 double R = Y + 1.28033 * (Cr-128);
             double G = Y - 0.21482 * (Cb-128) - 0.38059 * (Cr-128);
             double B = Y + 2.12798 * (Cb-128);
			 /* ok
			 double R = Y + 1.13983 * (Cr-128);
             double G = Y - 0.39465 * (Cb-128) - 0.58060 * (Cr-128);
             double B = Y + 2.03211 * (Cb-128);
             */
             /* bad
             uint8_t R = Y + 1.402 * (Cr-128);
             uint8_t G = Y - 0.344 * (Cb-128) - 0.714 * (Cr-128);
             uint8_t B = Y + 1.772 * (Cb-128);
             */
             out[3*j]=clip(round(R)), out[3*j+1]=clip(round(G)), out[3*j+2]=clip(round(B));
		}
		fwrite(out,3,bmp.H,fout);
		fwrite(bmppad,1,(4-(bmp.H*3)%4)%4,fout);
	}
	fclose(fout);
};
