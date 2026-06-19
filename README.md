# WikiTraverser

WikiTraverser builds a directed graph from Wikipedia link data, finds paths
between articles, and exports clustered graph subsets for an interactive browser
visualization.

## Build

```bash
make
```

## Code Layout

- `include/` contains the graph, algorithm, clustering, and export interfaces.
- `src/` contains the CLI entry point and C++ implementations.
- `visualization/` contains the browser UI and local graph-generation server.
- `data/` contains the input TSV and generated binary cache.

## Run The Visualizer

Build once, then start the local visualization server:

```bash
make
python3 visualization/server.py
```

Open:

```text
http://localhost:8000/
```

If port 8000 is busy, use:

```bash
python3 visualization/server.py --port 8765
```

This server serves the browser app and lets the UI generate new Louvain graph
JSON files locally through the C++ exporter.

## Customize Louvain Graphs

In the sidebar:

- Choose start and end pages.
- Set `Nodes` for the exported graph size.
- Set `Resolution` to control Louvain cluster granularity.
- Set `Depth` and `Neighbors` to control how much of the surrounding graph is included.
- Press `Generate`, then use `Path` and `Animate`.

Resolution guide:

- Lower values, such as `0.6`, create fewer larger clusters.
- `1.0` is the default modularity resolution.
- Higher values, such as `2.0`, create more smaller clusters.

## Generate From The Terminal

The visualizer loads JSON files from `visualization/`. These commands generate
the default Louvain small, medium, and large presets:

```bash
./wikiTraverser export data/links.tsv visualization/graph-small.json \
  --start "Military_dictatorship" --end "Michael_Jordan" \
  --nodes 140 --depth 2 --neighbors 24 \
  --cluster louvain --cluster-resolution 1.0

./wikiTraverser export data/links.tsv visualization/graph.json \
  --start "Military_dictatorship" --end "Michael_Jordan" \
  --nodes 280 --depth 2 --neighbors 36 \
  --cluster louvain --cluster-resolution 1.0

./wikiTraverser export data/links.tsv visualization/graph-large.json \
  --start "Military_dictatorship" --end "Michael_Jordan" \
  --nodes 520 --depth 3 --neighbors 48 \
  --cluster louvain --cluster-resolution 1.0
```

Example with more Louvain clusters:

```bash
./wikiTraverser export data/links.tsv visualization/graph-louvain-r2p0.json \
  --start "Military_dictatorship" --end "Michael_Jordan" \
  --nodes 280 --depth 2 --neighbors 36 \
  --cluster louvain --cluster-resolution 2.0
```

Useful export options:

- `--start <title>` chooses the article to expand from.
- `--end <title>` finds and exports a path from the start article.
- `--nodes <count>` caps the exported subset size.
- `--depth <count>` controls outgoing-neighbor expansion depth.
- `--neighbors <count>` caps outgoing neighbors expanded per node.
- `--cluster-resolution <value>` controls Louvain granularity.

You can also open a specific graph directly:

```text
http://localhost:8000/?data=graph-small.json
http://localhost:8000/?data=graph-louvain-r2p0.json
```

## Use The Visualizer

- Choose a graph preset or enter a JSON path, then press `Load`.
- Pick start and end pages in the sidebar, use the suggestion chips, or choose
  `Pick From Graph` in the Path tab.
- Press `Generate` to export a custom Louvain graph, or `Path` to route inside
  the currently loaded graph.
- Press `Animate` to trace the current path.
- Drag to rotate, use `Pan` to shift the view, and scroll to zoom.
- Click a node to highlight it without changing your point of view.

## How It Works

The C++ exporter builds a subset around the chosen start/path, runs a community
detection algorithm, and writes nodes, links, path data, and cluster IDs to JSON.
The browser then places communities around a sphere, pushes high-out-degree
pages farther outward, and uses zoom-based level of detail so larger exports stay
explorable.

## Other Commands

```bash
./wikiTraverser data/links.tsv
./wikiTraverser path data/links.tsv "Military_dictatorship" "Michael_Jordan"
make clean
```
