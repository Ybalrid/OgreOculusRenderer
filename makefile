CC = g++

SYSARCH = i386
ifeq ($(shell uname -m),x86_64)
SYSARCH = x86_64
endif

CFLAGS =
#LDFLAGS =  -L../OculusSDK/LibOVR/Lib/Linux/Release/$(SYSARCH) -lovr  -lpthread -lX11 -lXinerama -ludev -lGL -lXrandr -lXxf86vm -lboost_system -lOgreMain -lGLEW -lovr  -lpthread -lX11 -lXinerama -ludev -lGL -lXrandr -lXxf86vm -lboost_system -lglfw -lrt -lm -lXrandr -lXrender -lXi -lGL -lm -lpthread -ldl -ldrm -lXdamage -lXfixes -lX11-xcb -lxcb-glx -lxcb-dri2 -lxcb-dri3 -lxcb-present -lxcb-sync -lxshmfence -lXxf86vm -lXext -lX11 -lpthread -lxcb -lXau -lXdmcp -lGLEW -lGLU -lm -lGL -lm -lpthread -ldl -ldrm -lXdamage -lXfixes -lX11-xcb -lxcb-glx -lxcb-dri2 -lxcb-dri3 -lxcb-present -lxcb-sync -lxshmfence -lXxf86vm -lXext -lX11 -lpthread -lxcbu
LDFLAGS =  -L../OculusSDK/LibOVR/Lib/Linux/Release/$(SYSARCH) -lovr  -lpthread -lX11 -lXinerama -ludev -lGL -lXrandr -lXxf86vm -lboost_system -lOgreMain -lGLEW
IFLAGS = -I /usr/include/OGRE -I  ../OculusSDK/LibOVR/Include -I ../OculusSDK/LibOVR/Src -I /usr/include/GL -I ./ogre-procedural/include


all: OculusInterface.o OgreOculusRender.o OculusInterface.hpp test.cpp 
	$(CC) $(CFLAGS) OculusInterface.o OgreOculusRender.o test.cpp -o test $(IFLAGS) $(LDFLAGS)

OculusInterface.o: OculusInterface.cpp OculusInterface.hpp
	$(CC) $(CFLAGS) -c OculusInterface.cpp -o OculusInterface.o $(IFLAGS) $(LDFLAGS)

OgreOculusRender.o: OgreOculusRender.cpp OgreOculusRender.hpp
	$(CC) $(CFLAGS) -c OgreOculusRender.cpp -o OgreOculusRender.o $(IFLAGS) $(LDFLAGS)
.PHONY: clean

clean:
	rm *.o
