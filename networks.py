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
    networks = {'rcn': [], 'lcn': [], 'ncn': []}

    def check(self, relations):
        for osmid, tags, refs in relations:
            if tags.get('network') and tags['network'] in ('rcn', 'ncn', 'lcn'):
                for r in refs: self.networks[tags['network']].append(r[0])

    def save(self, fname):
        f = open(fname, "w")
        for k, v in self.networks.items():
            f.write(k + "," + ",".join(map(lambda x: str(x), v)) + "\n");
        f.close()

# instantiate counter and parser and start parsing
relation = Relation()
p = OSMParser(concurrency=4, relations_callback=relation.check)
p.parse(infile)

# done
relation.save(outfile)
