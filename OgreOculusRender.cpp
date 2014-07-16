#include "OgreOculusRender.hpp"
OgreOculusRender::OgreOculusRender(std::string winName)
{

    name = winName;
    root = NULL;
    window = NULL;
    smgr = NULL;
    for(char i(0); i < 2; i++)
    {
        cams[i] = NULL;
        rtts[i] = NULL;
        vpts[i] = NULL;
    }

    oc = NULL;
    CameraNode = NULL;
    cameraPosition = Ogre::Vector3(0,0,10);
    cameraOrientation = Ogre::Quaternion::IDENTITY;
    nearClippingDistance = 0.05;
    lastOculusPosition = cameraPosition;
    lastOculusOrientation = cameraOrientation;
    updateTime = 0;
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
    //Seems to only work in windows.
    misc["border"]="none";
    //TODO : find a way to open a borderless window on Linux (X system)
    window = root->initialise(false, name);
    window = root->createRenderWindow(name, oc->getHmdDesc().Resolution.w, oc->getHmdDesc().Resolution.h, false, &misc);
    window->reposition(oc->getHmdDesc().WindowsPos.x,oc->getHmdDesc().WindowsPos.y);
}

void OgreOculusRender::initCameras()
{
    assert(smgr != NULL);
    //Camera management is done at render time
    cams[left] = smgr->createCamera("lcam");
    cams[right] = smgr->createCamera("rcam");
    for(char i(0); i < 2; i++)
    {
        cams[i]->setPosition(cameraPosition);
        cams[i]->setAutoAspectRatio(true);
        cams[i]->setNearClipDistance(1);
        cams[i]->setFarClipDistance(1000);
    }
    //do NOT attach camera to this node... 
    CameraNode =  smgr->getRootSceneNode()->createChildSceneNode();

}

void OgreOculusRender::setCamerasNearClippingDistance(float distance)
{
    nearClippingDistance = distance;
}

void OgreOculusRender::initScene()
{
    assert(root != NULL);
    //Need plugin OctreeSceneManager.
    smgr = root->createSceneManager("OctreeSceneManager","OSM_SMGR");
}

void OgreOculusRender::initRttRendering()
{
    //Get texture size from ovr with default fov
    texSizeL = ovrHmd_GetFovTextureSize(oc->getHmd(), ovrEye_Left, oc->getHmdDesc().DefaultEyeFov[0], 1.0f);
    texSizeR = ovrHmd_GetFovTextureSize(oc->getHmd(), ovrEye_Right, oc->getHmdDesc().DefaultEyeFov[1], 1.0f);

    //Create texture
    Ogre::TexturePtr rtt_textureL = Ogre::TextureManager::getSingleton().createManual("RttTexL", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, texSizeL.w, texSizeL.h, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);

    Ogre::TexturePtr rtt_textureR = Ogre::TextureManager::getSingleton().createManual("RttTexR", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, Ogre::TEX_TYPE_2D, texSizeR.w, texSizeR.h, 0, Ogre::PF_R8G8B8, Ogre::TU_RENDERTARGET);
    
    //Create Render Texture
    Ogre::RenderTexture* rttEyeLeft = rtt_textureL->getBuffer(0,0)->getRenderTarget();
    Ogre::RenderTexture* rttEyeRight = rtt_textureR->getBuffer(0,0)->getRenderTarget();

    //Create and bind viewports to textures
    Ogre::ColourValue background(0.3f,0.3f,0.9f);
    
    Ogre::Viewport* vptl = rttEyeLeft->addViewport(cams[left]);  
    vptl->setBackgroundColour(background);

    Ogre::Viewport* vptr = rttEyeRight->addViewport(cams[right]);  
    vptr->setBackgroundColour(background);
    

    //Store viewport pointer
    vpts[left] = vptl;
    vpts[right] = vptr;

    //Populate textures with an initial render
    rttEyeLeft->update();
    rttEyeRight->update();
    
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
   
#ifdef __linux__
    size_t wID;
    window->getCustomAttribute("WINDOW", &wID);
    std::cout << "Wid : " << wID << endl;
    cfg.OGL.Win = wID;
    
    Display* display;
    window->getCustomAttribute("DISPLAY",&display);
    cfg.OGL.Disp = display;
    /////////////////////////////////////////////
#elif _WIN32
    //OpenGL context extraction from ogre needed. No time to reed doc now
    //Also nide a f*** windows.h include
#endif

    if(!ovrHmd_ConfigureRendering(
                oc->getHmd(),
                &cfg.Config,oc->getHmdDesc().DistortionCaps,
                EyeFov,
                EyeRenderDesc))
        abort(); //There is something WRONG if it doesn't work;
    
    //Feed OculusSDK with Texture info
    EyeTexture[left].OGL.Header.API = ovrRenderAPI_OpenGL; //I never heard about "Direct X". Is it something related to porn ? 
    EyeTexture[left].OGL.Header.TextureSize = texSizeL;
    EyeTexture[left].OGL.Header.RenderViewport.Pos.x = 0;
    EyeTexture[left].OGL.Header.RenderViewport.Pos.y = 0; 
    EyeTexture[left].OGL.Header.RenderViewport.Size = texSizeL; 
    
    EyeTexture[left].OGL.TexId = (static_cast<Ogre::GLTexture*>(Ogre::GLTextureManager::getSingleton().getByName("RttTexL").get()))->getGLID();
    
    EyeTexture[right] = EyeTexture[0];
    EyeTexture[right].OGL.Header.TextureSize = texSizeR;
    EyeTexture[right].OGL.Header.RenderViewport.Size = texSizeR;
    
    EyeTexture[right].OGL.TexId = (static_cast<Ogre::GLTexture*>(Ogre::GLTextureManager::getSingleton().getByName("RttTexR").get()))->getGLID();
}

