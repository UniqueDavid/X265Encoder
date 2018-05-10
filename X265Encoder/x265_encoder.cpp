#include<stdio.h>
#include<stdlib.h>

#include "stdint.h"

#if defined (__cplusplus)
extern "C"
{
#include"x265.h"
}
#else
#include"x265.h"
#endif

int main(int argc,char** argv)
{
	int ret;
	int y_size;
	int i,j;
	int frame_num =0;
	int width=1024,height=600;
	int csp=X265_CSP_I420;
	char *buf;

	FILE *fp_src = fopen("../1.yuv","rb");
	FILE *fp_des = fopen("../output.h265","wb");

	//开始解码
	uint32_t iNal = 0;
	x265_nal *pNals = NULL;
	x265_encoder *phandle = NULL;
	x265_picture *pPic_in = x265_picture_alloc();
	x265_picture *pPic_out = x265_picture_alloc();
	x265_param *pParam = x265_param_alloc();

	if(fp_src==NULL||fp_des==NULL)
	{
		printf("Error open files!!\n");
		return -1;
	}
	//开始设置参数
	x265_param_default(pParam);
	pParam->bRepeatHeaders = 1;
	pParam->internalCsp = csp;
	pParam->sourceWidth = width;
	pParam->sourceHeight = height;
	pParam->fpsNum = 25;
	pParam->fpsDenom = 1;


	phandle = x265_encoder_open(pParam);

	if(phandle==NULL)
	{
		printf("Error open encoder!!\n");
		return -1;
	}

	y_size = pParam->sourceWidth*pParam->sourceHeight;
	x265_picture_init(pParam,pPic_in);
	  switch (csp)
	{
	case X265_CSP_I420:
		buf = (char*)malloc(y_size*3/2);
		pPic_in->planes[0] = buf;
		pPic_in->planes[1] = buf+y_size;
		pPic_in->planes[2] = buf+y_size*5/4;
		pPic_in->stride[0]=width;
		pPic_in->stride[1]=width/2;
		pPic_in->stride[2]=width/2;
		break;
	default:
		printf("csp colorspace not support!!\n");
		return -1;
	}
	//计算下文件的大小
	fseek(fp_src,0,SEEK_END);

	frame_num = ftell(fp_src)/(y_size*3/2);

	fseek(fp_src,0,SEEK_SET);

	//Loop to encoder
	for (i=0;i<frame_num;i++)
	{
		switch (csp)
		{
		case X265_CSP_I420:
			fread(pPic_in->planes[0],1,y_size,fp_src);//Y
			fread(pPic_in->planes[1],1,y_size/4,fp_src);//U
			fread(pPic_in->planes[2],1,y_size/4,fp_src);//V
			break;
		default:
			{
				printf("colorspace not support!!\n");
				return -1;
			}
		}


		ret = x265_encoder_encode(phandle,&pNals,&iNal, pPic_in, NULL);

		if(ret<0)
		{
			printf("Error!!\n");
			return -1;
		}

		printf("Succeed encode frame:%5d\n",i);
		for(j=0;j<iNal;++j)
		{
			fwrite(pNals[j].payload, 1, pNals[j].sizeBytes, fp_des);
		}
	}
	i=0;
	//flush encoder
	while(1)
	{
		ret = x265_encoder_encode(phandle,&pNals,&iNal,NULL,NULL);
		if (ret == 0)
			break;
		printf("Flush 1 frame.\n");
		for(j=0;j<iNal;++j)
		{
			fwrite(pNals[j].payload, 1, pNals[j].sizeBytes, fp_des);
		}

	}

	//clean
	x265_picture_free(pPic_in);
	x265_encoder_close(phandle);
	x265_param_free(pParam);
	free(buf);

	fclose(fp_src);
	fclose(fp_des);
		
	return 0;
}