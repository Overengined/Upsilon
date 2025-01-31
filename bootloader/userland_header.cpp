#include <bootloader/userland_header.h>

namespace Bootloader {

const UserlandHeader* s_UserlandHeaderA = reinterpret_cast<const struct UserlandHeader*>(0x90010000);
const UserlandHeader* s_UserlandHeaderB = reinterpret_cast<const struct UserlandHeader*>(0x90410000);

const char * UserlandHeader::version() const {
  return m_expectedEpsilonVersion;
}

const bool UserlandHeader::isValid() const {
  return m_header == Magic && m_footer == Magic;
}

const bool UserlandHeader::isOmega() const {
  return m_omegaMagicHeader == OmegaMagic && m_omegaMagicFooter == OmegaMagic;
}


const char * UserlandHeader::omegaVersion() const {
  return m_omegaVersion;
}

const bool UserlandHeader::isUpsilon() const {
  return m_upsilonMagicHeader == UpsilonMagic && m_upsilonMagicFooter == UpsilonMagic;
}

const char * UserlandHeader::upsilonVersion() const {
  return m_UpsilonVersion;
}

}
