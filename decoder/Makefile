################################################################################
# Automatically-generated file. Do not edit!
################################################################################
#-march=armv7-a -mfloat-abi=softfp -mfpu=neon-vfpv4 -mno-unaligned-access -fno-aggressive-loop-optimizations
include ../Makefile.pub 

TOP_DIR 	:= $(shell pwd | awk '{split($$0, f, "/rtspYolov5/decoder"); print f[1]}')
CLASS_DIR 	:= $(TOP_DIR)/samples/common/classes




INCLUDES= $(wildcard *.h)
CFLAGS :=   -Wall     
CFLAGS +=  -I./      -I./osal -I../connet -I"$(TOP_DIR)/include" -I"$(TOP_DIR)/include/libjpeg-8b"  -I"$(TARGET_ROOTFS)/usr/include/libdrm"
CXXFLAGS:=$(CFLAGS) -std=c++11
CCFLAGS:=$(CFLAGS)

LIBPATH = -L./lib -L/usr/lib/aarch64-linux-gnu/tegra/
LIBS := -lpthread -lv4l2 -lEGL -lGLESv2 -lX11  -lnvjpeg\
	-lnvbuf_utils   -ldrm  ./osal/osal.a ../connet/connet.a


CLASS_CPP:=	$(wildcard $(CLASS_DIR)/*.cpp)

CLASS_OBJS := $(CLASS_CPP:.cpp=.o) 

##### End of variables to change
SOURCES =  $(wildcard *.c) 
OBJFILES = $(SOURCES:%.c=obj/%.o) $(CLASS_OBJS)

.PHONY :clean install prepare all
TARGET = dec_bin
all : prepare $(TARGET) 


#change .cpp files  to .o files


#$@--目标文件，$^--所有的依赖文件，$<--第一个依赖文件。

obj/%.o:%.c 
	$(COMPILE_CXX)
	


dec_bin:$(OBJFILES) 
	$(LINK_CXX)  
	
prepare:
	@echo "preparing..."
	@if ! [ -d obj ]; then mkdir obj; fi;
	@echo "Top dir: " $(TOP_DIR)
	

clean:
	-rm -rf *.o obj/*.o $(TARGET) *~ obj/

install:
	cp $(TARGET) /home/work/nfs/nvr


##### Any additional, platform-specific rules come here:

