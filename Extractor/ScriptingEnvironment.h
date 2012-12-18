/*
 open source routing machine
 Copyright (C) Dennis Luxen, others 2010

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU AFFERO General Public License as published by
 the Free Software Foundation; either version 3 of the License, or
 any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 or see http://www.gnu.org/licenses/agpl.txt.
 */

#ifndef SCRIPTINGENVIRONMENT_H_
#define SCRIPTINGENVIRONMENT_H_

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}
#include <luabind/luabind.hpp>
#include <luabind/iterator_policy.hpp>

#include "ExtractionHelperFunctions.h"
#include "ExtractorStructs.h"
#include "LuaUtil.h"

#include "../DataStructures/ImportNode.h"

class ScriptingEnvironment {
public:
    ScriptingEnvironment();
    ScriptingEnvironment(const char * fileName, const char * osmFileName);
    virtual ~ScriptingEnvironment();

    lua_State * getLuaStateForThreadID(const int);

    std::vector<lua_State *> luaStateVector;
};

#endif /* SCRIPTINGENVIRONMENT_H_ */
