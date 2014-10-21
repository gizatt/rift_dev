/* #########################################################################
        Rift class -- rift initialization and such convenience functions

   Rev history:
     Gregory Izatt  20130721  Init revision
   ######################################################################### */    

#ifndef __XEN_RIFT_H
#define __XEN_RIFT_H

// Base system stuff
#include <stdio.h>
#include <iostream>
#include <string>
#include <sstream>

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "../include/GL/glew.h"
#include "../include/gl_helper.h"
#include <gl/gl.h>

#ifdef WIN32
  #define OVR_OS_WIN32
#endif
#ifdef __APPLE__
  // don't let this make you think this will ever work on a mac...
  #define OVR_OS_MAC
#endif
#include <OVR.h>
#include <../src/OVR_CAPI.h>
#include <../src/OVR_CAPI_GL.h>
#include "xen_utils.h"

#include <windows.h>

namespace xen_rift {
	typedef enum _detect_prompt_t {
		PR_CONTINUE,
		PR_ABORT,
		PR_RETRY
	} detect_prompt_t;

	const OVR::Vector3f UpVector(0.0f, 1.0f, 0.0f);
	const OVR::Vector3f ForwardVector(0.0f, 0.0f, -1.0f);
	const OVR::Vector3f RightVector(1.0f, 0.0f, 0.0f);

	class Rift {
		public:
			Rift(bool verbose = true );
			void initialize(int inputWidth = 1280, int inputHeight = 720);
			void update_rtarg(int width, int height);
			int set_resolution(int width, int height);
			// glut passthroughs
			void normal_key_handler(unsigned char key, int x, int y);
			void normal_key_up_handler(unsigned char key, int x, int y);
			void special_key_handler(int key, int x, int y);
			void special_key_up_handler(int key, int x, int y);
			void mouse(int button, int state, int x, int y);
			void motion(int x, int y);
			void onIdle( void );
			void render(OVR::Vector3f EyePos, OVR::Vector3f EyeRot, void (*draw_scene)(void));
			char which_eye(){ return _which_eye; }
		protected:
			// which eye is in use right now? only active
			// and valid within a draw_scene call.

			char _which_eye;
			bool _have_rift;
		    // *** Rendering Variables
		    int                 _win_width;
		    int 				_win_height;
            GLuint _fbo, _fb_tex, _fb_depth;
            int _fb_width, _fb_height;
            int _fb_tex_width, _fb_tex_height;

		    // Stereo view parameters.
			ovrHmd _hmd;
			ovrSizei _eyeres[2];
			ovrSizei _resolution;
			ovrEyeRenderDesc _eye_rdesc[2];
			ovrGLTexture _fb_ovr_tex[2];

		    // timekeeping
		    LARGE_INTEGER _lasttime;
		    LARGE_INTEGER _currtime;

			// verbose?
			bool _verbose;

		private:
	};
}

#endif //__XEN_PLAYER_H