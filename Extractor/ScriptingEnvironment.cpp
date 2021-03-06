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

extern "C" {
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

#include <boost/foreach.hpp>

#include "ScriptingEnvironment.h"
#include "../typedefs.h"
#include "../Util/OpenMPWrapper.h"

ScriptingEnvironment::ScriptingEnvironment() {}
ScriptingEnvironment::ScriptingEnvironment(const char * fileName, const char * osmFileName) {
	INFO("Using script " << fileName);

    // Create a new lua state
    for(int i = 0; i < omp_get_max_threads(); ++i)
        luaStateVector.push_back(luaL_newstate());

    // Connect LuaBind to this lua state for all threads
#pragma omp parallel
    {
        lua_State * myLuaState = getLuaStateForThreadID(omp_get_thread_num());
        luabind::open(myLuaState);
        //open utility libraries string library;
        luaL_openlibs(myLuaState);

        luabind::globals(myLuaState)["osmFileName"] = osmFileName;

        // Add our function to the state's global scope
        luabind::module(myLuaState) [
                                     luabind::def("print", LUA_print<std::string>),
                                     luabind::def("parseMaxspeed", parseMaxspeed),
                                     luabind::def("durationIsValid", durationIsValid),
                                     luabind::def("parseDuration", parseDuration)
        ];
//#pragma omp critical
//        {
//            if(0 != luaL_dostring(
//                    myLuaState,
//                    "print('Initializing LUA engine')\n"
//            )) {
//                ERR(lua_tostring(myLuaState,-1)<< " occured in scripting block");
//            }
//        }
        luabind::module(myLuaState) [
                                     luabind::class_<HashTable<std::string, std::string> >("keyVals")
                                     .def("Add", &HashTable<std::string, std::string>::Add)
                                     .def("Find", &HashTable<std::string, std::string>::Find)
                                     .def("Holds", &HashTable<std::string, std::string>::Holds)
                                     ];

        luabind::module(myLuaState) [
                                     luabind::class_<ImportNode>("Node")
                                     .def(luabind::constructor<>())
                                     .def_readwrite("lat", &ImportNode::lat)
                                     .def_readwrite("lon", &ImportNode::lon)
                                     .def_readwrite("id", &ImportNode::id)
                                     .def_readwrite("bollard", &ImportNode::bollard)
                                     .def_readwrite("traffic_light", &ImportNode::trafficLight)
                                     .def_readwrite("altitude", &ImportNode::altitude)
                                     .def_readwrite("tags", &ImportNode::keyVals)
                                     ];

        luabind::module(myLuaState) [
                                     luabind::class_<_Coordinate>("Coordinate")
                                     .def(luabind::constructor<>())
                                     .def_readwrite("lat", &_Coordinate::lat)
                                     .def_readwrite("lon", &_Coordinate::lon)
                                     .def_readwrite("id", &_Coordinate::id)
                                     .def_readwrite("altitude", &_Coordinate::altitude)
                                     ];

        luabind::module(myLuaState) [
                                     luabind::class_<_Way>("Way")
                                     .def(luabind::constructor<>())
                                     .def_readwrite("id", &_Way::id)
                                     .def_readwrite("name", &_Way::name)
                                     .def_readwrite("speed", &_Way::speed)
                                     .def_readwrite("speed_backward", &_Way::speedBackward)
                                     .def_readwrite("type", &_Way::type)
                                     .def_readwrite("access", &_Way::access)
                                     .def_readwrite("roundabout", &_Way::roundabout)
                                     .def_readwrite("is_duration_set", &_Way::isDurationSet)
                                     .def_readwrite("is_access_restricted", &_Way::isAccessRestricted)
                                     .def_readwrite("ignore_in_grid", &_Way::ignoreInGrid)
                                     .def_readwrite("path", &_Way::path, luabind::return_stl_iterator)
                                     .def_readwrite("tags", &_Way::keyVals)
                                     .def_readwrite("direction", &_Way::direction)
                                     .enum_("constants") [
                                        luabind::value("notSure", 0),
                                        luabind::value("oneway", 1),
                                        luabind::value("bidirectional", 2),
                                        luabind::value("opposite", 3)
                                     ]
                                    ];

        luabind::module(myLuaState) [
                                     luabind::class_<_Edge>("Edge")
                                     .def(luabind::constructor<>())
                                     .def_readwrite("start", &_Edge::start)
                                     .def_readwrite("target", &_Edge::target)
                                     .def_readwrite("type", &_Edge::type)
                                     .def_readwrite("direction", &_Edge::direction)
                                     .def_readwrite("speed", &_Edge::speed)
                                     .def_readwrite("speed_backward", &_Edge::speedBackward)
                                     .def_readwrite("name_id", &_Edge::nameID)
                                     .def_readwrite("is_roundabout", &_Edge::isRoundabout)
                                     .def_readwrite("is_duration_set", &_Edge::isDurationSet)
                                     .def_readwrite("is_access_restricted", &_Edge::isAccessRestricted)
                                     .def_readwrite("ignore_in_grid", &_Edge::ignoreInGrid)
                                    ];

        if(0 != luaL_dofile(myLuaState, fileName) ) {
            ERR(lua_tostring(myLuaState,-1)<< " occured in scripting block");
        }
    }
}

ScriptingEnvironment::~ScriptingEnvironment() {
    for(unsigned i = 0; i < luaStateVector.size(); ++i) {
        //        luaStateVector[i];
    }
}

lua_State * ScriptingEnvironment::getLuaStateForThreadID(const int id) {
    return luaStateVector[id];
}
