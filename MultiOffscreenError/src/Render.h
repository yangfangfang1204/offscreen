#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <emscripten/html5.h>
#include <emscripten/stack.h>
#include <emscripten/threading.h>
#include <unistd.h>

#include <GLES2/gl2.h>
// #include <webgl/webgl1_ext.h>
#include <emscripten.h>
// #include <emscripten/html5_webgl.h>

#include <iostream>
#include <string>
#include <string.h>
#include <vector>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <iostream> // std::cout
#include <sstream>  // std::stringstream



class CRender{
	public:
		CRender();
		~CRender();
		void initBuffers();
		void initTexture();
		GLuint initShaderProgram();
		void initContextGL();
		int RenderFrame(unsigned char *pFrameData, int nFrameWidth, int nFrameHeight, int size);	
		int makeCurrent();
		void *initRender(void *hWindow);
		int startPlay(void* hwnd);
		int stopPlay();
		
		static void request(void *);
		static void* displayThread(void *p);
		
		static void requestRender(void *pUser);
		int InitMain(void *hwnd);
		
		void makeCurrent(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context);
        void DestroyContext();
	    		
	
	public:
		
		GLuint g_ShaderProgram;
		GLuint g_Texture2D[3];
		GLuint g_vertexPosBuffer;
		GLuint g_texturePosBuffer;
		unsigned char *pYUVData;
		int g_yuvLen;
		int g_nFrameWidth;
		int g_nFrameHeight;
		int g_size;

		char g_pHwd[128];
		bool bNeedStop;
		bool bNeedStart;

		char g_pHwdT[128];

		EMSCRIPTEN_WEBGL_CONTEXT_HANDLE glContext;
		GLuint nVertexShader;
		GLuint nFragmentShader;

		FILE *pFile;
        int g_stopFlag;
		
		pthread_mutex_t g_pMutex;
		pthread_cond_t cond;
		pthread_t thread;

};

