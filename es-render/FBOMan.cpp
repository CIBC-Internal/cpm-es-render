
#include <set>
#include <stdexcept>
#include <string>

#include <entity-system/GenericSystem.hpp>
#include <es-systems/SystemCore.hpp>

#include "FBOMan.hpp"
#include "comp/StaticFBOMan.hpp"
#include "comp/FBO.hpp"

namespace es = CPM_ES_NS;
namespace shaders = CPM_GL_SHADERS_NS;

namespace ren {

FBOMan::FBOMan()
{
}

FBOMan::~FBOMan()
{
  for (auto it = mFBOData.begin(); it != mFBOData.end(); ++it)
  {
    GLuint idToErase = it->first;
    GL(glDeleteBuffers(1, &idToErase));
  }
  mFBOData.clear();
}

GLuint FBOMan::addInMemoryFBO(void* fboData, size_t fboDataSize, GLenum primMode,
                              GLenum primType, GLsizei numPrims,
                              const std::string& assetName)
{
  GLuint glid;

  GL(glGenBuffers(1, &glid));
  GL(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glid));
  GL(glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(fboDataSize),
                  fboData, GL_STATIC_DRAW));

  mFBOData.insert(std::make_pair(glid, FBOData(assetName, primMode, primType, numPrims)));

  return glid;
}

void FBOMan::removeInMemoryFBO(GLuint glid)
{
  auto iter =  mFBOData.find(glid);
  mFBOData.erase(iter);

  GL(glDeleteBuffers(1, &glid));
}

GLuint FBOMan::hasFBO(const std::string& assetName) const
{
  for (auto it = mFBOData.begin(); it != mFBOData.end(); ++it)
  {
    if (it->second.assetName == assetName)
    {
      return it->first;
    }
  }
  return 0;
}

const FBOMan::FBOData& FBOMan::getFBOData(const std::string& assetName) const
{
  for (auto it = mFBOData.begin(); it != mFBOData.end(); ++it)
  {
    if (it->second.assetName == assetName)
    {
      return it->second;
    }
  }

  throw std::runtime_error("FBOMan: Unable to find FBO data");
}

//------------------------------------------------------------------------------
// GARBAGE COLLECTION
//------------------------------------------------------------------------------

void FBOMan::runGCAgainstVaidIDs(const std::set<GLuint>& validKeys)
{
  // Every GLuint in validKeys should be in our map. If there is not, then
  // there is an error in the system, and it should be reported.
  // The reverse is not expected to be true, and is what we are attempting to
  // correct with this function.
  auto it = mFBOData.begin();
  for (const GLuint& id : validKeys)
  {
    // Find the key in the map, eliminating any keys that do not match the
    // current id along the way.
    while (it != mFBOData.end() && it->first < id)
    {
      //\cb std::cout << "FBO GC: " << it->second.assetName << std::endl;

      GLuint idToErase = it->first;
      mFBOData.erase(it++);
      GL(glDeleteBuffers(1, &idToErase));
    }

    if (it == mFBOData.end())
    {
      std::cerr << "runGCAgainstVaidIDs: terminating early, validKeys contains "
                << "elements not in FBO map." << std::endl;
      break;
    }

    // Check to see if the valid ids contain a component that is not in
    // mFBOData. If an object manages its own FBO, but still uses the FBO
    // component, this is not an error.
    if (it->first > id)
    {
      std::cerr << "runGCAgainstVaidIDs: validKeys contains elements not in the FBO map." << std::endl;
    }

    // Increment passed current validKey id.
    ++it;
  }

  while (it != mFBOData.end())
  {
    //\cb std::cout << "FBO GC: " << it->second.assetName << std::endl;

    GLuint idToErase = it->first;
    mFBOData.erase(it++);
    GL(glDeleteBuffers(1, &idToErase));
  }
}

class FBOGarbageCollector :
    public es::GenericSystem<false, FBO>
{
public:

  static const char* getName()    {return "ren:FBOGarbageCollector";}

  std::set<GLuint> mValidKeys;

  void preWalkComponents(es::ESCoreBase&) {mValidKeys.clear();}
  void postWalkComponents(es::ESCoreBase& core)
  {
    std::weak_ptr<FBOMan> im = core.getStaticComponent<StaticFBOMan>()->instance_;
    if (std::shared_ptr<FBOMan> man = im.lock()) {
      man->runGCAgainstVaidIDs(mValidKeys);
      mValidKeys.clear();
    }
    else
    {
      std::cerr << "FBOMan: StaticFBOMan not found is given core. Failed to run FBO GC." << std::endl;
    }
  }

  void execute(es::ESCoreBase&, uint64_t /* entityID */, const FBO* fbo) override
  {
    mValidKeys.insert(fbo->glid);
  }
};

void FBOMan::registerSystems(CPM_ES_ACORN_NS::Acorn& core)
{
  // Register the garbage collector.
  core.registerSystem<FBOGarbageCollector>();
}

void FBOMan::runGCCycle(CPM_ES_NS::ESCoreBase& core)
{
  FBOGarbageCollector gc;
  gc.walkComponents(core);
}

const char* FBOMan::getGCName()
{
  return FBOGarbageCollector::getName();
}

} // namespace ren

