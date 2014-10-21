NVCC = nvcc
CL = cl

## UPDATE TO YOUR RIFTDIR
## todo: pull riftdir from environmental variables, don't harcode here.
RIFTDIR=C:\Users\gizatt\Dropbox\Programming\xen_rift\OculusSDK\LibOVR
RIFTIDIR="$(RIFTDIR)\Include"
RIFTLDIR="$(RIFTDIR)\Lib\Win32\VS2010"

## UPDATE TO YOUR SIXENSE SDK DIR
## todo: pull from env vars...
#HYDRADIR=C:\Program Files (x86)\Steam\steamapps\common\Sixense SDK\SixenseSDK
HYDRADIR=E:\SteamLibrary\SteamApps\common\Sixense SDK\SixenseSDK
HYDRAIDIR="$(HYDRADIR)\include"
HYDRALDIR="$(HYDRADIR)\lib\win32\debug_dll"

## UPDATE TO YOUR CUDA DIR
CUDADIR=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v5.0
CUDAIDIR="$(CUDADIR)\include"
CUDALDIR="$(CUDADIR)\lib\Win32"

## UPDATE TO YOUR OPENCV DIR
OPENCVDIR=E:\opencv
OPENCVIDIR="$(OPENCVDIR)\build\include"
OPENCVSLDIR="$(OPENCVDIR)\build\x86\vc10\staticlib"
OPENCVLDIR="$(OPENCVDIR)\build\x86\vc10\lib"

## UPDATE TO PTHREAD DIR
PTHREADDIR=C:\Users\gizatt\Dropbox\Programming\pthreads-w32\Pre-built.2
PTHREADIDIR="$(PTHREADDIR)/include"
PTHREADLDIR="$(PTHREADDIR)/lib"

## UPDATE TO LIBFREENECT DIR
LIBFREENECTDIR=C:\Users\gizatt\Dropbox\Programming\libfreenect
LIBFREENECTIWDIR="$(LIBFREENECTDIR)\wrappers\cpp"
LIBFREENECTISDIR="$(LIBFREENECTDIR)\wrappers\c_sync"
LIBFREENECTIDIR="$(LIBFREENECTDIR)\include"
LIBFREENECTLDIR="$(LIBFREENECTDIR)\build\lib\Release"

ODIR=./obj
IDIR=./include
BDIR=./bin

CFLAGS=/I$(RIFTIDIR) /I$(HYDRAIDIR) /I$(IDIR) /I$(OPENCVDIR) /I$(OPENCVIDIR) /I$(PTHREADIDIR)\
	/I$(LIBFREENECTIDIR) /I$(LIBFREENECTIWDIR) /I$(LIBFREENECTISDIR) /Od /Zi
LFLAGS= /MD /link /LIBPATH:./lib /LIBPATH:$(RIFTLDIR) /LIBPATH:$(HYDRALDIR) /LIBPATH:$(RIFTLDIR) \
	/NODEFAULTLIB:LIBCMT\ SOIL.lib sixensed.lib sixense_utilsd.lib libovr.lib libovrd.lib opengl32.lib User32.lib Gdi32.lib \
    glew32d.lib cutil32d.lib shell32.lib

NVCC_CFLAGS=
NVCC_LFLAGS= -L./lib -Xlinker=/NODEFAULTLIB:MSVCRT -Xlinker=/NODEFAULTLIB:LIBCMT \
	-Xlinker=/NODEFAULTLIB:MSVCRTD -Xlinker=/NODEFAULTLIB:LIBCMTD \
	-m32 -lmsvcrt -lopengl32 -lUser32 -lGdi32 \
    -lglew32d -lcutil32d --optimize 9001 \
    -use_fast_math

all: $(BDIR)/simple_scene.exe $(BDIR)/webcam_feedthrough.exe

$(BDIR)/simple_scene.exe: $(ODIR)/player.obj $(ODIR)/ironman_hud.obj $(ODIR)/xen_utils.obj \
	$(ODIR)/rift.obj simple_scene/simple_scene.cpp simple_scene/simple_scene.h
	vcvars32
	$(CL) simple_scene/simple_scene.cpp $(CFLAGS) /Fe$@  \
		$(LFLAGS) $(ODIR)/xen_utils.obj $(ODIR)/player.obj $(ODIR)/rift.obj \
		$(ODIR)/ironman_hud.obj $(ODIR)/textbox_3d.obj

$(BDIR)/webcam_feedthrough.exe: $(ODIR)/rift.obj $(ODIR)/xen_utils.obj $(ODIR)/textbox_3d.obj \
		webcam_feedthrough/webcam_feedthrough.cpp webcam_feedthrough/webcam_feedthrough.h
	vcvars32
	$(CL) webcam_feedthrough/webcam_feedthrough.cpp $(CFLAGS) /Fe$@  \
		$(LFLAGS) /LIBPATH:$(OPENCVLDIR) /LIBPATH:$(OPENCVSLDIR) $(ODIR)/rift.obj \
		$(ODIR)/xen_utils.obj $(ODIR)/textbox_3d.obj opencv_core248.lib opencv_highgui248.lib \
		opencv_imgproc248.lib opencv_features2d248.lib \
		/LIBPATH:$(LIBFREENECTLDIR) freenect.lib /LIBPATH:$(PTHREADLDIR) pthreadVC2.lib \
		freenect_sync.lib

$(ODIR)/rift.obj: $(ODIR)/xen_utils.obj common/rift.cpp common/rift.h
	vcvars32
	$(CL) /c common/rift.cpp $(CFLAGS) /Fo$@ $(LFLAGS) /xen_utils.obj

$(ODIR)/player.obj: $(ODIR)/textbox_3d.obj common/player.cpp common/player.h
	vcvars32
	$(CL) /c common/player.cpp $(CFLAGS) /Fo$@ $(LFLAGS) /xen_utils.obj

$(ODIR)/hydra.obj: $(ODIR)/ironman_hud.obj $(ODIR)/xen_utils.obj common/hydra.cpp common/hydra.h
	vcvars32
	$(CL) /c common/hydra.cpp $(CFLAGS) /Fo$@ $(LFLAGS) /xen_utils.obj 

$(ODIR)/textbox_3d.obj: $(ODIR)/xen_utils.obj common/textbox_3d.cpp common/textbox_3d.h
	vcvars32
	$(CL) /c common/textbox_3d.cpp $(CFLAGS) /Fo$@ $(LFLAGS)

$(ODIR)/ironman_hud.obj: $(ODIR)/xen_utils.obj common/ironman_hud.cpp \
			common/ironman_hud.h
	vcvars32
	$(CL) /c common/ironman_hud.cpp $(CFLAGS) /Fo$@ $(LFLAGS)

$(ODIR)/kinect.obj: common/kinect.cpp common/kinect.h $(ODIR)/xen_utils.obj
	vcvars32
	$(CL) /c common/kinect.cpp $(CFLAGS) /Fo$@ $(LFLAGS) /LIBPATH:$(LIBFREENECTLDIR) \
		/LIBPATH:$(OPENCVLDIR) /LIBPATH:$(OPENCVSLDIR) opencv_core246.lib

$(ODIR)/xen_utils.obj: common/xen_utils.cpp common/xen_utils.h
	vcvars32
	$(CL) /c common/xen_utils.cpp $(CFLAGS) /Fo$@ $(LFLAGS) /LIBPATH:$(PTHREADLDIR) \
		pthreadVC2.lib

# Clean & Debug
.PHONY: clean
clean:
	rm -f $(ODIR)/* $(BDIR)/*.exe