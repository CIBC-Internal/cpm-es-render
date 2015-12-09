#ifndef IAUNS_RENDER_COMPONENT_FBO_HPP
#define IAUNS_RENDER_COMPONENT_FBO_HPP

#include <glm/glm.hpp>
#include <gl-platform/GLPlatform.hpp>
#include <es-cereal/ComponentSerialize.hpp>

namespace ren {

struct FBO
{
  // -- Data --
  GLuint glid;

  // -- Functions --
  FBO()
  {
    glid = 0;    
  }

  static const char* getName() {return "ren:FBO";}

  bool serialize(CPM_ES_CEREAL_NS::ComponentSerialize& /* s */, uint64_t /* entityID */)
  {
    /// Nothing needs to be serialized. This is context specific.
    return true;
  }
};

} // namespace ren

#endif 
