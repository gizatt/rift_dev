/* #########################################################################
        Rift class -- rift initialization and such convenience functions
            (provides convenient wrappers to setup Rift in such a way
            that's reasonably robust to lack of device, etc)
        
        Much reference to Rift SDK samples for ways of robustly handling
            failure cases.

        Wraps the Rift SDK to manage its initialization (robust to lack
        of device) and support reasonably flexible stereo rendering.
        Keeping this updated with screen dimensions (mutators for
        those are a %TODO) will allow this to be called as a core render
        helper, passing its render() method a pointer to a render() function
        that does the actual drawing (after view, perspective, modelview
        are set).



   Rev history:
     Gregory Izatt  20130721  Init revision
     Gregory Izatt  201308**  Various revisions, fleshing this out to base
                                functionality and squashing bugs.
     Gregory Izatt  201410**  Updating to SDK 0.4xx. Huge revisions, largely
        referencing http://nuclear.mutantstargoat.com/hg/oculus2/file/
        5b04743fd3d0/src/main.c
   ######################################################################### */    

#include "rift.h"
using namespace std;
using namespace xen_rift;
using namespace OVR;

/* forward declaration to avoid including non-public headers of libovr */
//OVR_EXPORT void ovrhmd_EnableHSWDisplaySDKRender(ovrHmd hmd, ovrBool enable);
#include "../Src/CAPI/CAPI_HSWDisplay.h"

Rift::Rift(bool verbose) {

    ovr_Initialize();
    if (!(_hmd = ovrHmd_Create(0))) {
      fprintf(stderr, "failed to open Oculus HMD, falling back to virtual debug HMD\n");
      if(!(_hmd = ovrHmd_CreateDebug(ovrHmd_DK2))) {
        fprintf(stderr, "failed to create virtual debug HMD\n");
          exit(1);
      }
    }

    printf("initialized HMD: %s - %s\n", _hmd->Manufacturer, _hmd->ProductName);
}

void Rift::initialize(int inputWidth, int inputHeight)
{
    unsigned int flags, dcaps, i;
    union ovrGLConfig glcfg;
    ovrHmd_ConfigureTracking(_hmd, 0xffffffff, 0);

    // Get window size
    _win_width = inputWidth; //_hmd->Resolution.w;
    _win_height = inputHeight; //_hmd->Resolution.h;
    _eyeres[0] = ovrHmd_GetFovTextureSize(_hmd, ovrEye_Left, _hmd->DefaultEyeFov[0], 1.0);
    _eyeres[1] = ovrHmd_GetFovTextureSize(_hmd, ovrEye_Right, _hmd->DefaultEyeFov[1], 1.0);
    // create a single render target to encompass both
    _fb_width = inputWidth; //_eyeres[0].w + _eyeres[1].w;
    _fb_height = inputHeight; //_eyeres[0].h > _eyeres[1].h ? _eyeres[0].h : _eyeres[1].h;
    _resolution.w = inputWidth;
    _resolution.h = inputHeight;
    update_rtarg(_fb_width, _fb_height);
    /* fill in the ovrGLTexture structures that describe our render target texture */
    for(i=0; i<2; i++) {
        _fb_ovr_tex[i].OGL.Header.API = ovrRenderAPI_OpenGL;
        _fb_ovr_tex[i].OGL.Header.TextureSize.w = _fb_tex_width;
        _fb_ovr_tex[i].OGL.Header.TextureSize.h = _fb_tex_height;
        /* this next field is the only one that differs between the two eyes */
        _fb_ovr_tex[i].OGL.Header.RenderViewport.Pos.x = i == 0 ? 0 : _fb_width / 2.0;
        _fb_ovr_tex[i].OGL.Header.RenderViewport.Pos.y = _fb_tex_height - _fb_height;
        _fb_ovr_tex[i].OGL.Header.RenderViewport.Size.w = _fb_width / 2.0;
        _fb_ovr_tex[i].OGL.Header.RenderViewport.Size.h = _fb_height;
        _fb_ovr_tex[i].OGL.TexId = _fb_tex;   /* both eyes will use the same texture id */
    }

    /* fill in the ovrGLConfig structure needed by the SDK to draw our stereo pair
     * to the actual HMD display (SDK-distortion mode)
     */
    memset(&glcfg, 0, sizeof glcfg);
    glcfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    glcfg.OGL.Header.RTSize = _resolution; //_hmd->Resolution;
    glcfg.OGL.Header.Multisample = 1;

    ovrHmd_SetEnabledCaps(_hmd, ovrHmdCap_LowPersistence | ovrHmdCap_DynamicPrediction);
    dcaps = ovrDistortionCap_Chromatic | ovrDistortionCap_Vignette | ovrDistortionCap_TimeWarp |
        ovrDistortionCap_Overdrive;

    if(!ovrHmd_ConfigureRendering(_hmd, &glcfg.Config, dcaps, _hmd->DefaultEyeFov, _eye_rdesc)) {
        printf("Failed to configure distortion renderer!\n");
    }
    // display health and safety warning
    ovrhmd_EnableHSWDisplaySDKRender(_hmd, 0);

    // Set the list of draw buffers.
    //GLenum DrawBuffers[2] = {GL_COLOR_ATTACHMENT0};
    //glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers
    // framebuffer is ok?
    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE){
        printf("Framebuffer problem.\n");
        exit(1);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //glUseProgram(_program_num);

    QueryPerformanceCounter(&_lasttime);
}

