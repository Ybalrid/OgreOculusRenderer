#include "OgreOculusRender.hpp"
#include <cstdio>
OgreOculusRender::OgreOculusRender(std::string winName)
{

    name = winName;
    root = NULL;
    window = NULL;
    smgr = NULL;
    for(size_t i(0); i < 2; i++)
    {
        cams[i] = NULL;
        rtts[i] = NULL;
        vpts[i] = NULL;
    }

    oc = NULL;
    cameraPosition = Ogre::Vector3(0,0,10);
}

OgreOculusRender::~OgreOculusRender()
{
    delete oc;
}

void OgreOculusRender::loadReseourceFile(const char path[])
{
    /*from ogre wiki : load the given resource file*/
    Ogre::ConfigFile configFile;
    configFile.load(path);

    Ogre::ConfigFile::SectionIterator seci = configFile.getSectionIterator();
    Ogre::String secName, typeName, archName;
    while (seci.hasMoreElements())
    {
        secName = seci.peekNextKey();
        Ogre::ConfigFile::SettingsMultiMap *settings = seci.getNext();
        Ogre::ConfigFile::SettingsMultiMap::iterator i;
        for (i = settings->begin(); i != settings->end(); ++i)
        {
            typeName = i->first;
            archName = i->second;
            Ogre::ResourceGroupManager::getSingleton().addResourceLocation(
                    archName, typeName, secName);
        }
    }  
}

void OgreOculusRender::initAllResources()
{
    Ogre::ResourceGroupManager::getSingleton().initialiseAllResourceGroups();
}

void OgreOculusRender::initLibraries()
{
    //Create the ogre root
    root = new Ogre::Root("plugins.cfg","ogre.cfg","Ogre.log");
    //Class to get basic information from the Rift. Initialize the RiftSDK 
    oc = new OculusInterface();
}

void OgreOculusRender::initialize()
{
    //init libraries;
    initLibraries();

    assert(root != NULL && oc != NULL);

    //Get configuration via ogre.cfg OR via the config dialog.
    getOgreConfig();
    
    //Create the render window with the given sice from the Oculus 
    createWindow();
    
    //Load resources from the resources.cfg file
    loadReseourceFile("resources.cfg");
    initAllResources();

    //Create scene manager
    initScene();
    
    //Create cameras and handeling nodes
    initCameras();

    //Create rtts and viewports on them
    initRttRendering();
    
    //Init the oculus rendering
    initOculus();
}

void OgreOculusRender::getOgreConfig()
{
    assert(root != NULL);
    if(!root->restoreConfig())
        if(!root->showConfigDialog())
            abort();
}

void OgreOculusRender::createWindow()
{
    assert(root != NULL && oc != NULL);
    Ogre::NameValuePairList misc;
    misc["border"]="none";
    window = root->initialise(false, name);
    window = root->createRenderWindow(name, oc->getHmdDesc().Resolution.w, oc->getHmdDesc().Resolution.h, false, &misc);
    window->reposition(oc->getHmdDesc().WindowsPos.x,oc->getHmdDesc().WindowsPos.y);
}

void OgreOculusRender::initCameras()
{
    cams[left] = smgr->createCamera("lcam");
    cams[right] = smgr->createCamera("rcam");

    //default IPD for now : 0.0064 
    cams[left]->setPosition(cams[left]->getPosition() + Ogre::Vector3(-0.064/2,0,0));
    cams[right]->setPosition(cams[right]->getPosition() + Ogre::Vector3(+0.064/2,0,0));


    Ogre::Vector3 lookAt(0,0,0);

    cams[left]->lookAt(lookAt);
    cams[right]->lookAt(lookAt);

    for(int i = 0; i < 2; i++)
    {
        cams[i]->setPosition(cameraPosition);
        cams[i]->setAutoAspectRatio(true);
        cams[i]->setNearClipDistance(0.01);
        cams[i]->setFarClipDistance(1000);
    }
}

void OgreOculusRender::initScene()
{
    assert(root != NULL);
    smgr = root->createSceneManager("OctreeSceneManager","OSM_SMGR");
}

