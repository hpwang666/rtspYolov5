




TOP_DIR 	:= $(shell pwd | awk '{split($$0, f, "/rtspYolov5/fileDecoder"); print f[1]}')
CLASS_DIR 	:= $(TOP_DIR)/samples/common/classes

INCLUDES = -I ./inc/ -I"$(TOP_DIR)/include" -I"$(TOP_DIR)/include/libjpeg-8b"  -I"$(TARGET_ROOTFS)/usr/include/libdrm"
		
##### Change the following for your environment: 
CROSS_COMPILE =
C_COMPILER =		$(CROSS_COMPILE)g++
COMPILE_OPTS =		-std=c++11 $(INCLUDES)    #-DSOCKLEN_T=socklen_t -DNO_SSTREAM=1 -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64
C_FLAGS =		$(COMPILE_OPTS)  
#-D可以把编译宏传进去

# -ffunction-sections、-fdata-sections，在编译生成的.o目标文件中，会将每个函数或数据段，放在各种单独独立的section中；
# 链接时候使用这个参数 -Wl,–gc-sections 就不会把不用的函数链接进执行文件
CPLUSPLUS_COMPILER =	$(CROSS_COMPILE)g++ 
CPLUSPLUS_FLAGS =	$(COMPILE_OPTS) -Wall #-DBSD=1 -g -fPIC

LINK  =	 $(CROSS_COMPILE)g++  

##### End of variables to change
ALL = prepare $(TARGET) 
TARGET = decoder_bin
all:  $(ALL)

CLASS_CPP:=	$(wildcard $(CLASS_DIR)/*.cpp)

CLASS_OBJS := $(CLASS_CPP:.cpp=.o)

SOURCES =  $(wildcard *.cpp) 

#change .cpp files  to .o files
OBJFILES = $(SOURCES:%.cpp=obj/%.o) $(CLASS_OBJS)



#$@--目标文件，$^--所有的依赖文件，$<--第一个依赖文件。
obj/%.o:%.cpp   
	@echo -e CC $<
	@$(CPLUSPLUS_COMPILER)  $(CPLUSPLUS_FLAGS) -c -o $@ $< 


LIBPATH = -L./lib -L/usr/lib/aarch64-linux-gnu/tegra/
#注意lib的顺序，最底层的放在最后面，以便于被前面的库引用,a需要b里面的文件，b需要c里面的文件
LIBS = -lpthread -lv4l2 -lEGL -lGLESv2 -lX11  -lnvjpeg\
	-lnvbuf_utils   -ldrm 
	
RUNLIB = -Wl,-rpath=/home/whp/jetson_multimedia_api/rtsp/decoder:./lib
ALLLIBS = $(RUNLIB) $(LIBPATH) $(LIBS) 

#不会删除中间文件
.SECONDARY:
	
$(TARGET):	$(OBJFILES) 
	@echo "linking $(TARGET) from ..."
	$(LINK) -o $@  $^  $(ALLLIBS) 
	

prepare:
	@echo "preparing..."
	@echo "Top dir: " $(TOP_DIR)
	@if ! [ -d obj ]; then mkdir obj; fi;

clean:
	-rm -rf *.o obj/*.o  $(TARGET) *~ 
install:
	install $(TARGET) $(INSTALL_DIR)
	
.PHONY :clean install prepare all

##### Any additional, platform-specific rules come here:
