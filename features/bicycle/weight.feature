@routing @bicycle @weight
Feature: Way type weights
	
	Background:
		Given the speedprofile "bicycle"
	
	Scenario: Pick the geometrically shortest route, way types being equal
		Given the node map
		 |   | s |  | t |   |
		 | x | a |  | b | y |

		And the ways
		 | nodes | highway |
		 | xa    | primary |
		 | by    | primary |
		 | ab    | primary |
		 | astb  | primary |

		When I route I should get
		 | from | to | route    | distance  | time    |
		 | x    | y  | xa,ab,by | 400m +-10 | 85s +-2 |
		 | y    | x  | by,ab,xa | 400m +-10 | 85s +-2 |

	Scenario: Pick  the shortest travel time, even when it's longer
		Given the node map
		 |   | s |  | t |   |
		 | x | a |  | b | y |

		And the ways
		 | nodes | highway  |
		 | xa    | primary  |
		 | by    | primary  |
		 | ab    | primary  |
		 | astb  | cycleway |

		When I route I should get
		 | from | to | route      | distance  | time     |
		 | x    | y  | xa,astb,by | 600m +-10 | 128s +-2 |
		 | y    | x  | by,astb,xa | 600m +-10 | 128s +-2 |
