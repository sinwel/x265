
#include	"ram.h"
#include	"stdlib.h"
#include	"string.h"

RAM::RAM()
{
	bitwidth = 0;
	size     = 0;
	step     = 0;
	pbuff    = NULL;	
}

RAM::~RAM()
{
	if(pbuff)
		free(pbuff);
	bitwidth = 0;
	size     = 0;
	step     = 0;
	pbuff    = NULL;
}
void RAM::init(int bits, int depth)
{
	bitwidth = bits;
	step = (bitwidth+7)>>3;
	size = depth;
	pbuff = (char*)malloc(depth*step);
}

void RAM::read(int addr, char* dst)
{
	addr = addr&(size-1);
	memcpy(dst, pbuff+addr*step,step);
}

void RAM::write(int addr, char* src)
{
	addr = addr&(size-1);
	memcpy(pbuff+addr*step, src, step);
}

RAM2D::RAM2D()
{
	pics              = 0 ;
	bitwidth          = 0 ;
	depth             = 0 ;
	vstrid_r          = 0 ;
	vstrid_w          = 0 ;
	vaddress_r        = 0 ;
    vaddress_w        = 0 ;
	step              = 0 ;
	list              = NULL ;

}

RAM2D::~RAM2D()
{
	if (list)
	{
    int i;
    
    for(i=0; i<pics; i++)
      list[i].~RAM();
    free(list);
	}
	pics              = 0 ;
	bitwidth          = 0 ;
	depth             = 0 ;
	vstrid_r          = 0 ;
	vstrid_w          = 0 ;
	vaddress_r        = 0 ;
  vaddress_w        = 0 ;
	step              = 0 ;
	list              = NULL ;
}

void RAM2D::init(int blocks, int bits, int size)
{
	int		i;
	list = (RAM*)malloc(blocks*sizeof(RAM));
	for (i=0; i<blocks; i++)
		list[i].init(bits,size);
	pics = blocks;
	bitwidth = bits;
	depth = size;
	step = (bits+7)>>3;
}

void RAM2D::settrix_read(int addr, int strid)
{
	vaddress_r = addr;
	vstrid_r   = strid;
}

void RAM2D::settrix_write(int addr, int strid)
{
	vaddress_w = addr;
	vstrid_w   = strid;
}


void RAM2D::read(int x, int y, int dir, char* dst)
{
	int			i,addr,pidx;
	
	if (bitwidth&7)
	{
	}
	else
	{
		if (dir)
		{
			for (i=0; i<pics; i++)
			{
				addr = vaddress_r/pics + ((y+i)*vstrid_r+x)/pics;
				pidx = (x+y+i)%pics;
				list[pidx].read(addr,dst);
				dst += step;
			}
		}
		else
		{
			addr = vaddress_r/pics + (y*vstrid_r + x)/pics;
			for (i=0; i<pics; i++)
			{
			    pidx = (x+y+i)%pics;
				list[pidx].read(addr,dst);
				dst += step;
			}
		}
	}	
}

void RAM2D::write(int x, int y, int dir, char* src)
{
	int			i,addr,pidx;
	
	if (bitwidth&7)
	{
		
	}
	else
	{
		if (dir)
		{
			for (i=0; i<pics; i++)
			{
				addr = vaddress_w/pics + ((y+i)*vstrid_w+x)/pics;
				pidx = (x+y+i)%pics;
				list[pidx].write(addr,src);
				src += step;
			}
		}
		else
		{
			addr = vaddress_w/pics + (y*vstrid_w + x)/pics;
			for (i=0; i<pics; i++)
			{
			    pidx = (x+y+i)%pics;
				list[pidx].write(addr,src);
				src += step;
			}
		}
	}
}

void RAM2D::writeprint(int x, int y, int dir, unsigned char* src, FILE *fp)
{
	int			i,addr,pidx;
	
	if (!fp)
		return;
	if (bitwidth&7)
	{
		
	}
	else
	{
		if (dir)
		{
			for (i=0; i<pics; i++)
			{
				addr = vaddress_w/pics + ((y+i)*vstrid_w+x)/pics;
				pidx = (x+y+i)%pics;
                fprintf(fp,"addr:%04x data:%02x%02x%02x%02x%02x%02x%02x%02x\n",(addr&(depth-1))*pics+pidx,src[7],src[6],src[5],src[4],src[3],src[2],src[1],src[0]);
				src += step;
			}
		}
		else
		{
			addr = vaddress_w/pics + (y*vstrid_w + x)/pics;
			for (i=0; i<pics; i++)
			{
			    pidx = (x+y+i)%pics;
                fprintf(fp,"addr:%04x data:%02x%02x%02x%02x%02x%02x%02x%02x\n",(addr&(depth-1))*pics+pidx,src[7],src[6],src[5],src[4],src[3],src[2],src[1],src[0]);
				src += step;
			}
		}
	}
}

void RAM2D::write(int addr, char *src)
{
	int	ad,idx;

	ad  = vaddress_w+addr;
	idx = ad%pics;
	ad  = ad/pics;
	list[idx].write(ad, src);
}

void RAM2D::read(int addr, char *dst)
{
	int	ad,idx;

	ad  = vaddress_r+addr;
	idx = ad%pics;
	ad  = ad/pics;
	list[idx].read(ad, dst);
}

void RAM2D::write2(int addr, char *src)
{
	int	ad,idx;

	ad  = vaddress_w+addr;
	ad  = ad/pics;
	for (idx=0; idx<pics; idx++)
	{
		list[idx].write(ad, src+idx*step);
	}
}

void RAM2D::write2print(int addr, unsigned char *src, FILE *fp)
{
	int	ad,idx;

    if(!fp)
        return;
	ad  = vaddress_w+addr;
	ad  = ad/pics;
	for (idx=0; idx<pics; idx++)
	{
        fprintf(fp,"addr:%04x data:%02x%02x%02x%02x%02x%02x%02x%02x\n",(ad&(depth-1))*pics+idx,src[idx*step+7],src[idx*step+6],src[idx*step+5],src[idx*step+4],src[idx*step+3],src[idx*step+2],src[idx*step+1],src[idx*step+0]);
	}
}


void RAM2D::read2(int addr, char *dst)
{
	int	ad,idx;

	ad  = vaddress_r+addr;
	ad  = ad/pics;
	for (idx=0; idx<pics; idx++)
	{
		list[idx].read(ad, dst+idx*step);
	}
}

