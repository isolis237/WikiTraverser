<b>WikiTraverser</b>
_________________________________________________
<b>Description</b>
This program uses a dataset of wikipedia pages and all of their hyperlinks and finds
the shortest path to get from one webpage to another using Dijkstras algorithm, and
also finds a general path using a depth first search algorithm

<b>How to use </b>
In the directory the package is cloned into,
1) open a terminal and run <i>make</i> this will compile all the files
2) run <i>./wikiTraverser "start" "end"</i> with "start/end" being the titles
    of the wikipedia articles you want to find a path between
    * All possible articles can be found in data/links.tsv