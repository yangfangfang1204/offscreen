#include "Render.h"

CRender *pRenderHandle[4];
char g_pHwd[4][128];
int totalNum = 0;
int yuvsize = 704*576*3/2;
unsigned char *yuvData = NULL;
FILE *pFile = NULL;

#ifdef __cplusplus
extern "C"
{
#endif

EMSCRIPTEN_KEEPALIVE
int start(void* hwnd,int num)
{
	totalNum = num;
	if(yuvData == NULL)
	{
		yuvData = new unsigned char[yuvsize];
		memset(yuvData,0,yuvsize);
	}

	if (NULL == pFile)
	{
	
		pFile = fopen("./704-576.data", "ab+");
		if (NULL == pFile)
		{
			printf("open YUV file failed");
			return NULL;
		}
		fread(yuvData, 1, yuvsize, pFile);
	}
	for(int i=0;i<num;i++)
	{
		sprintf(g_pHwd[i], "%s%d", "#player",i);
		
		printf("%s\n",g_pHwd[i]);
		pRenderHandle[i] = new CRender();
		pRenderHandle[i]->startPlay(g_pHwd[i],yuvData);
	}
	return 1;
}
EMSCRIPTEN_KEEPALIVE
int stop()
{
	for(int i=0;i<totalNum;i++)
	{
		pRenderHandle[i]->stopPlay();
		delete pRenderHandle[i];
		pRenderHandle[i] = NULL;
		printf("delete render\n");
	}
	
	return 1;
}

#ifdef __cplusplus
}
#endif
