#include "OgreOculusRender.hpp"
#include <vector>


int main()
{

    bool tryClass(true);

    if(tryClass)
    {
        OgreOculusRender* oor = new OgreOculusRender("Ogre Oculus Render Test");
        oor->initialize();

        Ogre::SceneManager* smgr = oor->getSceneManager();
        Ogre::RenderWindow* window = oor->getWindow();

        Ogre::SceneNode* Sinbad = smgr->getRootSceneNode()->createChildSceneNode();
        Ogre::Entity* SinbadMesh = smgr->createEntity("Sinbad.mesh");
        Sinbad->attachObject(SinbadMesh);

        Sinbad->setPosition(0,0,-7);

        smgr->setAmbientLight(Ogre::ColourValue(1,1,1));

        while(!window->isClosed()) 
            oor->RenderOneFrame();
        delete oor;
    }
    return 0;
}
