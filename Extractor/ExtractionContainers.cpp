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

#include "ExtractionContainers.h"
#include "../Util/Lua.h"

void ExtractionContainers::PrepareData(const std::string & outputFileName, const std::string restrictionsFileName, const unsigned amountOfRAM, lua_State *myLuaState) {
    try {
        unsigned usedNodeCounter = 0;
        unsigned usedEdgeCounter = 0;
        double time = get_timestamp();
        boost::uint64_t memory_to_use = static_cast<boost::uint64_t>(amountOfRAM) * 1024 * 1024 * 1024;

        cout << "[extractor] Sorting used nodes        ... " << flush;
        stxxl::sort(usedNodeIDs.begin(), usedNodeIDs.end(), Cmp(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        time = get_timestamp();
        cout << "[extractor] Erasing duplicate nodes   ... " << flush;
        stxxl::vector<NodeID>::iterator NewEnd = unique ( usedNodeIDs.begin(),usedNodeIDs.end() ) ;
        usedNodeIDs.resize ( NewEnd - usedNodeIDs.begin() );
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] Sorting all nodes         ... " << flush;
        stxxl::sort(allNodes.begin(), allNodes.end(), CmpNodeByID(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] Sorting used ways         ... " << flush;
        stxxl::sort(wayStartEndVector.begin(), wayStartEndVector.end(), CmpWayByID(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        cout << "[extractor] Sorting restrctns. by from... " << flush;
        stxxl::sort(restrictionsVector.begin(), restrictionsVector.end(), CmpRestrictionContainerByFrom(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        cout << "[extractor] Fixing restriction starts ... " << flush;
        STXXLRestrictionsVector::iterator restrictionsIT = restrictionsVector.begin();
        STXXLWayIDStartEndVector::iterator wayStartAndEndEdgeIT = wayStartEndVector.begin();

        while(wayStartAndEndEdgeIT != wayStartEndVector.end() && restrictionsIT != restrictionsVector.end()) {
            if(wayStartAndEndEdgeIT->wayID < restrictionsIT->fromWay){
                ++wayStartAndEndEdgeIT;
                continue;
            }
            if(wayStartAndEndEdgeIT->wayID > restrictionsIT->fromWay) {
                ++restrictionsIT;
                continue;
            }
            assert(wayStartAndEndEdgeIT->wayID == restrictionsIT->fromWay);
            NodeID viaNode = restrictionsIT->restriction.viaNode;

            if(wayStartAndEndEdgeIT->firstStart == viaNode) {
                restrictionsIT->restriction.fromNode = wayStartAndEndEdgeIT->firstTarget;
            } else if(wayStartAndEndEdgeIT->firstTarget == viaNode) {
                restrictionsIT->restriction.fromNode = wayStartAndEndEdgeIT->firstStart;
            } else if(wayStartAndEndEdgeIT->lastStart == viaNode) {
                restrictionsIT->restriction.fromNode = wayStartAndEndEdgeIT->lastTarget;
            } else if(wayStartAndEndEdgeIT->lastTarget == viaNode) {
                restrictionsIT->restriction.fromNode = wayStartAndEndEdgeIT->lastStart;
            }
            ++restrictionsIT;
        }

        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] Sorting restrctns. by to  ... " << flush;
        stxxl::sort(restrictionsVector.begin(), restrictionsVector.end(), CmpRestrictionContainerByTo(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        time = get_timestamp();
        unsigned usableRestrictionsCounter(0);
        cout << "[extractor] Fixing restriction ends   ... " << flush;
        restrictionsIT = restrictionsVector.begin();
        wayStartAndEndEdgeIT = wayStartEndVector.begin();
        while(wayStartAndEndEdgeIT != wayStartEndVector.end() && restrictionsIT != restrictionsVector.end()) {
            if(wayStartAndEndEdgeIT->wayID < restrictionsIT->toWay){
                ++wayStartAndEndEdgeIT;
                continue;
            }
            if(wayStartAndEndEdgeIT->wayID > restrictionsIT->toWay) {
                ++restrictionsIT;
                continue;
            }
            NodeID viaNode = restrictionsIT->restriction.viaNode;
            if(wayStartAndEndEdgeIT->lastStart == viaNode) {
                restrictionsIT->restriction.toNode = wayStartAndEndEdgeIT->lastTarget;
            } else if(wayStartAndEndEdgeIT->lastTarget == viaNode) {
                restrictionsIT->restriction.toNode = wayStartAndEndEdgeIT->lastStart;
            } else if(wayStartAndEndEdgeIT->firstStart == viaNode) {
                restrictionsIT->restriction.toNode = wayStartAndEndEdgeIT->firstTarget;
            } else if(wayStartAndEndEdgeIT->firstTarget == viaNode) {
                restrictionsIT->restriction.toNode = wayStartAndEndEdgeIT->firstStart;
            }

            if(UINT_MAX != restrictionsIT->restriction.fromNode && UINT_MAX != restrictionsIT->restriction.toNode) {
                ++usableRestrictionsCounter;
            }
            ++restrictionsIT;
        }
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        INFO("usable restrictions: " << usableRestrictionsCounter );
        //serialize restrictions
        ofstream restrictionsOutstream;
        restrictionsOutstream.open(restrictionsFileName.c_str(), ios::binary);
        restrictionsOutstream.write((char*)&usableRestrictionsCounter, sizeof(unsigned));
        for(restrictionsIT = restrictionsVector.begin(); restrictionsIT != restrictionsVector.end(); ++restrictionsIT) {
            if(UINT_MAX != restrictionsIT->restriction.fromNode && UINT_MAX != restrictionsIT->restriction.toNode) {
                restrictionsOutstream.write((char *)&(restrictionsIT->restriction), sizeof(_Restriction));
            }
        }
        restrictionsOutstream.close();

        ofstream fout;
        fout.open(outputFileName.c_str(), ios::binary);
        fout.write((char*)&usedNodeCounter, sizeof(unsigned));
        time = get_timestamp();
        cout << "[extractor] Confirming/Writing used nodes     ... " << flush;

        STXXLNodeVector::iterator nodesIT = allNodes.begin();
        STXXLNodeIDVector::iterator usedNodeIDsIT = usedNodeIDs.begin();
        while(usedNodeIDsIT != usedNodeIDs.end() && nodesIT != allNodes.end()) {
            if(*usedNodeIDsIT < nodesIT->id){
                ++usedNodeIDsIT;
                continue;
            }
            if(*usedNodeIDsIT > nodesIT->id) {
                ++nodesIT;
                continue;
            }
            if(*usedNodeIDsIT == nodesIT->id) {
                fout.write((char*)&(*nodesIT), sizeof(_Node));
                ++usedNodeCounter;
                ++usedNodeIDsIT;
                ++nodesIT;
            }
        }

        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        cout << "[extractor] setting number of nodes   ... " << flush;
        ios::pos_type positionInFile = fout.tellp();
        fout.seekp(ios::beg);
        fout.write((char*)&usedNodeCounter, sizeof(unsigned));
        fout.seekp(positionInFile);

        cout << "ok" << endl;
        time = get_timestamp();

        // Sort edges by start.
        cout << "[extractor] Sorting edges by start    ... " << flush;
        stxxl::sort(allEdges.begin(), allEdges.end(), CmpEdgeByStartID(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] Setting start coords      ... " << flush;
        fout.write((char*)&usedEdgeCounter, sizeof(unsigned));
        // Traverse list of edges and nodes in parallel and set start coord
        nodesIT = allNodes.begin();
        STXXLEdgeVector::iterator edgeIT = allEdges.begin();
        while(edgeIT != allEdges.end() && nodesIT != allNodes.end()) {
            if(edgeIT->start < nodesIT->id){
                ++edgeIT;
                continue;
            }
            if(edgeIT->start > nodesIT->id) {
                nodesIT++;
                continue;
            }
            if(edgeIT->start == nodesIT->id) {
                edgeIT->startCoord.lat = nodesIT->lat;
                edgeIT->startCoord.lon = nodesIT->lon;
                edgeIT->startCoord.id = nodesIT->id;
                edgeIT->startCoord.altitude = nodesIT->altitude;
                ++edgeIT;
            }
        }
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        // Sort Edges by target
        cout << "[extractor] Sorting edges by target   ... " << flush;
        stxxl::sort(allEdges.begin(), allEdges.end(), CmpEdgeByTargetID(), memory_to_use);
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        time = get_timestamp();

        cout << "[extractor] Setting target coords     ... " << flush;
        // Traverse list of edges and nodes in parallel and set target coord
        nodesIT = allNodes.begin();
        edgeIT = allEdges.begin();

        bool have_segment_function = lua_function_exists(myLuaState, "segment_function");
        while(edgeIT != allEdges.end() && nodesIT != allNodes.end()) {
            if(edgeIT->target < nodesIT->id){
                ++edgeIT;
                continue;
            }
            if(edgeIT->target > nodesIT->id) {
                ++nodesIT;
                continue;
            }
            if(edgeIT->target == nodesIT->id) {
                if(edgeIT->startCoord.lat != INT_MIN && edgeIT->startCoord.lon != INT_MIN) {
                    edgeIT->targetCoord.lat = nodesIT->lat;
                    edgeIT->targetCoord.lon = nodesIT->lon;
                    edgeIT->targetCoord.id = nodesIT->id;
                    edgeIT->targetCoord.altitude = nodesIT->altitude;

                    assert(edgeIT->speed != -1);
                    assert(edgeIT->type >= 0);
                    double distance = ApproximateDistance(edgeIT->startCoord.lat, edgeIT->startCoord.lon, nodesIT->lat, nodesIT->lon);

                    if (have_segment_function) try {
                        luabind::object r = luabind::call_function<luabind::object> (
                            myLuaState,
                            "segment_function",
                            boost::ref(*edgeIT),
                            edgeIT->startCoord,
                            edgeIT->targetCoord,
                            distance
                        );
                    } catch (const luabind::error &er) {
                        lua_State* Ler=er.state();
                        std::cerr << "-- " << lua_tostring(Ler, -1) << std::endl;
                        lua_pop(Ler, 1); // remove error message
                        ERR(er.what());
                    }

                    double weight = ( distance * 10. ) / (edgeIT->speed / 3.6);
                    double weightBackward = ( distance * 10. ) / (((edgeIT->speedBackward == -1) ? edgeIT->speed : edgeIT->speedBackward) / 3.6);
                    int intDist = std::max(1, (int)distance);
                    int intWeight = std::max(1, (int)std::floor((edgeIT->isDurationSet ? edgeIT->speed : weight)+.5) );
                    int intWeightBackward = std::max(1, (int)std::floor((edgeIT->isDurationSet ? ((edgeIT->speedBackward == -1) ? edgeIT->speed : edgeIT->speedBackward) : weightBackward)+.5) );
                    short zero = 0, one = 1;

                    // hack create 2 edges of type oneway for different weight and weightBackward
                    if (intWeight != intWeightBackward && edgeIT->direction == _Way::bidirectional) {
                        fout.write((char*)&edgeIT->start, sizeof(unsigned));
                        fout.write((char*)&edgeIT->target, sizeof(unsigned));
                        fout.write((char*)&intDist, sizeof(int));
                        fout.write((char*)&one, sizeof(short));
                        fout.write((char*)&intWeight, sizeof(int));
                        fout.write((char*)&edgeIT->type, sizeof(short));
                        fout.write((char*)&edgeIT->nameID, sizeof(unsigned));
                        fout.write((char*)&edgeIT->isRoundabout, sizeof(bool));
                        fout.write((char*)&edgeIT->ignoreInGrid, sizeof(bool));
                        fout.write((char*)&edgeIT->isAccessRestricted, sizeof(bool));
                        ++usedEdgeCounter;

                        fout.write((char*)&edgeIT->target, sizeof(unsigned));
                        fout.write((char*)&edgeIT->start, sizeof(unsigned));
                        fout.write((char*)&intDist, sizeof(int));
                        fout.write((char*)&one, sizeof(short));
                        fout.write((char*)&intWeightBackward, sizeof(int));
                        fout.write((char*)&edgeIT->type, sizeof(short));
                        fout.write((char*)&edgeIT->nameID, sizeof(unsigned));
                        fout.write((char*)&edgeIT->isRoundabout, sizeof(bool));
                        fout.write((char*)&edgeIT->ignoreInGrid, sizeof(bool));
                        fout.write((char*)&edgeIT->isAccessRestricted, sizeof(bool));
                        ++usedEdgeCounter;

                    } else {
                        fout.write((char*)&edgeIT->start, sizeof(unsigned));
                        fout.write((char*)&edgeIT->target, sizeof(unsigned));
                        fout.write((char*)&intDist, sizeof(int));
                        switch(edgeIT->direction) {
                        case _Way::notSure:
                        case _Way::bidirectional:
                            fout.write((char*)&zero, sizeof(short));
                            break;
                        case _Way::oneway:
                        case _Way::opposite:
                            // FIXME! may switch intWeight and intWeightBackward in case of opposite
                            fout.write((char*)&one, sizeof(short));
                            break;
                        default:
                            cerr << "[error] edge with no direction: " << edgeIT->direction << endl;
                            assert(false);
                            break;
                        }
                        fout.write((char*)&intWeight, sizeof(int));
                        fout.write((char*)&edgeIT->type, sizeof(short));
                        fout.write((char*)&edgeIT->nameID, sizeof(unsigned));
                        fout.write((char*)&edgeIT->isRoundabout, sizeof(bool));
                        fout.write((char*)&edgeIT->ignoreInGrid, sizeof(bool));
                        fout.write((char*)&edgeIT->isAccessRestricted, sizeof(bool));
                        ++usedEdgeCounter;
                    }
                }
                ++edgeIT;
            }
        }
        cout << "ok, after " << get_timestamp() - time << "s" << endl;
        cout << "[extractor] setting number of edges   ... " << flush;

        fout.seekp(positionInFile);
        fout.write((char*)&usedEdgeCounter, sizeof(unsigned));
        fout.close();
        cout << "ok" << endl;
        time = get_timestamp();
        cout << "[extractor] writing street name index ... " << flush;
        std::string nameOutFileName = (outputFileName + ".names");
        ofstream nameOutFile(nameOutFileName.c_str(), ios::binary);
        unsigned sizeOfNameIndex = nameVector.size();
        nameOutFile.write((char *)&(sizeOfNameIndex), sizeof(unsigned));

        BOOST_FOREACH(string str, nameVector) {
            unsigned lengthOfRawString = strlen(str.c_str());
            nameOutFile.write((char *)&(lengthOfRawString), sizeof(unsigned));
            nameOutFile.write(str.c_str(), lengthOfRawString);
        }

        nameOutFile.close();
        cout << "ok, after " << get_timestamp() - time << "s" << endl;

        INFO("Processed " << usedNodeCounter << " nodes and " << usedEdgeCounter << " edges");


    } catch ( const exception& e ) {
        cerr <<  "Caught Execption:" << e.what() << endl;
    }
}