/* update_rtarg creates (and/or resizes) the render target used to draw the two stero views */
void Rift::update_rtarg(int width, int height)
{
 if(!_fbo) {
     /* if fbo does not exist, then nothing does... create every opengl object */
     glGenFramebuffers(1, &_fbo);
     glGenTextures(1, &_fb_tex);
     glGenRenderbuffers(1, &_fb_depth);
     glBindTexture(GL_TEXTURE_2D, _fb_tex);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
 }

 glBindFramebuffer(GL_FRAMEBUFFER, _fbo);

 /* calculate the next power of two in both dimensions and use that as a texture size */
 _fb_tex_width = next_pow2(width);
 _fb_tex_height = next_pow2(height);

 /* create and attach the texture that will be used as a color buffer */
 glBindTexture(GL_TEXTURE_2D, _fb_tex);
 glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, _fb_tex_width, _fb_tex_height, 0,
         GL_RGBA, GL_UNSIGNED_BYTE, 0);
 glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, _fb_tex, 0);

 /* create and attach the renderbuffer that will serve as our z-buffer */
 glBindRenderbuffer(GL_RENDERBUFFER, _fb_depth);
 glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, _fb_tex_width, _fb_tex_height);
 glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _fb_depth);

 if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
     fprintf(stderr, "incomplete framebuffer!\n");
 }

 glBindFramebuffer(GL_FRAMEBUFFER, 0);
 if (_verbose)
    printf("created render target: %dx%d (texture size: %dx%d)\n", width, height, _fb_tex_width, _fb_tex_height);
}

int Rift::set_resolution(int width, int height)
{
    initialize(width, height);
    return 0;
}

void Rift::normal_key_handler(unsigned char key, int x, int y){
    /*
    switch (key){
        case 'R':
            _SFusion->Reset();
            break;
        case '+':
        case '=':
            _SConfig->SetIPD(_SConfig->GetIPD() + 0.0005f);
            break;
        case '-':
        case '_':
            _SConfig->SetIPD(_SConfig->GetIPD() - 0.0005f);  
            break;     
        case 'c':
            _c_down = true;
            break;
    }
    */
}

void Rift::normal_key_up_handler(unsigned char key, int x, int y){
    /*
    switch (key){
        case 'c':
            _c_down = false;
        default:
            break;
    }
    */
}

void Rift::special_key_handler(int key, int x, int y){
    /*
    switch (key) {
        case GLUT_KEY_F1:
            _SConfig->SetStereoMode(Stereo_None);
            _PostProcess = PostProcess_None;
            break;
        case GLUT_KEY_F2:
            _SConfig->SetStereoMode(Stereo_LeftRight_Multipass);
            _PostProcess = PostProcess_None;
            break;
        case GLUT_KEY_F3:
            _SConfig->SetStereoMode(Stereo_LeftRight_Multipass);
            _PostProcess = PostProcess_Distortion;
            break;
    }
    */
}
void Rift::special_key_up_handler(int key, int x, int y){
    return;
}

void Rift::mouse(int button, int state, int x, int y) {
    /*
    if (state == GLUT_DOWN) {
        _mouseButtons |= 1<<button;
    } else if (state == GLUT_UP) {
        _mouseButtons = 0;
    }
    _mouseOldX = x;
    _mouseOldY = y;
    return;
    */
}

void Rift::motion(int x, int y) {
    /*
    // mouse emulates head orientation
    float dx, dy;
    dx = (float)(x - _mouseOldX);
    dy = (float)(y - _mouseOldY);

    if (_c_down){

        if (_mouseButtons == 1) {
            // left
            _EyeYaw -= dx * 0.001f;
            _EyePitch -= dy * 0.001f;
        } else if (_mouseButtons == 2) {   
        } else if (_mouseButtons == 4) {
            // right
            _EyeRoll += dx * 0.001f;
        }

    }

    _mouseOldX = x;
    _mouseOldY = y;  
    */
}


