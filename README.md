<b>WikiTraverser</b>
_________________________________________________
<b>Description</b>
This program uses a dataset of wikipedia pages and all of their hyperlinks and finds
the shortest path to get from one webpage to another using Dijkstras algorithm, and
also finds a general path using a depth first search algorithm

<b>How to use </b>
In the directory the package is cloned into,
1) open a terminal and run <b><i>make</i></b> this will compile all the files
2) run <b><i>./wikiTraverser "start" "end"</i></b> with "start/end" being the titles
    of the wikipedia articles you want to find a path between
    * All possible articles can be found in data/links.tsv

<b>Example Output</b>
_________________________________________________

<img width="771" alt="Screen Shot 2021-12-29 at 9 25 11 PM" src="https://user-images.githubusercontent.com/67722662/147719310-4a2e596b-c18e-487d-9839-276979aec241.png">
