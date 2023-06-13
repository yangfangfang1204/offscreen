#include "Render.h"

#define SR_OK 0;
#define SR_FAIL 1;

#define Y 0
#define U 1
#define V 2

/*顶点着色器代码*/
static const char g_pGLVS[] = ///< 普通纹理渲染顶点着色器
{
	"precision mediump float;"
	"attribute vec4 position; "
	"attribute vec4 texCoord; "
	"varying vec4 pp; "

	"void main() "
	"{ "
	"    gl_Position = position; "
	"    pp = texCoord; "
	"} "};

/*像素着色器代码*/
static const char *g_pGLFS = ///< YV12片段着色器
{
	"precision mediump float;"
	"varying vec4 pp; "
	"uniform sampler2D Ytexture; "
	"uniform sampler2D Utexture; "
	"uniform sampler2D Vtexture; "
	"void main() "
	"{ "
	"    float r,g,b,y,u,v; "

	"    y=texture2D(Ytexture, pp.st).r; "
	"    u=texture2D(Utexture, pp.st).r; "
	"    v=texture2D(Vtexture, pp.st).r; "

	"    y=1.1643*(y-0.0625); "
	"    u=u-0.5; "
	"    v=v-0.5; "

	"    r=y+1.5958*v; "
	"    g=y-0.39173*u-0.81290*v; "
	"    b=y+2.017*u; "
	"    gl_FragColor=vec4(r,g,b,1.0); "
	"} "
};

const GLfloat g_Vertices[] = {
    -1.0f,
    1.0f,
    -1.0f,
    -1.0f,
    1.0f,
    1.0f,
    1.0f,
    1.0f,
    -1.0f,
    -1.0f,
    1.0f,
    -1.0f,
};

const GLfloat g_TexCoord[] = {
    0.0f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    0.0f,
    1.0f,
    0.0f,
    0.0f,
    1.0f,
    1.0f,
    1.0f,
};
const int totalCanvasNum =1;

CRender::CRender()
{
	pYUVData = NULL;
	g_yuvLen = 704*576*3/2;
	g_nFrameWidth = 704;
	g_nFrameHeight = 576;
	g_size =g_yuvLen;
	pFile = NULL;
	g_stopFlag = 0;
	g_pMutex=PTHREAD_MUTEX_INITIALIZER;
    cond = PTHREAD_COND_INITIALIZER;
}
CRender::~CRender()
{

}
void CRender::initBuffers()
{
	GLuint vertexPosBuffer;
	glGenBuffers(1, &vertexPosBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexPosBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_Vertices), g_Vertices, GL_STATIC_DRAW);

	GLint nPosLoc = glGetAttribLocation(g_ShaderProgram, "position");
	glEnableVertexAttribArray(nPosLoc);
	glVertexAttribPointer(nPosLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
	g_vertexPosBuffer= vertexPosBuffer;

	GLuint texturePosBuffer;
	glGenBuffers(1, &texturePosBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, texturePosBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_TexCoord), g_TexCoord, GL_STATIC_DRAW);

	GLint nTexcoordLoc = glGetAttribLocation(g_ShaderProgram, "texCoord");
	glEnableVertexAttribArray(nTexcoordLoc);
	glVertexAttribPointer(nTexcoordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
	g_texturePosBuffer= texturePosBuffer;
}
int CRender::makeCurrent(){
	emscripten_webgl_make_context_current(glContext);
	return 1;
}