void Rift::onIdle() {
    QueryPerformanceCounter(&_currtime);

    float dt = float((unsigned long)(_currtime.QuadPart) - (unsigned long)(_lasttime.QuadPart));
    _lasttime = _currtime;

/*
    // Handle Sensor motion.
    // We extract Yaw, Pitch, Roll instead of directly using the orientation
    // to allow "additional" yaw manipulation with mouse/controller.
    if (_pSensor)
    {        
        Quatf    hmdOrient = _SFusion->GetOrientation();
        float    yaw = 0.0f;

        hmdOrient.GetEulerAngles<Axis_Y, Axis_X, Axis_Z>(&yaw, &_EyePitch, &_EyeRoll);

        _EyeYaw += (yaw - _LastSensorYaw);
        _LastSensorYaw = yaw;    
    }   
    */ 
}

void Rift::render(Vector3f EyePos, Vector3f EyeRot, void (*draw_scene)(void)){

 int i;
 ovrMatrix4f proj;
 ovrPosef pose[2];
 float rot_mat[16];

 // Rotate and position view camera BEFORE the individual eye poses
 Matrix4f rollPitchYaw = Matrix4f::RotationY(EyeRot.y) * 
                            Matrix4f::RotationX(EyeRot.x) *
                            Matrix4f::RotationZ(EyeRot.z);
 Vector3f up      = rollPitchYaw.Transform(UpVector);
 Vector3f forward = rollPitchYaw.Transform(ForwardVector);
 Matrix4f View = Matrix4f::LookAtRH(EyePos, EyePos + forward, up); 

 /* the drawing starts with a call to ovrHmd_BeginFrame */
 ovrHmd_BeginFrame(_hmd, 0);

 /* start drawing onto our texture render target */
 glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
 glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 /* for each eye ... */
 for(i=0; i<2; i++) {
     ovrEyeType eye = _hmd->EyeRenderOrder[i];

     _which_eye = (eye == ovrEye_Left ? 'l' : 'r');
     /* -- viewport transformation --
      * setup the viewport to draw in the left half of the framebuffer when we're
      * rendering the left eye's view (0, 0, width/2, height), and in the right half
      * of the framebuffer for the right eye's view (width/2, 0, width/2, height)
      */
     glViewport(eye == ovrEye_Left ? 0 : _fb_width / 2, 0, _fb_width / 2, _fb_height);

     /* -- projection transformation --
      * we'll just have to use the projection matrix supplied by the oculus SDK for this eye
      * note that libovr matrices are the transpose of what OpenGL expects, so we have to
      * use glLoadTransposeMatrixf instead of glLoadMatrixf to load it.
      */
     proj = ovrMatrix4f_Projection(_hmd->DefaultEyeFov[eye], 0.1, 1000.0, 1);
     glMatrixMode(GL_PROJECTION);
     glLoadTransposeMatrixf(proj.M[0]);

     /* -- view/camera transformation --
      * we need to construct a view matrix by combining all the information provided by the oculus
      * SDK, about the position and orientation of the user's head in the world.
      */
     pose[eye] = ovrHmd_GetEyePose(_hmd, eye);
     glMatrixMode(GL_MODELVIEW);
     glLoadIdentity();
     
     glTranslatef(_eye_rdesc[eye].ViewAdjust.x, _eye_rdesc[eye].ViewAdjust.y, _eye_rdesc[eye].ViewAdjust.z);

     /* retrieve the orientation quaternion and convert it to a rotation matrix */
     quat_to_matrix(&pose[eye].Orientation.x, rot_mat);
     glMultMatrixf(rot_mat);
     /* translate the view matrix with the positional tracking */
     glTranslatef(-pose[eye].Position.x, -pose[eye].Position.y, -pose[eye].Position.z);
     /* move the camera to the eye level of the user */
     //glTranslatef(0, -ovrHmd_GetFloat(_hmd, OVR_KEY_EYE_HEIGHT, 1.65), 0);

     // Load our desired translation
     GLfloat tmp[16];
     //glTranslatef( EyePos.x, EyePos.y, EyePos.z );
     for (int i=0; i<4; i++){
         for (int j=0; j<4; j++){
             // tranpose this too...
             tmp[j*4+i] = View.M[i][j];
         }
     }
     glMultMatrixf(tmp);

     /* finally draw the scene for this eye */
     glPushMatrix();
     draw_scene();
     glPopMatrix();
 }

 /* after drawing both eyes into the texture render target, revert to drawing directly to the
  * display, and we call ovrHmd_EndFrame, to let the Oculus SDK draw both images properly
  * compensated for lens distortion and chromatic abberation onto the HMD screen.
  */
 glBindFramebuffer(GL_FRAMEBUFFER, 0);
 glViewport(0, 0, _win_width, _win_height);

 ovrHmd_EndFrame(_hmd, pose, &_fb_ovr_tex[0].Texture);

 assert(glGetError() == GL_NO_ERROR);
 //glutSwapBuffers();  
}