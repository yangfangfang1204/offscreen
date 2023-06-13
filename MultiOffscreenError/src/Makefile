# CROSS_COMPILE = arm-hisiv300-linux-
CC = emcc
AR = emar
# LD=$(CROSS_COMPILE)ld
THREAD = yes

IFLAG = -I./

CFLAGS 	= $(IFLAG) -Wall-O3 
#-fsanitize=address

LDFLAGS = -s "EXPORTED_RUNTIME_METHODS=['ccall', 'getValue','writeArrayToMemory','print','UTF8ToString','setValue','addFunction']" -s EXPORTED_FUNCTIONS="['_malloc','_free','_start','_stop']" -s USE_FREETYPE=1 -s OFFSCREENCANVAS_SUPPORT=1 -s ENVIRONMENT=web,worker --preload-file 704-576.data

LDFLAGS_THREAD = -s PTHREAD_POOL_SIZE_STRICT=0 -s USE_PTHREADS=1

SRCS =$(wildcard ./*.cpp)\

OBJS = $(patsubst %.cpp,%.o,$(SRCS))

TARGET = Render.js
OBJ_TARGET = STRender.a

	
all: $(OBJS) $(OBJ_TARGET) $(TARGET)

$(OBJ_TARGET): $(OBJS)
	$(AR) rcs $(OBJ_TARGET) $(notdir $(OBJS))

$(TARGET):
	$(CC) -O3 --bind $(OBJ_TARGET) -o $(TARGET) $(LDFLAGS) $(LDFLAGS_THREAD)

#--memoryprofiler -O3 --profiling -fsanitize=address
	
.cpp.o:
	$(CC) $(CFLAGS) $(CFLAGS_THREAD) -c $<

clean:
	rm -f *.o *.a *.js $(OBJS) $(OBJ_TARGET) $(TARGET)
	rm -rf $(DIR)
