# this script extracts all bicycle relations into the *.osrm.networks file

import os
import sys
from imposm.parser import OSMParser

# create filenames
if len(sys.argv) < 2:
    sys.exit('Usage: %s osm-file' % sys.argv[0])
if not os.path.exists(sys.argv[1]):
    sys.exit('ERROR: osm-file %s was not found!' % sys.argv[1])
infile = sys.argv[1]
outfile = sys.argv[1].replace('.osm.pbf', '.osrm.networks')

# simple class that handles the parsed OSM data.
class Relation(object):
    names = []
    namesI = {}
    networks = {'icn': [], 'rcn': [], 'lcn': [], 'ncn': []}

    def check(self, relations):
        for osmid, tags, refs in relations:
            if tags.get('network') and tags['network'] in ('icn', 'rcn', 'ncn', 'lcn'):
                if not self.namesI.get(str(osmid)):
                    self.namesI[str(osmid)] = len(self.names)
                    self.names.append(tags.get('name'))
                for r in refs:
                    network = str(r[0]) + ";" + str(self.namesI[str(osmid)])
                    self.networks[tags['network']].append(network)

    def save(self, fname):
        f = open(fname, "w")
        f.write("|".join(map(lambda x: x or "", self.names)).encode('utf8') + "\n")
        for k, v in self.networks.items():
            f.write(k + "," + ",".join(map(lambda x: str(x), v)) + "\n")
        f.close()

# instantiate counter and parser and start parsing
relation = Relation()
p = OSMParser(concurrency=4, relations_callback=relation.check)
p.parse(infile)

# done
relation.save(outfile)
