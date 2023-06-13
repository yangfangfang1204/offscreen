
#include "Render.h"

int bRender[32];

CRender *pRenderHandle[4];
char g_pHwd[4][128];
int totalNum = 0;

#ifdef __cplusplus
extern "C"
{
#endif

EMSCRIPTEN_KEEPALIVE
int start(void* hwnd,int num)
{
	totalNum = num;
	for(int i=0;i<num;i++)
	{
		sprintf(g_pHwd[i], "%s%d", "#player",i);
		
		printf("%s\n",g_pHwd[i]);
		pRenderHandle[i] = new CRender();
		pRenderHandle[i]->startPlay(g_pHwd[i]);
	}
	return 1;
}
EMSCRIPTEN_KEEPALIVE
int stop()
{
	for(int i=0;i<totalNum;i++)
	{
		pRenderHandle[i]->stopPlay();
	}
	
	return 1;
}

#ifdef __cplusplus
}
#endif