void OgreOculusRender::initRttRendering()
{
    //get texture sice from ovr with default fov
    texSizeL = ovrHmd_GetFovTextureSize(oc->getHmd(), ovrEye_Left, oc->getHmdDesc().DefaultEyeFov[0], 1.0f);
    texSizeR = ovrHmd_GetFovTextureSize(oc->getHmd(), ovrEye_Right, oc->getHmdDesc().DefaultEyeFov[1], 1.0f);

    //Create texture
    Ogre::TexturePtr rtt_textureL = Ogre::TextureManager::getSingleton().createManual("RttTexL", 
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
            Ogre::TEX_TYPE_2D, 
            texSizeL.w, 
            texSizeL.h, 
            0, 
            Ogre::PF_R8G8B8, 
            Ogre::TU_RENDERTARGET);

    Ogre::TexturePtr rtt_textureR = Ogre::TextureManager::getSingleton().createManual("RttTexR", 
            Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, 
            Ogre::TEX_TYPE_2D, 
            texSizeR.w, 
            texSizeR.h, 
            0, 
            Ogre::PF_R8G8B8, 
            Ogre::TU_RENDERTARGET);

    //Create Render Texture 
    Ogre::RenderTexture* rttEyeLeft = rtt_textureL->getBuffer(0,0)->getRenderTarget();
    Ogre::RenderTexture* rttEyeRight = rtt_textureR->getBuffer(0,0)->getRenderTarget();

    //Create and bind a viewport to the texture
    Ogre::Viewport* vptl = rttEyeLeft->addViewport(cams[left]);  
    vptl->setBackgroundColour(Ogre::ColourValue(0.3,0.3,0.9));
    Ogre::Viewport* vptr = rttEyeRight->addViewport(cams[right]);  
    vptr->setBackgroundColour(Ogre::ColourValue(0.3,0.3,0.9));
    
    //Store viewport pointer
    vpts[left] = vptl;
    vpts[right] = vptr;

    //Pupulate textures with an initial render
    rttEyeLeft->update();
    rttEyeRight->update();
    
    rttEyeLeft->writeContentsToFile("debug_init_cam_left.png");

    //Store rtt textures pointer
    rtts[left] = rttEyeLeft;
    rtts[right] = rttEyeRight;
}

void OgreOculusRender::initOculus()
{
    //Get FOV
    EyeFov[left] = oc->getHmdDesc().DefaultEyeFov[left];
    EyeFov[right] = oc->getHmdDesc().DefaultEyeFov[right];

    //Set OpenGL configuration 
    ovrGLConfig cfg;
    cfg.OGL.Header.API = ovrRenderAPI_OpenGL;
    cfg.OGL.Header.Multisample = 1;
    cfg.OGL.Header.RTSize = oc->getHmdDesc().Resolution;
    
    ///////////////////// LINUX SPECIFIC 
    //TODO define to protect that and to put Windows equivalent 
    size_t wID;
    window->getCustomAttribute("WINDOW", &wID);
    std::cout << "Wid : " << wID << endl;
    cfg.OGL.Win = wID;
    
    Display* display;
    window->getCustomAttribute("DISPLAY",&display);
    cfg.OGL.Disp = display;
    /////////////////////////////////////////////

    if(!ovrHmd_ConfigureRendering(
                oc->getHmd(),
                &cfg.Config,oc->getHmdDesc().DistortionCaps,
                EyeFov,
                EyeRenderDesc))
        abort();
    
    EyeTexture[left].OGL.Header.API = ovrRenderAPI_OpenGL;
    EyeTexture[left].OGL.Header.TextureSize = texSizeL;
    EyeTexture[left].OGL.Header.RenderViewport.Pos.x = 0;
    EyeTexture[left].OGL.Header.RenderViewport.Pos.y = 0; 
    EyeTexture[left].OGL.Header.RenderViewport.Size = texSizeL; 
    
    Ogre::GLTexture* gl_rtt_l = static_cast<Ogre::GLTexture*>(Ogre::GLTextureManager::getSingleton().getByName("RttTexL").get());
    EyeTexture[left].OGL.TexId = gl_rtt_l->getGLID();
    
    EyeTexture[right] = EyeTexture[0];
    EyeTexture[right].OGL.Header.TextureSize = texSizeR;
    EyeTexture[right].OGL.Header.RenderViewport.Size = texSizeR;
    
    Ogre::GLTexture* gl_rtt_r = static_cast<Ogre::GLTexture*>(Ogre::GLTextureManager::getSingleton().getByName("RttTexR").get());
    EyeTexture[right].OGL.TexId = gl_rtt_r->getGLID();
}