void OgreOculusRender::RenderOneFrame()
{

    //Get info from the "Virtual Camera Node"
    cameraPosition = this->CameraNode->getPosition();
    cameraOrientation = this->CameraNode->getOrientation();
    
    ///Begin frame
    //Timing for OVR
    ovrFrameTiming hmdFrameTiming = ovrHmd_BeginFrame(oc->getHmd(), 0);
    //Ogre Event : Frame Started 
    root->_fireFrameStarted();
    //Let objects in scene get event
    for (Ogre::SceneManagerEnumerator::SceneManagerIterator it = root->getSceneManagerIterator(); it.hasMoreElements(); it.moveNext())
        it.peekNextValue()->_handleLodEvents();

    //Get Window messages from OS
    Ogre::WindowEventUtilities::messagePump();


    for(char eyeIndex(0); eyeIndex < ovrEye_Count; eyeIndex++)
    {

        //Get the correct eye to render
        ovrEyeType eye = oc->getHmdDesc().EyeRenderOrder[eyeIndex];

        //Set the Ogre render target to the texture (OpenGL will bind the correct viewport)
        root->getRenderSystem()->_setRenderTarget(rtts[eye]);

        //Get the eye pose 
        ovrPosef eyePose = ovrHmd_BeginEyeRender(oc->getHmd(), eye);
        
        //Get the hmd orientation
        OVR::Quatf camOrient = eyePose.Orientation;
        
        //Compute the projection matrix from FOV and clipping planes distances
        OVR::Matrix4f proj = ovrMatrix4f_Projection(EyeRenderDesc[eye].Fov,static_cast<float>(nearClippingDistance), 10000.0f, true);

        //Convert it to Ogre matrix 
        Ogre::Matrix4 OgreProj;
        for(char x(0); x < 4; x++)
            for(char y(0); y < 4; y++)
                OgreProj[x][y] = proj.M[x][y];
        
        //Set the matrix
        cams[eye]->setCustomProjectionMatrix(true, OgreProj);
        //Set the orientation
        cams[eye]->setOrientation(cameraOrientation * Ogre::Quaternion(camOrient.w,camOrient.x,camOrient.y,camOrient.z));

        //Set Position
        cams[eye]->setPosition( cameraPosition + 
                (cams[eye]->getOrientation() *
                 - //For some reason cams are inverted here. Take the oposite vector
                 Ogre::Vector3(
                     EyeRenderDesc[eye].ViewAdjust.x,
                     EyeRenderDesc[eye].ViewAdjust.y,
                     EyeRenderDesc[eye].ViewAdjust.z))
                );

        if(eye == left) //get an eye pos/orient for game logic
        {
            lastOculusPosition = cams[eye]->getPosition();
            lastOculusOrientation = cams[eye]->getOrientation();
        }

        //Send Ogre Event "Frame will render"
        root->_fireFrameRenderingQueued();

        //Get the texture from the computed viewpoint
        rtts[eye]->update();

        //Finaly render the distortion
        ovrHmd_EndEyeRender(oc->getHmd(), eye, eyePose, &EyeTexture[eye].Texture);
    }
    
    //Set the window at render target so OpenGL bind to the "viewport" on the window (I haven't created a viewport but it seems Oculus lib takes care of it);
    root->getRenderSystem()->_setRenderTarget(window); 
    //Get timing from Oculus
    updateTime = hmdFrameTiming.DeltaSeconds;

    //End frame timing
    ovrHmd_EndFrame(oc->getHmd());
    //Fire Ogre ended frame event.
    root->_fireFrameEnded();
}

