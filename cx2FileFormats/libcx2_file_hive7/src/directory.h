#ifndef HIVE7_DIRECTORY_H
#define HIVE7_DIRECTORY_H

#include "registryarray.h"
#include "registry.h"
#include "link.h"

#include <string>
#include <list>

namespace CX2 { namespace Files { namespace Hive7 {

enum eH7FileType
{
    H7_FT_REG,
    H7_FT_LINK,
    H7_FT_REGARRAY,
    H7_FT_DIR
};

class Directory : public Attributes, public Permissions
{
public:
    Directory();

    // TODO: substitute Hive7 for namespace.

    // Find by glob format *.*
    //std::list<std::pair<eHive7FileType,std::string>> find( StackedRules );

    // Links
    Link createLink( const std::string & linkName,  Registry reg,  bool soft = true );
    Link createLink( const std::string & linkName,  RegistryArray reg,  bool soft = true );
    Link createLink( const std::string & linkName,  Directory dir,  bool soft = true );
    Link openLink(  const std::string & linkName );

    // Registry Arrays
    Registry createRegArray( const std::string & regArrayName );
    Registry openRegArray(  const std::string & regArrayName );

    // Registries
    Registry createReg( const std::string & regName );
    Registry openReg(  const std::string & regName );

    // Directories
    Directory createDir( const std::string & dirName );
    Directory openDir( const std::string & dirName );
};

}}}

#endif // HIVE7_DIRECTORY_H