void OgreOculusRender::writeTextureToFile()
{
    //It's a C style function, yeah...

    char path[] = "debug.ppm";
    FILE* debugFile = fopen(path,"w");
    fprintf(debugFile,"P3\n");//PPM ASCII MAGIC NUMBER
    
    //W H size
    fprintf(debugFile, "%d %d\n" ,texSizeL.w, texSizeL.h);
    
    //Compute the biggest nomber an unsigned int cat represent on this computer : 
    unsigned int  max = 0;
    max--;

    //MAX SUBPIXEL VALULE
    fprintf(debugFile, "%u\n", 255);

    size_t arraySize =  texSizeL.w*texSizeL.h;
    //Cast because C++ will not like affecting a void* on a int* 
    unsigned int* rIntArray = static_cast<unsigned int*>(malloc(sizeof(unsigned int) * arraySize));
    unsigned int* gIntArray = static_cast<unsigned int*>(malloc(sizeof(unsigned int) * arraySize));
    unsigned int* bIntArray = static_cast<unsigned int*>(malloc(sizeof(unsigned int) * arraySize));
    
    GLuint tex = static_cast<Ogre::GLTexture*>(Ogre::GLTextureManager::getSingleton().getByName("RttTexL").get())->getGLID();
    
    glBindTexture(GL_TEXTURE_2D, tex);
    
    //I'm not searching to do this right, I just want to get the data in any format I can...
    glGetTexImage(GL_TEXTURE_2D,0, GL_RED,GL_UNSIGNED_INT, static_cast<GLvoid*>(rIntArray));
    glGetTexImage(GL_TEXTURE_2D,0, GL_GREEN, GL_UNSIGNED_INT, static_cast<GLvoid*>(gIntArray));
    glGetTexImage(GL_TEXTURE_2D,0, GL_BLUE, GL_UNSIGNED_INT, static_cast<GLvoid*>(bIntArray));
    
    //Convert to a 255 max value
    for(int i = 0; i < arraySize; i++)
    {
        rIntArray[i] =(int) (((float)rIntArray[i]/(float)max)*(float)255);
        gIntArray[i] =(int) (((float)gIntArray[i]/(float)max)*(float)255);
        bIntArray[i] =(int) (((float)bIntArray[i]/(float)max)*(float)255);
    }

    for(int i = 0; i < arraySize; i++)
    {
        fprintf(debugFile,"%u %u %u",rIntArray[i], gIntArray[i], bIntArray[i]);
        if(!(i%80)) fprintf(debugFile,"\n");
        else fprintf(debugFile," ");
    }

    free(rIntArray);
    free(gIntArray);
    free(bIntArray);

    fclose(debugFile);
}

void OgreOculusRender::RenderOneFrame()
{
    //Begin frame
    ovrFrameTiming hmdFrameTiming = ovrHmd_BeginFrame(oc->getHmd(), 0);
    //Message pump events 
    Ogre::WindowEventUtilities::messagePump();

    for(int eyeIndex = 0; eyeIndex < ovrEye_Count; eyeIndex++)
    {
        //Get the correct eye to render
        ovrEyeType eye = oc->getHmdDesc().EyeRenderOrder[eyeIndex];
        
        //Set the Ogre render target to the texture
        root->getRenderSystem()->_setRenderTarget(rtts[eye]);
        
        //Get the eye pose 
        ovrPosef eyePose = ovrHmd_BeginEyeRender(oc->getHmd(), eye);
        
        //Get the hmd orientation
        OVR::Quatf camOrient = eyePose.Orientation;
        
        //Get the projection matrix
        OVR::Matrix4f proj = ovrMatrix4f_Projection(EyeRenderDesc[eye].Fov, 0.01f, 10000.0f, true);

        //Convert it to Ogre matrix
        Ogre::Matrix4 OgreProj;
        for(int x(0); x < 4; x++)
            for(int y(0); y < 4; y++)
                OgreProj[x][y] = proj.M[x][y];

        //Set the matrix
        cams[eye]->setCustomProjectionMatrix(true, OgreProj);
        //Set the orientation
        cams[eye]->setOrientation(Ogre::Quaternion(camOrient.w,camOrient.x,camOrient.y,camOrient.z));
    
        //Set Position
        cams[eye]->setPosition( cameraPosition + 
                Ogre::Vector3(EyeRenderDesc[eye].ViewAdjust.x,
                    EyeRenderDesc[eye].ViewAdjust.y,
                    EyeRenderDesc[eye].ViewAdjust.z));
    
        rtts[eye]->update();
        writeTextureToFile();
        ovrHmd_EndEyeRender(oc->getHmd(), eye, eyePose, &EyeTexture[eye].Texture);
    }
    Ogre::Root::getSingleton().getRenderSystem()->_setRenderTarget(window); 
    ovrHmd_EndFrame(oc->getHmd());
}

