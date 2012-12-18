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

#ifndef OSMIDSPLUGIN_H_
#define OSMIDSPLUGIN_H_

#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "BasePlugin.h"
#include "RouteParameters.h"

#include "../DataStructures/HashTable.h"
#include "../DataStructures/QueryEdge.h"
#include "../DataStructures/StaticGraph.h"
#include "../DataStructures/SearchEngine.h"

#include "../Util/StringUtil.h"

#include "../Server/DataStructures/QueryObjectsStorage.h"

class OsmIDsPlugin : public BasePlugin {

private:
    NodeInformationHelpDesk * nodeHelpDesk;
    std::vector<std::string> & names;
    StaticGraph<QueryEdge::EdgeData> * graph;
    SearchEngine<QueryEdge::EdgeData, StaticGraph<QueryEdge::EdgeData> > * searchEngine;

public:

    OsmIDsPlugin(QueryObjectsStorage * objects, std::string psd = "osmids") : names(objects->names) {
        nodeHelpDesk = objects->nodeHelpDesk;
        graph = objects->graph;

        searchEngine = new SearchEngine<QueryEdge::EdgeData, StaticGraph<QueryEdge::EdgeData> >(graph, nodeHelpDesk, names);
    }

    virtual ~OsmIDsPlugin() {
        delete searchEngine;
    }

    std::string GetDescriptor() const { return std::string("osmids"); }
    std::string GetVersionString() const { return std::string("0.3 (DL)"); }
    void HandleRequest(const RouteParameters & routeParameters, http::Reply& reply) {
        std::string tmp;

        // check number of parameters
        if( 2 > routeParameters.coordinates.size() ) {
            reply = http::Reply::stockReply(http::Reply::badRequest);
            return;
        }

        // get shortestPath
        RawRouteData rawRoute;
        rawRoute.checkSum = nodeHelpDesk->GetCheckSum();
        std::vector<std::string> textCoord;
        for(unsigned i = 0; i < routeParameters.coordinates.size(); ++i) {
            if(false == checkCoord(routeParameters.coordinates[i])) {
                reply = http::Reply::stockReply(http::Reply::badRequest);
                return;
            }
            rawRoute.rawViaNodeCoordinates.push_back(routeParameters.coordinates[i]);
        }
        std::vector<PhantomNode> phantomNodeVector(rawRoute.rawViaNodeCoordinates.size());
        for(unsigned i = 0; i < rawRoute.rawViaNodeCoordinates.size(); ++i) {
            searchEngine->FindPhantomNodeForCoordinate(rawRoute.rawViaNodeCoordinates[i], phantomNodeVector[i], routeParameters.zoomLevel);
        }
        for(unsigned i = 0; i < phantomNodeVector.size()-1; ++i) {
            PhantomNodes segmentPhantomNodes;
            segmentPhantomNodes.startPhantom = phantomNodeVector[i];
            segmentPhantomNodes.targetPhantom = phantomNodeVector[i+1];
            rawRoute.segmentEndCoordinates.push_back(segmentPhantomNodes);
        }
        searchEngine->shortestPath(rawRoute.segmentEndCoordinates, rawRoute);

        reply.status = http::Reply::ok;
        reply.content += ("[");

        if(rawRoute.lengthOfShortestPath != INT_MAX) {
            _Coordinate current;
            BOOST_FOREACH(const _PathData & pathData, rawRoute.computedShortestPath) {
                searchEngine->GetCoordinatesForNodeID(pathData.node, current);
                intToString(current.id, tmp);                
                reply.content += tmp;
                reply.content += ",";
            }
        }

        reply.content += ("null]");
        reply.headers.resize(2);
        reply.headers[0].name = "Content-Length";
        intToString(reply.content.size(), tmp);
        reply.headers[0].value = tmp;
        reply.headers[1].name = "Content-Type";
        reply.headers[1].value = "application/x-javascript";
    }
private:
    inline bool checkCoord(const _Coordinate & c) {
        if(c.lat > 90*100000 || c.lat < -90*100000 || c.lon > 180*100000 || c.lon <-180*100000) {
            return false;
        }
        return true;
    }
};


#endif /* OSMIDSPLUGIN_H_ */