void CRender::initTexture()
{
	glUseProgram(g_ShaderProgram);
	GLuint nTexture2D[3]; ///<< YUV三层纹理数组
	glGenTextures(3, nTexture2D);
	for (int i = 0; i < 3; ++i)
	{
		glBindTexture(GL_TEXTURE_2D, nTexture2D[i]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, NULL);
	}

	GLuint nTextureUniform[3];
	nTextureUniform[Y] = glGetUniformLocation(g_ShaderProgram, "Ytexture");
	nTextureUniform[U] = glGetUniformLocation(g_ShaderProgram, "Utexture");
	nTextureUniform[V] = glGetUniformLocation(g_ShaderProgram, "Vtexture");
	glUniform1i(nTextureUniform[Y], 0);
	glUniform1i(nTextureUniform[U], 1);
	glUniform1i(nTextureUniform[V], 2);
	g_Texture2D[0] = nTexture2D[0];
	g_Texture2D[1] = nTexture2D[1];
	g_Texture2D[2] = nTexture2D[2];
	glUseProgram(NULL);
	emscripten_webgl_make_context_current(NULL);
}
GLuint CRender::initShaderProgram()
{
	///< 顶点着色器相关操作
	nVertexShader = glCreateShader(GL_VERTEX_SHADER);
	const GLchar *pVS = g_pGLVS;
	GLint nVSLen = static_cast<GLint>(strlen(g_pGLVS));
	glShaderSource(nVertexShader, 1, (const GLchar **)&pVS, &nVSLen);
	printf("initShaderProgram 2 nVSLen:%d\n", nVSLen);
	GLint nCompileRet;
	glCompileShader(nVertexShader);
	glGetShaderiv(nVertexShader, GL_COMPILE_STATUS, &nCompileRet);
	if (0 == nCompileRet)
	{
		return SR_FAIL;
	}
	///< 片段着色器相关操作
	nFragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	const GLchar *pFS = g_pGLFS;
	GLint nFSLen = static_cast<GLint>(strlen(g_pGLFS));
	glShaderSource(nFragmentShader, 1, (const GLchar **)&pFS, &nFSLen);
	glCompileShader(nFragmentShader);
	glGetShaderiv(nFragmentShader, GL_COMPILE_STATUS, &nCompileRet);
	if (0 == nCompileRet)
	{
		return SR_FAIL;
	}

	///< program相关
	GLuint nShaderProgram = glCreateProgram();
	glAttachShader(nShaderProgram, nVertexShader);
	glAttachShader(nShaderProgram, nFragmentShader);
	glLinkProgram(nShaderProgram);
	GLint nLinkRet;
	glGetProgramiv(nShaderProgram, GL_LINK_STATUS, &nLinkRet);
	if (0 == nLinkRet)
	{
		return SR_FAIL;
	}
	glUseProgram(nShaderProgram);

	return nShaderProgram;
}

void CRender::initContextGL()
{
	// printf("create size success\n");
	EmscriptenWebGLContextAttributes attr;
	emscripten_webgl_init_context_attributes(&attr);
	attr.explicitSwapControl = EM_TRUE;
	attr.alpha = 0;
	// attr.premultipliedAlpha = 0; // 预乘阿尔法通道
	// #if MAX_WEBGL_VERSION >= 2
	attr.majorVersion = 2;
	// #endif
	// printf("create context fallback\n");
	// emscripten_webgl_make_context_current(NULL);
	attr.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_FALLBACK; // EMSCRIPTEN_WEBGL_CONTEXT_PROXY_FALLBACK
	attr.renderViaOffscreenBackBuffer = EM_TRUE;

	glContext = emscripten_webgl_create_context(g_pHwd, &attr);
	printf("initContextGL 2 ,g_pHwd:%s，glContext：%d\n", g_pHwd, glContext);
	assert(glContext);
	EMSCRIPTEN_RESULT res = emscripten_webgl_make_context_current(glContext);
	assert(res == EMSCRIPTEN_RESULT_SUCCESS);
	assert(emscripten_webgl_get_current_context() == glContext);

	EM_BOOL supported = emscripten_webgl_enable_extension(glContext, "WEBGL_lose_context");
	printf("WEBGL_lose_context supported:%d\n", supported);

}

int CRender::RenderFrame(unsigned char *pFrameData, int nFrameWidth, int nFrameHeight, int size)
{
	if (pFrameData == NULL || nFrameWidth <= 0 || nFrameHeight <= 0)
	{
		return 0;
	}
	glClearColor(0.8f, 0.8f, 1.0f, 1.0);
	glClear(GL_COLOR_BUFFER_BIT);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, g_Texture2D[Y]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, nFrameWidth, nFrameHeight, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pFrameData);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, g_Texture2D[V]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, nFrameWidth / 2, nFrameHeight / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pFrameData + nFrameWidth * nFrameHeight * 5 / 4);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, g_Texture2D[U]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, nFrameWidth / 2, nFrameHeight / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, pFrameData + nFrameWidth * nFrameHeight);

	glUseProgram(g_ShaderProgram);
	// 将所有顶点数据上传至顶点着色器的顶点缓存
#ifdef DRAW_FROM_CLIENT_MEMORY
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 20, pos_and_color);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 20, (void *)(pos_and_color + 2));

	GLint nPosLoc = glGetAttribLocation(g_ShaderProgram, "position");
	glVertexAttribPointer(nPosLoc, 2, GL_FLOAT, GL_FALSE, 0, g_Vertices);
	GLint nTexcoordLoc = glGetAttribLocation(g_ShaderProgram, "texCoord");
	glVertexAttribPointer(nTexcoordLoc, 2, GL_FLOAT, GL_FALSE, 0, g_TexCoord);
#else
	glBindBuffer(GL_ARRAY_BUFFER, g_vertexPosBuffer);
	GLint nPosLoc = glGetAttribLocation(g_ShaderProgram, "position");
	glEnableVertexAttribArray(nPosLoc);
	glVertexAttribPointer(nPosLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);

	glBindBuffer(GL_ARRAY_BUFFER, g_texturePosBuffer);
	GLint nTexcoordLoc = glGetAttribLocation(g_ShaderProgram, "texCoord");
	glEnableVertexAttribArray(nTexcoordLoc);
	glVertexAttribPointer(nTexcoordLoc, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, NULL);
