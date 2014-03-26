#ifndef	_RAM_H_
#define	_RAM_H_

//#include	"cmd_define.h"
#include	"stdio.h"

class RAM
{
	private:
		int		bitwidth;
		int		size;
		int		step;
		char	*pbuff;
	public:
		RAM();
		~RAM();
		void init(int bits, int depth);
		void read(int addr, char* dst);
		void write(int addr, char* src);
};

class RAM2D
{
	private:
		int		pics;
		int		bitwidth;       /*2D RAM的位宽，表示一次读写操作的数据比特数*/
		int		depth;          /*2D RAM的深度，表示可以存储多少个N位的数据（如果RAM位宽为N）*/
		int		vstrid_r;
		int		vstrid_w;
		int		step;
		int		vaddress_r;
        int		vaddress_w;
		RAM		*list;
	public:
		RAM2D();
		~RAM2D();
		void init(int blocks, int bits, int size);
		void settrix_read(int addr, int strid);
		void settrix_write(int addr, int strid);
		void read(int x, int y, int dir, char* dst);
		void write(int x, int y, int dir, char* src);
		void write(int addr, char *src);
		void read(int addr, char *dst);
		void write2(int addr, char *src);
        void write2print(int addr, unsigned char *src, FILE *fp);
        void writeprint(int x, int y, int dir, unsigned char* src, FILE *fp);
		void read2(int addr, char *dst);
};
#endif
