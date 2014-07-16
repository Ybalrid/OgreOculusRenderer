#include <glew.h>
#include <glxew.h>
#include <iostream>
#include <OVR.h>
#include <OVR_CAPI.h>
#include <OVR_CAPI_GL.h>
#include <CAPI/GL/CAPI_GL_Util.h>

#include <unistd.h>

#include <Ogre.h>
#include <OgreTexture.h>
#include <RenderSystems/GL/OgreGLRenderSystem.h>
#include <RenderSystems/GL/OgreGLRenderTexture.h>
#include <RenderSystems/GL/OgreGLTexture.h>
#include <RenderSystems/GL/OgreGLTextureManager.h>

#include "OculusInterface.hpp"

using namespace std;
using namespace OVR;


class OgreOculusRender
{
    public:
        OgreOculusRender(std::string windowName = "OgreOculusRender");
        ~OgreOculusRender();
        
        //Automaticly load resources.cfg file and config everything
        void initialize();

        //Init both Ogre and OVR
        void initLibraries();

        //Get Data form the given .cfg file and regiter into ResourceManager
        void loadReseourceFile(const char path[]);

        //Init all declared resource groups
        void initAllResources();
        
        //Get ogre configuration from ogre.cfg (or Config window if fails)
        void getOgreConfig();
        
        //Create rendering window
        void createWindow();

        //Init scene manager
        void initScene();

        //Create stereoscopic cam 
        void initCameras();

        //Create Textures for intermediate rendering and bind cameras to viewports on it. 2 texture (one for each eye);
        void initRttRendering();
        
        //Init Oculus Rendering 
        void initOculus();

        //Do the same thing that should do Ogre::Root::renderOneFrame, but let Oculus take care of buffer swpaing and frame timing
        void RenderOneFrame();
        
        //Set near clipping plane distance
        void setCamerasNearClippingDistance(float distance);

        //Print basic information
        void debugPrint()
        {
            for(char i(0); i < 2; i++)
            {
               cout << "cam " << i << " " << cams[i]->getPosition() << endl;
               cout << cams[i]->getOrientation() << endl;
            }

        }
        
        //GETTERS

        //Write texture to image file. Compression chousen from file extension (have to be a free image compatible type)
        void debugSaveToFile(const char path[])
        {
            if(rtts[0])rtts[0]->writeContentsToFile(path);
        }

        //Get a pointer to a node acting like a virtual camera
        Ogre::SceneNode* getCameraInformationNode()
        {
            return CameraNode;
        }
        
        //Gett the Ogre Root timer. but please consider using the update time value below, it comes from Oculus Frame Timing 
        Ogre::Timer* getTimer()
        {
            if(root)
                return root->getTimer();
            return NULL;
        }
        
        //Get the Oculus "UpdateFrame" delta-time (seconds)
        float getUpdateTime()
        {
            return updateTime;
        }

        //Get the SceneManager
        Ogre::SceneManager* getSceneManager()
        {
            return smgr;
        }

        //Get the RenderWindow
        Ogre::RenderWindow* getWindow()
        {
            return window;
        }


    private:
        enum 
        {
            left = 0,
            right = 1
        };

        //Name of the Window
        string name;
        //Ogre
        Ogre::Root* root;
        Ogre::RenderWindow* window;
        Ogre::SceneManager* smgr;
        Ogre::Camera* cams[2];
        Ogre::SceneNode* CameraNode;
        Ogre::RenderTexture* rtts[2];
        Ogre::Viewport* vpts[2];
        Ogre::Real nearClippingDistance;
        float updateTime; //seconds
        
        //Oculus
        OculusInterface* oc;
        
        ovrFovPort EyeFov[2];
        ovrEyeRenderDesc EyeRenderDesc[2];
        ovrGLConfig cfg; //OpenGL
        ovrGLTexture EyeTexture[2];
        ovrSizei texSizeL;
        ovrSizei texSizeR;

        Ogre::Vector3 cameraPosition;
        Ogre::Quaternion cameraOrientation;

    public: //Only for game logic, does not do anything with the render.
        Ogre::Vector3 lastOculusPosition;
        Ogre::Quaternion lastOculusOrientation;
};