#endif
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glDisableVertexAttribArray(nPosLoc);
	glDisableVertexAttribArray(nTexcoordLoc);

#define EXPLICIT_SWAP
#ifdef EXPLICIT_SWAP
	emscripten_webgl_commit_frame();
#endif

#ifdef REPORT_RESULT
	printf("REPORT_RESULT\n");
	REPORT_RESULT(0);
#endif
	glUseProgram(NULL);
	return 1;
}
void CRender::makeCurrent(EMSCRIPTEN_WEBGL_CONTEXT_HANDLE context)
{
	emscripten_webgl_make_context_current(context);
	return ;
}

void CRender::requestRender(void *pUser)
{
	CRender* pThis = (CRender*)(pUser);
	usleep(40 * 1000);
	if (pThis->g_stopFlag)
	{
		pthread_mutex_lock(&pThis->g_pMutex);
		pThis->DestroyContext();
		pthread_cond_signal(&pThis->cond);
	    pthread_mutex_unlock(&pThis->g_pMutex);
		emscripten_cancel_main_loop();
		printf("thread exit\n");
		return ;
	}
	if (0 == fread(pThis->pYUVData, 1, pThis->g_size, pThis->pFile))
	{
		printf("read YUV file failed\n");
		return ;
	}


	if (pThis->glContext != NULL)
	{
		pThis->makeCurrent(pThis->glContext);
		pThis->RenderFrame(pThis->pYUVData, pThis->g_nFrameWidth, pThis->g_nFrameHeight, pThis->g_size);
		pThis->makeCurrent(NULL);
	}

	return ;
}
void CRender::DestroyContext()
{
	if (glContext != NULL)
	{
		EMSCRIPTEN_RESULT res = emscripten_webgl_make_context_current(glContext);
		if (res != EMSCRIPTEN_RESULT_SUCCESS)
		{
			return ;
		}
		glDetachShader(g_ShaderProgram, nVertexShader);
		glDetachShader(g_ShaderProgram, nFragmentShader);
		glDeleteProgram(g_ShaderProgram);
		glDeleteShader(nVertexShader);
		glDeleteShader(nFragmentShader);

		res = emscripten_webgl_destroy_context(glContext);

		if (res != EMSCRIPTEN_RESULT_SUCCESS)
		{
			return ;
		}
		glContext = NULL;
		if (res == 0)
		{
			printf("DeInit() destroy context\n");
		}
	}
	return ;
	
}

void* CRender::initRender(void *hWindow)
{
	// 初始化Context
	initContextGL();
	// 初始化着色器程序
	g_ShaderProgram = initShaderProgram();
	// 初始化buffer
	initBuffers();
	// 初始化纹理
	initTexture();
	

	if (pYUVData == NULL || g_yuvLen < g_size)
	{
		if (pYUVData != NULL)
		{
			delete[] pYUVData;
			pYUVData = NULL;
		}
		pYUVData = new unsigned char[g_size];
		if (pYUVData == NULL)
			return NULL;
		g_yuvLen = g_size;
	}
	if (NULL != pYUVData)
	{
		pFile = fopen("./704-576.data", "rb");
		if (NULL == pFile)
		{
			printf("open YUV file failed");
			return NULL;
		}
	}
	return hWindow;
}

void* CRender::displayThread(void *pUser)
{
	CRender* pThis = (CRender*)(pUser);
	pThis->initRender(pThis);
	printf("init success\n");
	// 读数据渲染
	emscripten_set_main_loop_arg(requestRender,pThis,0,0);
	return pUser;
}

int CRender::InitMain(void *hwnd)
{
	if (!emscripten_supports_offscreencanvas())
	{
		printf("Current browser does not support OffscreenCanvas. Skipping the rest of the tests.\n");
#ifdef REPORT_RESULT
		REPORT_RESULT(1);
#endif
		return -1;
	}
	strcpy(g_pHwd, (char *)hwnd);
	printf("InitMain hwnd:%s,g_pHwd:%s\n", (char *)hwnd, g_pHwd);

	pthread_attr_t attr;
	pthread_attr_init(&attr);
	emscripten_pthread_attr_settransferredcanvases(&attr, g_pHwd);

	pthread_create(&thread, &attr, displayThread, this);
	pthread_detach(thread);

	return 0;
}
int CRender::startPlay(void* hwnd)
{
	InitMain(hwnd);
	return 1;
	
}
int CRender::stopPlay()
{
	pthread_mutex_lock(&g_pMutex);
	g_stopFlag = 1;
	pthread_cond_wait(&cond,&g_pMutex);
	pthread_mutex_unlock(&g_pMutex);
	
	//pthread_kill(thread, SIGUSR2);
    //pthread_join(thread, NULL);
	return 1;
	
}