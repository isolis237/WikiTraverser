import * as THREE from "./vendor/three.module.min.js";

let canvas = document.getElementById("graphCanvas");

const els = {
    graphMeta: document.getElementById("graphMeta"),
    status: document.getElementById("statusPill"),
    graphPreset: document.getElementById("graphPresetSelect"),
    dataUrl: document.getElementById("dataUrlInput"),
    loadData: document.getElementById("loadDataBtn"),
    start: document.getElementById("startInput"),
    end: document.getElementById("endInput"),
    generateNodes: document.getElementById("generateNodesInput"),
    generateDepth: document.getElementById("generateDepthInput"),
    generateNeighbors: document.getElementById("generateNeighborsInput"),
    clusterResolution: document.getElementById("clusterResolutionInput"),
    generateGraph: document.getElementById("generateGraphBtn"),
    routeSuggestions: document.getElementById("routeSuggestions"),
    refreshSuggestions: document.getElementById("refreshSuggestionsBtn"),
    nodeOptions: document.getElementById("nodeOptions"),
    findPath: document.getElementById("findPathBtn"),
    animate: document.getElementById("animateBtn"),
    pickPath: document.getElementById("pickPathBtn"),
    clearPathPick: document.getElementById("clearPathPickBtn"),
    pathPickHint: document.getElementById("pathPickHint"),
    zoomIn: document.getElementById("zoomInBtn"),
    zoomOut: document.getElementById("zoomOutBtn"),
    fit: document.getElementById("fitBtn"),
    panMode: document.getElementById("panModeBtn"),
    nodeSearch: document.getElementById("nodeSearch"),
    nodeList: document.getElementById("nodeList"),
    pathSummary: document.getElementById("pathSummary"),
    pathList: document.getElementById("pathList"),
    selectedDetails: document.getElementById("selectedDetails"),
    tabs: Array.from(document.querySelectorAll(".tab")),
    panels: Array.from(document.querySelectorAll(".panel"))
};

const palette = [
    "#15847d", "#d95f43", "#c8911f", "#7660a8", "#4f8f4f", "#3b73a3",
    "#b65b8d", "#8b6f31", "#2f8fbd", "#bc704c", "#667a31", "#8357c5"
];

let renderer = null;
let canvasContext = null;
let renderMode = "webgl";
const queryParams = new URLSearchParams(window.location.search);
const forceCanvasRenderer = queryParams.get("renderer") === "canvas";
const initialDataUrl = queryParams.get("data");
if (initialDataUrl) {
    els.dataUrl.value = initialDataUrl;
    els.graphPreset.value = Array.from(els.graphPreset.options).some((option) => option.value === initialDataUrl)
        ? initialDataUrl
        : "";
}

function activateCanvasFallback(reason) {
    renderMode = "canvas2d";
    renderer = null;
    canvasContext = canvas.getContext("2d");

    if (!canvasContext) {
        const replacement = canvas.cloneNode(false);
        canvas.replaceWith(replacement);
        canvas = replacement;
        canvasContext = canvas.getContext("2d");
    }

    if (!canvasContext) {
        throw new Error("Canvas 2D renderer could not start after WebGL failed.");
    }

    els.graphMeta.textContent = "Canvas renderer";
    setStatus(reason ? `Canvas renderer active: ${reason}` : "Canvas renderer active.");
}

if (forceCanvasRenderer) {
    activateCanvasFallback("forced by URL");
} else {
    try {
        renderer = new THREE.WebGLRenderer({
            canvas,
            antialias: true,
            alpha: true,
            powerPreference: "high-performance"
        });
        renderer.setClearColor(0x000000, 0);
    } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        console.warn("WebGL renderer could not start; falling back to Canvas 2D.", err);
        activateCanvasFallback(message);
    }
}

const scene = new THREE.Scene();
const camera = new THREE.PerspectiveCamera(44, 1, 1, 4000);
const graphGroup = new THREE.Group();
const guideGroup = new THREE.Group();
const labelGroup = new THREE.Group();
const focusGroup = new THREE.Group();
graphGroup.add(guideGroup, focusGroup, labelGroup);
scene.add(graphGroup);

scene.add(new THREE.HemisphereLight(0xffffff, 0xb9c5c2, 2.4));
const keyLight = new THREE.DirectionalLight(0xffffff, 2.1);
keyLight.position.set(280, 420, 520);
scene.add(keyLight);

const raycaster = new THREE.Raycaster();
const pointer = new THREE.Vector2();
const frontVector = new THREE.Vector3(0, 0, 1);
const identityQuaternion = new THREE.Quaternion();
const projectionScratch = new THREE.Vector3();

const state = {
    data: null,
    nodes: [],
    edges: [],
    nodeById: new Map(),
    titleToNode: new Map(),
    adjacency: new Map(),
    selected: null,
    hovered: null,
    pathHovered: null,
    currentPath: [],
    visibleNodes: [],
    visibleNodeSet: new Set(),
    visibleEdges: [],
    lodBucket: -1,
    width: 1,
    height: 1,
    nodeMesh: null,
    edgeMesh: null,
    pathMesh: null,
    selectedEdgeMesh: null,
    selectedHalo: null,
    hoverHalo: null,
    pathHoverHalo: null,
    animationMarker: null,
    maxGraphRadius: 360,
    cameraDistance: 720,
    targetDistance: 720,
    pointerDown: false,
    pointerStart: { x: 0, y: 0 },
    pointerLast: { x: 0, y: 0 },
    movedSinceDown: false,
    panMode: false,
    interactionMode: "rotate",
    routeTarget: "start",
    pathPick: { active: false, phase: "start", startNode: null },
    animation: { running: false, startedAt: 0, segmentMs: 760 }
};

function setStatus(message) {
    els.status.textContent = message;
}

function syncGraphPresetSelection() {
    const current = els.dataUrl.value.trim();
    const hasPreset = Array.from(els.graphPreset.options).some((option) => option.value === current);
    els.graphPreset.value = hasPreset ? current : "";
}

function loadCurrentGraph() {
    const url = els.dataUrl.value.trim() || "graph.json";
    els.dataUrl.value = url;
    syncGraphPresetSelection();
    return loadGraph(url);
}

function numericInputValue(input, fallback, min, max) {
    const parsed = Number(input.value);
    if (!Number.isFinite(parsed)) return fallback;
    return Math.min(max, Math.max(min, parsed));
}

function formatResolution(value) {
    return Number(value).toFixed(2).replace(/\.?0+$/, "");
}

function customGraphFilename(options) {
    const resolution = formatResolution(options.resolution).replace(".", "p");
    return `graph-louvain-n${options.nodes}-r${resolution}.json`;
}

function syncGenerateControlsFromMeta() {
    const meta = state.data?.meta || {};
    if (Number.isFinite(Number(meta.requestedMaxNodes))) {
        els.generateNodes.value = Number(meta.requestedMaxNodes);
    }
    if (Number.isFinite(Number(meta.neighborDepth))) {
        els.generateDepth.value = Number(meta.neighborDepth);
    }
    if (Number.isFinite(Number(meta.maxNeighborsPerNode))) {
        els.generateNeighbors.value = Number(meta.maxNeighborsPerNode);
    }
    if (Number.isFinite(Number(meta.clusterResolution))) {
        els.clusterResolution.value = Number(meta.clusterResolution).toFixed(2);
    }
}

function setRouteTarget(target) {
    state.routeTarget = target === "end" ? "end" : "start";
    els.start.classList.toggle("route-active", state.routeTarget === "start");
    els.end.classList.toggle("route-active", state.routeTarget === "end");
}

function compactTitle(title, maxLength = 24) {
    const value = String(title || "");
    return value.length > maxLength ? `${value.slice(0, maxLength - 1)}...` : value;
}

function randomSuggestionNodes(count = 5) {
    const pool = state.nodes
        .filter((node) => node.title && node.visibleOutDegree !== 0)
        .sort((a, b) => (b.outDegree || 0) - (a.outDegree || 0) || a.title.localeCompare(b.title));
    if (pool.length === 0) return [];

    const topWindow = pool.slice(0, Math.max(count, Math.min(pool.length, Math.ceil(pool.length * 0.55))));
    const chosen = [];
    const used = new Set();

    while (chosen.length < count && chosen.length < topWindow.length) {
        const index = Math.floor(Math.random() * topWindow.length);
        const node = topWindow[index];
        if (used.has(node.id)) continue;
        used.add(node.id);
        chosen.push(node);
    }

    return chosen;
}

function applySuggestion(node) {
    if (!node) return;

    let target = state.routeTarget;
    if (!els.start.value.trim()) target = "start";
    else if (!els.end.value.trim() && target === "start") target = "end";

    if (target === "end") {
        els.end.value = node.title;
        setRouteTarget("start");
    } else {
        els.start.value = node.title;
        setRouteTarget("end");
    }

    selectNode(node);
}

function renderRouteSuggestions() {
    els.routeSuggestions.replaceChildren();
    randomSuggestionNodes(5).forEach((node) => {
        const button = document.createElement("button");
        button.type = "button";
        button.className = "suggestion-chip";
        button.title = node.title;
        button.textContent = compactTitle(node.title);
        button.addEventListener("click", () => applySuggestion(node));
        els.routeSuggestions.appendChild(button);
    });
}

function updatePathPickUi() {
    els.pickPath.classList.toggle("active", state.pathPick.active);
    els.clearPathPick.disabled = !state.pathPick.active;

    if (!state.pathPick.active) {
        els.pathPickHint.textContent = "Use start/end fields or pick a route from the graph.";
        return;
    }

    els.pathPickHint.textContent = state.pathPick.phase === "start"
        ? "Pick a start node from the graph."
        : `Start: ${state.pathPick.startNode?.title || "selected"}. Now pick an end node.`;
}

function setPathPickMode(active) {
    state.pathPick = {
        active,
        phase: "start",
        startNode: null
    };
    canvas.classList.toggle("picking-path", active);
    updatePathPickUi();
    setStatus(active ? "Pick a start node from the graph." : "Path picking canceled.");
}

function showPanel(name) {
    els.tabs.forEach((item) => item.classList.toggle("active", item.dataset.tab === name));
    els.panels.forEach((panel) => panel.classList.toggle("active", panel.id === `${name}Panel`));
}

function applyVisiblePathFromInputs() {
    const path = findVisiblePathByTitle(els.start.value, els.end.value);
    if (path.length === 0) {
        setCurrentPath([]);
        setStatus("No path inside the visible subset.");
        return false;
    }
    setCurrentPath(path);
    if (path.length > 0) selectNode(state.nodeById.get(path[0]));
    setStatus(`Visible path found: ${path.length} pages.`);
    return true;
}

function handlePathPickNode(node) {
    if (!state.pathPick.active || !node) return false;

    if (state.pathPick.phase === "start") {
        state.pathPick.startNode = node;
        state.pathPick.phase = "end";
        els.start.value = node.title;
        setRouteTarget("end");
        selectNode(node);
        updatePathPickUi();
        setStatus(`Start set to ${node.title}. Pick an end node.`);
        return true;
    }

    els.end.value = node.title;
    selectNode(node);
    state.pathPick.active = false;
    canvas.classList.remove("picking-path");
    updatePathPickUi();
    applyVisiblePathFromInputs();
    return true;
}

async function generateLouvainGraph() {
    const options = {
        start: els.start.value.trim(),
        end: els.end.value.trim(),
        nodes: Math.round(numericInputValue(els.generateNodes, 320, 20, 2000)),
        depth: Math.round(numericInputValue(els.generateDepth, 2, 1, 5)),
        neighbors: Math.round(numericInputValue(els.generateNeighbors, 36, 1, 200)),
        resolution: numericInputValue(els.clusterResolution, 1.0, 0.05, 8.0),
        iterations: 32
    };
    options.output = customGraphFilename(options);

    els.generateGraph.disabled = true;
    setStatus(`Generating Louvain graph (${options.nodes} nodes, resolution ${formatResolution(options.resolution)})...`);

    try {
        const response = await fetch("/api/generate", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify(options)
        });

        let payload = null;
        try {
            payload = await response.json();
        } catch (_err) {
            payload = null;
        }

        if (!response.ok || !payload?.ok) {
            const message = payload?.error ||
                "Graph generation requires `python3 visualization/server.py` from the project root.";
            throw new Error(message);
        }

        els.dataUrl.value = payload.file;
        syncGraphPresetSelection();
        await loadGraph(payload.file);

        const communities = payload.meta?.detectedCommunities;
        const clusterText = Number.isFinite(Number(communities)) ? ` | ${communities} clusters` : "";
        setStatus(`Generated ${payload.file}${clusterText}.`);
    } finally {
        els.generateGraph.disabled = false;
    }
}

window.addEventListener("error", (event) => {
    const message = event.error?.message || event.message || "Unknown runtime error";
    els.graphMeta.textContent = "Visualization error";
    setStatus(`Visualization error: ${message}`);
});

window.addEventListener("unhandledrejection", (event) => {
    const message = event.reason?.message || String(event.reason || "Unknown promise rejection");
    els.graphMeta.textContent = "Visualization error";
    setStatus(`Visualization error: ${message}`);
});

canvas.addEventListener("webglcontextlost", (event) => {
    if (!renderer) return;
    event.preventDefault();
    els.graphMeta.textContent = "WebGL context lost";
    setStatus("WebGL context was lost. Refresh the page, or try a smaller graph export.");
});

canvas.addEventListener("webglcontextrestored", () => {
    if (!renderer) return;
    setStatus("WebGL context restored. Reloading graph...");
    loadGraph(els.dataUrl.value.trim() || "graph.json").catch((err) => {
        setStatus(err.message);
        console.error(err);
    });
});

function normalizeTitle(title) {
    let value = String(title || "");
    try {
        value = decodeURIComponent(value);
    } catch (_err) {
        // Keep malformed URI text searchable as-is.
    }
    return value.replace(/_/g, " ").replace(/\s+/g, " ").trim().toLocaleLowerCase();
}

function seededUnit(seed) {
    const x = Math.sin((seed + 1) * 9301.17) * 43758.5453;
    return x - Math.floor(x);
}

function clusterColor(cluster) {
    const index = Math.abs(Number(cluster) || 0) % palette.length;
    return palette[index];
}

function displayCluster(node) {
    return Number.isFinite(node?.displayCluster) ? node.displayCluster : node?.cluster || 0;
}

function hasExportedClusters() {
    return state.nodes.some((node) => node.hasExportedCluster);
}

function colorFromHex(hex) {
    return new THREE.Color(hex);
}

function isCanvasMode() {
    return renderMode === "canvas2d";
}

function rendererName() {
    return isCanvasMode() ? "Canvas" : "WebGL";
}

function clamp(value, min, max) {
    return Math.min(max, Math.max(min, value));
}

function hexToRgb(hex) {
    const value = hex.replace("#", "");
    const numeric = Number.parseInt(value.length === 3
        ? value.split("").map((part) => part + part).join("")
        : value, 16);
    return {
        r: (numeric >> 16) & 255,
        g: (numeric >> 8) & 255,
        b: numeric & 255
    };
}

function rgba(hex, alpha) {
    const { r, g, b } = hexToRgb(hex);
    return `rgba(${r}, ${g}, ${b}, ${alpha})`;
}

function disposeObject(object) {
    if (!object) return;
    object.traverse((child) => {
        if (child.geometry) child.geometry.dispose();
        if (child.material) {
            if (Array.isArray(child.material)) {
                child.material.forEach((material) => material.dispose());
            } else {
                if (child.material.map) child.material.map.dispose();
                child.material.dispose();
            }
        }
    });
    object.parent?.remove(object);
}

function clearGroup(group) {
    while (group.children.length > 0) {
        disposeObject(group.children[0]);
    }
}

function resizeRenderer() {
    const rect = canvas.getBoundingClientRect();
    state.width = Math.max(1, rect.width);
    state.height = Math.max(1, rect.height);
    camera.aspect = state.width / state.height;
    camera.updateProjectionMatrix();

    if (renderer) {
        renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
        renderer.setSize(state.width, state.height, false);
        return;
    }

    if (canvasContext) {
        const dpr = Math.min(window.devicePixelRatio || 1, 2);
        canvas.width = Math.max(1, Math.floor(state.width * dpr));
        canvas.height = Math.max(1, Math.floor(state.height * dpr));
        canvasContext.setTransform(dpr, 0, 0, dpr, 0, 0);
    }
}

function fibonacciDirection(index, count) {
    if (count <= 1) return new THREE.Vector3(0, 0, 1);
    const goldenAngle = Math.PI * (3 - Math.sqrt(5));
    const y = 1 - (index / (count - 1)) * 2;
    const radius = Math.sqrt(Math.max(0, 1 - y * y));
    const theta = index * goldenAngle;
    return new THREE.Vector3(Math.cos(theta) * radius, y, Math.sin(theta) * radius).normalize();
}

function tangentBasis(direction) {
    const helper = Math.abs(direction.y) < 0.9
        ? new THREE.Vector3(0, 1, 0)
        : new THREE.Vector3(1, 0, 0);
    const tangentA = new THREE.Vector3().crossVectors(direction, helper).normalize();
    const tangentB = new THREE.Vector3().crossVectors(direction, tangentA).normalize();
    return { tangentA, tangentB };
}

function nodeScaleFromDegree(node, maxLogDegree) {
    const degreeNorm = Math.log1p(node.outDegree || 0) / maxLogDegree;
    return 1.55 + Math.pow(degreeNorm, 0.72) * 3.75;
}

function assignDisplayClusters() {
    if (state.nodes.length === 0) return;

    if (hasExportedClusters()) {
        state.nodes.forEach((node) => {
            node.displayCluster = Number.isFinite(node.cluster) && node.cluster >= 0 ? node.cluster : 0;
        });
        return;
    }

    const undirected = new Map(state.nodes.map((node) => [node.id, new Set()]));
    state.edges.forEach((edge) => {
        undirected.get(edge.source.id)?.add(edge.target.id);
        undirected.get(edge.target.id)?.add(edge.source.id);
    });

    const targetClusterCount = THREE.MathUtils.clamp(
        Math.round(Math.sqrt(state.nodes.length) * 0.9),
        6,
        18
    );
    const candidates = state.nodes
        .slice()
        .sort((a, b) => {
            const scoreA = (a.visibleOutDegree || 0) * 2 + Math.log1p(a.outDegree || 0);
            const scoreB = (b.visibleOutDegree || 0) * 2 + Math.log1p(b.outDegree || 0);
            return scoreB - scoreA || a.title.localeCompare(b.title);
        });

    const seeds = [];
    for (const candidate of candidates) {
        if (seeds.length >= targetClusterCount) break;
        const neighbors = undirected.get(candidate.id) || new Set();
        const tooClose = seeds.some((seed) => seed.id === candidate.id || neighbors.has(seed.id));
        if (!tooClose || seeds.length < Math.ceil(targetClusterCount * 0.45)) {
            seeds.push(candidate);
        }
    }
    for (const candidate of candidates) {
        if (seeds.length >= targetClusterCount) break;
        if (!seeds.some((seed) => seed.id === candidate.id)) seeds.push(candidate);
    }

    const queue = [];
    const assigned = new Set();
    seeds.forEach((seed, index) => {
        seed.displayCluster = index;
        assigned.add(seed.id);
        queue.push(seed);
    });

    for (let cursor = 0; cursor < queue.length; cursor += 1) {
        const current = queue[cursor];
        const neighbors = Array.from(undirected.get(current.id) || [])
            .map((id) => state.nodeById.get(id))
            .filter(Boolean)
            .sort((a, b) => (b.visibleOutDegree || 0) - (a.visibleOutDegree || 0));
        neighbors.forEach((neighbor) => {
            if (assigned.has(neighbor.id)) return;
            neighbor.displayCluster = current.displayCluster;
            assigned.add(neighbor.id);
            queue.push(neighbor);
        });
    }

    state.nodes.forEach((node) => {
        if (!assigned.has(node.id)) {
            const fallback = seeds.length === 0
                ? 0
                : Math.abs(node.id + node.graphId) % seeds.length;
            node.displayCluster = fallback;
        }
    });
}

function computeSphericalLayout() {
    if (state.nodes.length === 0) return;

    assignDisplayClusters();

    const maxLogDegree = Math.max(
        1,
        ...state.nodes.map((node) => Math.log1p(node.outDegree || 0))
    );
    const clusters = Array.from(new Set(state.nodes.map((node) => displayCluster(node)))).sort((a, b) => a - b);
    const clusterAnchors = new Map();
    clusters.forEach((cluster, index) => {
        clusterAnchors.set(cluster, fibonacciDirection(index, clusters.length));
    });

    const nodesByCluster = new Map();
    state.nodes.forEach((node) => {
        const cluster = displayCluster(node);
        if (!nodesByCluster.has(cluster)) nodesByCluster.set(cluster, []);
        nodesByCluster.get(cluster).push(node);
    });

    const baseRadius = Math.max(42, Math.min(88, 38 + state.nodes.length * 0.11));
    const radialRange = Math.max(285, Math.min(470, 260 + state.nodes.length * 0.42));
    state.maxGraphRadius = baseRadius + radialRange + 45;

    nodesByCluster.forEach((clusterNodes, cluster) => {
        clusterNodes.sort((a, b) => (b.outDegree || 0) - (a.outDegree || 0) || a.title.localeCompare(b.title));
        const anchor = clusterAnchors.get(cluster) || frontVector;
        const { tangentA, tangentB } = tangentBasis(anchor);
        const spread = Math.min(0.92, 0.28 + Math.sqrt(clusterNodes.length) * 0.034);

        clusterNodes.forEach((node, index) => {
            const degreeNorm = Math.log1p(node.outDegree || 0) / maxLogDegree;
            const ring = Math.sqrt((index + 0.5) / Math.max(1, clusterNodes.length));
            const theta = (index + 1) * Math.PI * (3 - Math.sqrt(5));
            const seed = seededUnit(node.graphId + index * 17) - 0.5;
            const elevationJitter = (seededUnit(node.id * 29 + node.graphId) - 0.5) * 0.11;
            const direction = anchor.clone()
                .addScaledVector(tangentA, Math.cos(theta) * ring * spread)
                .addScaledVector(tangentB, Math.sin(theta) * ring * spread)
                .addScaledVector(anchor, seed * 0.025 + elevationJitter)
                .normalize();

            const radialDistance = baseRadius + Math.pow(degreeNorm, 1.08) * radialRange;
            node.position = direction.multiplyScalar(radialDistance);
            node.direction = node.position.clone().normalize();
            node.visualScale = nodeScaleFromDegree(node, maxLogDegree);
            node.lodScore = degreeNorm * 0.82 + Math.min(1, (node.visibleOutDegree || 0) / 42) * 0.18;
        });
    });
}

function buildGuides() {
    clearGroup(guideGroup);

    const radii = [
        state.maxGraphRadius * 0.35,
        state.maxGraphRadius * 0.68,
        state.maxGraphRadius
    ];
    radii.forEach((radius, index) => {
        const geometry = new THREE.WireframeGeometry(new THREE.SphereGeometry(radius, 48, 24));
        const material = new THREE.LineBasicMaterial({
            color: index === radii.length - 1 ? 0x8ba5a3 : 0xb9c9c6,
            transparent: true,
            opacity: index === radii.length - 1 ? 0.075 : 0.04,
            depthWrite: false
        });
        guideGroup.add(new THREE.LineSegments(geometry, material));
    });
}

function lodBucketForDistance(distance = state.targetDistance) {
    const ratio = distance / Math.max(1, state.maxGraphRadius);
    if (ratio >= 2.58) return 0;
    if (ratio >= 2.02) return 1;
    if (ratio >= 1.46) return 2;
    if (ratio >= 0.98) return 3;
    return 4;
}

function forcedVisibleNodeIds() {
    const forced = new Set(state.currentPath);
    [state.selected, state.hovered, state.pathHovered].forEach((node) => {
        if (node) forced.add(node.id);
    });
    return forced;
}

function edgeImportance(edge) {
    let score = Math.max(edge.source.lodScore || 0, edge.target.lodScore || 0);
    score += Math.min(0.18, Math.log1p(edge.source.visibleOutDegree || 0) / 36);
    if (displayCluster(edge.source) !== displayCluster(edge.target)) score += 0.12;
    if (edge.pathStep >= 0 || edge.inPath) score += 8;
    if (state.selected && edge.source.id === state.selected.id) score += 3;
    return score;
}

function recomputeVisibleGraph(force = false) {
    if (state.nodes.length === 0) return false;
    const bucket = lodBucketForDistance();
    const forced = forcedVisibleNodeIds();
    if (!force && bucket === state.lodBucket) return false;

    const nodeFractions = [0.34, 0.52, 0.70, 0.88, 1];
    const edgeFractions = [0.075, 0.18, 0.36, 0.68, 1];
    const selectedNeighborCaps = [14, 22, 34, 54, 96];

    const sortedNodes = state.nodes
        .slice()
        .sort((a, b) => {
            const aForced = forced.has(a.id) ? 1 : 0;
            const bForced = forced.has(b.id) ? 1 : 0;
            if (aForced !== bForced) return bForced - aForced;
            return (b.lodScore || 0) - (a.lodScore || 0) ||
                (b.outDegree || 0) - (a.outDegree || 0) ||
                a.title.localeCompare(b.title);
        });

    const nodeCap = Math.max(
        forced.size,
        Math.ceil(state.nodes.length * nodeFractions[bucket])
    );
    const visibleIds = new Set(forced);
    for (const node of sortedNodes) {
        if (visibleIds.size >= nodeCap) break;
        visibleIds.add(node.id);
    }

    if (state.selected) {
        const outgoing = state.edges
            .filter((edge) => edge.source.id === state.selected.id)
            .sort((a, b) => edgeImportance(b) - edgeImportance(a));
        outgoing.slice(0, selectedNeighborCaps[bucket]).forEach((edge) => {
            visibleIds.add(edge.target.id);
        });
    }

    state.visibleNodeSet = visibleIds;
    state.visibleNodes = state.nodes.filter((node) => visibleIds.has(node.id));

    const pathEdgeKeys = new Set();
    for (let i = 1; i < state.currentPath.length; i += 1) {
        pathEdgeKeys.add(`${state.currentPath[i - 1]}:${state.currentPath[i]}`);
    }

    const candidateEdges = state.edges.filter((edge) =>
        visibleIds.has(edge.source.id) && visibleIds.has(edge.target.id)
    );
    const edgeCap = Math.max(
        pathEdgeKeys.size,
        Math.ceil(state.edges.length * edgeFractions[bucket])
    );
    const visibleEdges = [];
    const includedEdgeKeys = new Set();

    candidateEdges
        .filter((edge) => pathEdgeKeys.has(`${edge.source.id}:${edge.target.id}`))
        .forEach((edge) => {
            const key = `${edge.source.id}:${edge.target.id}`;
            includedEdgeKeys.add(key);
            visibleEdges.push(edge);
        });

    candidateEdges
        .filter((edge) => !includedEdgeKeys.has(`${edge.source.id}:${edge.target.id}`))
        .sort((a, b) => edgeImportance(b) - edgeImportance(a))
        .forEach((edge) => {
            if (visibleEdges.length >= edgeCap) return;
            visibleEdges.push(edge);
        });

    state.visibleEdges = visibleEdges;
    state.lodBucket = bucket;

    buildNodes();
    buildEdges();
    buildPathMesh();
    buildSelectedEdges();

    const pathText = state.currentPath.length > 1 ? ` | path ${state.currentPath.length} nodes` : "";
    setStatus(`${state.visibleNodes.length}/${state.nodes.length} nodes | ${state.visibleEdges.length}/${state.edges.length} links${pathText}`);
    return true;
}

function buildNodes() {
    disposeObject(state.nodeMesh);
    const geometry = new THREE.SphereGeometry(1, 18, 12);
    const meshGroup = new THREE.Group();
    const matrix = new THREE.Matrix4();
    const scale = new THREE.Vector3();
    const nodesByCluster = new Map();

    state.visibleNodes.forEach((node) => {
        const cluster = displayCluster(node);
        const index = node.originalIndex;
        if (!nodesByCluster.has(cluster)) nodesByCluster.set(cluster, []);
        nodesByCluster.get(cluster).push({ node, index });
    });

    nodesByCluster.forEach((clusterNodes) => {
        const cluster = displayCluster(clusterNodes[0]?.node);
        const material = new THREE.MeshBasicMaterial({
            color: colorFromHex(clusterColor(cluster))
        });
        const mesh = new THREE.InstancedMesh(geometry, material, clusterNodes.length);
        mesh.userData.type = "nodes";
        mesh.userData.nodeIndices = clusterNodes.map(({ index }) => index);

        clusterNodes.forEach(({ node }, instanceIndex) => {
            scale.setScalar(node.visualScale);
            matrix.compose(node.position, identityQuaternion, scale);
            mesh.setMatrixAt(instanceIndex, matrix);
        });

        mesh.instanceMatrix.needsUpdate = true;
        meshGroup.add(mesh);
    });

    state.nodeMesh = meshGroup;
    graphGroup.add(meshGroup);
}

function makeLineSegments(edges, color, opacity, linewidth = 1) {
    const positions = new Float32Array(edges.length * 6);
    edges.forEach((edge, index) => {
        const offset = index * 6;
        positions[offset] = edge.source.position.x;
        positions[offset + 1] = edge.source.position.y;
        positions[offset + 2] = edge.source.position.z;
        positions[offset + 3] = edge.target.position.x;
        positions[offset + 4] = edge.target.position.y;
        positions[offset + 5] = edge.target.position.z;
    });

    const geometry = new THREE.BufferGeometry();
    geometry.setAttribute("position", new THREE.BufferAttribute(positions, 3));
    const material = new THREE.LineBasicMaterial({
        color,
        transparent: true,
        opacity,
        linewidth,
        depthWrite: false
    });
    return new THREE.LineSegments(geometry, material);
}

function buildEdges() {
    disposeObject(state.edgeMesh);
    state.edgeMesh = makeLineSegments(state.visibleEdges, 0x435558, state.visibleEdges.length > 4500 ? 0.1 : 0.16);
    graphGroup.add(state.edgeMesh);
}

function buildPathMesh() {
    disposeObject(state.pathMesh);
    const pathEdges = [];
    for (let i = 1; i < state.currentPath.length; i += 1) {
        const source = state.nodeById.get(state.currentPath[i - 1]);
        const target = state.nodeById.get(state.currentPath[i]);
        if (source && target) pathEdges.push({ source, target });
    }

    if (pathEdges.length === 0) {
        state.pathMesh = null;
        return;
    }

    state.pathMesh = makeLineSegments(pathEdges, 0xd95f43, 0.95, 2);
    graphGroup.add(state.pathMesh);
}

function buildSelectedEdges() {
    disposeObject(state.selectedEdgeMesh);
    if (!state.selected) {
        state.selectedEdgeMesh = null;
        return;
    }

    const focused = state.edges.filter((edge) =>
        edge.source.id === state.selected.id &&
        state.visibleNodeSet.has(edge.source.id) &&
        state.visibleNodeSet.has(edge.target.id)
    );
    if (focused.length === 0) {
        state.selectedEdgeMesh = null;
        return;
    }

    state.selectedEdgeMesh = makeLineSegments(focused, 0x15847d, 0.9, 2);
    graphGroup.add(state.selectedEdgeMesh);
}

function makeHalo(color, opacity) {
    const geometry = new THREE.SphereGeometry(1, 24, 16);
    const material = new THREE.MeshBasicMaterial({
        color,
        transparent: true,
        opacity,
        wireframe: true,
        depthWrite: false
    });
    const mesh = new THREE.Mesh(geometry, material);
    mesh.visible = false;
    focusGroup.add(mesh);
    return mesh;
}

function ensureFocusMeshes() {
    if (!state.selectedHalo) state.selectedHalo = makeHalo(0x1d2527, 0.72);
    if (!state.hoverHalo) state.hoverHalo = makeHalo(0xc8911f, 0.62);
    if (!state.pathHoverHalo) state.pathHoverHalo = makeHalo(0xd95f43, 0.82);
    if (!state.animationMarker) {
        const geometry = new THREE.SphereGeometry(7.6, 24, 16);
        const material = new THREE.MeshBasicMaterial({ color: 0xffffff });
        state.animationMarker = new THREE.Mesh(geometry, material);
        state.animationMarker.visible = false;
        focusGroup.add(state.animationMarker);
    }
}

function updateHalo(mesh, node, multiplier) {
    if (!mesh || !node) {
        if (mesh) mesh.visible = false;
        return;
    }
    mesh.position.copy(node.position);
    mesh.scale.setScalar(node.visualScale * multiplier);
    mesh.visible = true;
}

function createLabelSprite(text, colorHex) {
    const textValue = text.length > 36 ? `${text.slice(0, 35)}...` : text;
    const labelCanvas = document.createElement("canvas");
    const context = labelCanvas.getContext("2d");
    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    context.font = `${Math.floor(26 * dpr)}px Inter, system-ui, sans-serif`;
    const width = Math.ceil(Math.min(520 * dpr, context.measureText(textValue).width + 34 * dpr));
    const height = Math.ceil(48 * dpr);
    labelCanvas.width = width;
    labelCanvas.height = height;

    context.font = `${Math.floor(26 * dpr)}px Inter, system-ui, sans-serif`;
    context.fillStyle = "rgba(255, 255, 255, 0.92)";
    roundRect(context, 0, 0, width, height, 10 * dpr);
    context.fill();
    context.strokeStyle = colorHex;
    context.lineWidth = 3 * dpr;
    context.stroke();
    context.fillStyle = "#1d2527";
    context.textBaseline = "middle";
    context.fillText(textValue, 16 * dpr, height / 2 + 1 * dpr, width - 28 * dpr);

    const texture = new THREE.CanvasTexture(labelCanvas);
    texture.colorSpace = THREE.SRGBColorSpace;
    const material = new THREE.SpriteMaterial({
        map: texture,
        transparent: true,
        depthTest: false,
        depthWrite: false
    });
    const sprite = new THREE.Sprite(material);
    const scale = 0.12;
    sprite.scale.set(width * scale, height * scale, 1);
    sprite.renderOrder = 4;
    return sprite;
}

function roundRect(context, x, y, width, height, radius) {
    context.beginPath();
    context.moveTo(x + radius, y);
    context.arcTo(x + width, y, x + width, y + height, radius);
    context.arcTo(x + width, y + height, x, y + height, radius);
    context.arcTo(x, y + height, x, y, radius);
    context.arcTo(x, y, x + width, y, radius);
    context.closePath();
}

function rebuildLabels() {
    clearGroup(labelGroup);
    const labeled = new Map();

    state.currentPath.forEach((id) => {
        const node = state.nodeById.get(id);
        if (node) labeled.set(node.id, { node, color: "#d95f43" });
    });

    if (state.selected) labeled.set(state.selected.id, { node: state.selected, color: "#1d2527" });
    if (state.hovered) labeled.set(state.hovered.id, { node: state.hovered, color: "#c8911f" });
    if (state.pathHovered) labeled.set(state.pathHovered.id, { node: state.pathHovered, color: "#d95f43" });

    labeled.forEach(({ node, color }) => {
        const sprite = createLabelSprite(node.title, color);
        const labelOffset = node.position.clone().normalize().multiplyScalar(node.visualScale + 24);
        sprite.position.copy(node.position).add(labelOffset);
        labelGroup.add(sprite);
    });
}

function updateFocusVisuals() {
    ensureFocusMeshes();
    updateHalo(state.selectedHalo, state.selected, 2.1);
    updateHalo(state.hoverHalo, state.hovered, 1.75);
    updateHalo(state.pathHoverHalo, state.pathHovered, 2.45);
    buildSelectedEdges();
    rebuildLabels();
}

function rebuildPathEdgeSteps() {
    const stepByKey = new Map();
    for (let i = 1; i < state.currentPath.length; i += 1) {
        stepByKey.set(`${state.currentPath[i - 1]}:${state.currentPath[i]}`, i - 1);
    }

    state.nodes.forEach((node) => {
        node.activePathIndex = state.currentPath.indexOf(node.id);
    });
    state.edges.forEach((edge) => {
        const key = `${edge.source.id}:${edge.target.id}`;
        edge.pathStep = stepByKey.has(key) ? stepByKey.get(key) : -1;
    });
}

function setCurrentPath(pathIds) {
    state.currentPath = Array.isArray(pathIds) ? pathIds.filter((id) => state.nodeById.has(id)) : [];
    rebuildPathEdgeSteps();
    buildPathMesh();
    renderPathList();
    rebuildLabels();

    if (state.currentPath.length > 0) {
        els.start.value = state.nodeById.get(state.currentPath[0]).title;
        els.end.value = state.nodeById.get(state.currentPath[state.currentPath.length - 1]).title;
    }
}

function initGraph(data) {
    state.data = data;
    state.nodes = (data.nodes || []).map((node, index) => ({
        ...node,
        id: Number(node.id),
        graphId: Number(node.graphId),
        originalIndex: index,
        hasExportedCluster: Number.isFinite(Number(node.cluster)),
        cluster: Number.isFinite(Number(node.cluster)) ? Number(node.cluster) : 0,
        displayCluster: Number.isFinite(Number(node.cluster)) ? Number(node.cluster) : 0,
        pathIndex: Number.isFinite(Number(node.pathIndex)) ? Number(node.pathIndex) : -1,
        activePathIndex: -1,
        position: new THREE.Vector3(),
        direction: new THREE.Vector3(),
        visualScale: 4,
        lodScore: 0
    }));

    state.nodeById = new Map(state.nodes.map((node) => [node.id, node]));
    state.titleToNode = new Map();
    state.nodes.forEach((node) => {
        state.titleToNode.set(normalizeTitle(node.title), node);
    });

    state.edges = (data.edges || [])
        .map((edge) => ({
            source: state.nodeById.get(Number(edge.source)),
            target: state.nodeById.get(Number(edge.target)),
            inPath: Boolean(edge.inPath),
            pathStep: -1
        }))
        .filter((edge) => edge.source && edge.target);

    state.adjacency = new Map(state.nodes.map((node) => [node.id, []]));
    state.edges.forEach((edge) => {
        state.adjacency.get(edge.source.id).push(edge.target.id);
    });

    computeSphericalLayout();
    disposeObject(state.nodeMesh);
    disposeObject(state.edgeMesh);
    disposeObject(state.pathMesh);
    disposeObject(state.selectedEdgeMesh);
    clearGroup(guideGroup);
    clearGroup(focusGroup);
    clearGroup(labelGroup);
    state.nodeMesh = null;
    state.edgeMesh = null;
    state.pathMesh = null;
    state.selectedEdgeMesh = null;
    state.visibleNodes = [];
    state.visibleNodeSet = new Set();
    state.visibleEdges = [];
    state.lodBucket = -1;
    state.selectedHalo = null;
    state.hoverHalo = null;
    state.pathHoverHalo = null;
    state.animationMarker = null;
    if (!graphGroup.children.includes(guideGroup)) graphGroup.add(guideGroup);
    if (!graphGroup.children.includes(focusGroup)) graphGroup.add(focusGroup);
    if (!graphGroup.children.includes(labelGroup)) graphGroup.add(labelGroup);
    buildGuides();
    ensureFocusMeshes();

    const exportedPath = Array.isArray(data.path) ? data.path.map(Number) : [];
    state.selected = null;
    state.hovered = null;
    state.pathHovered = null;
    state.pathPick = { active: false, phase: "start", startNode: null };
    canvas.classList.remove("picking-path");
    setCurrentPath(exportedPath);
    updateFocusVisuals();

    renderMeta();
    syncGenerateControlsFromMeta();
    renderNodeOptions();
    renderRouteSuggestions();
    renderNodeList();
    renderDetails();
    updatePathPickUi();
    fitToGraph();
    recomputeVisibleGraph(true);

    const pathText = state.currentPath.length > 1 ? ` | path ${state.currentPath.length} nodes` : "";
    setStatus(`${state.visibleNodes.length}/${state.nodes.length} nodes | ${state.visibleEdges.length}/${state.edges.length} links${pathText}`);
}

async function loadGraph(url) {
    setStatus("Loading graph...");
    const response = await fetch(url, { cache: "no-store" });
    if (!response.ok) {
        throw new Error(`Could not load ${url} (${response.status})`);
    }
    const data = await response.json();
    initGraph(data);
}

function renderMeta() {
    const meta = state.data?.meta || {};
    const rendererText = isCanvasMode() ? " | Canvas fallback" : "";
    const resolutionText = Number.isFinite(Number(meta.clusterResolution))
        ? ` r${formatResolution(meta.clusterResolution)}`
        : "";
    const clusterText = meta.clusterAlgorithm
        ? ` | ${meta.detectedCommunities || 0} ${meta.clusterAlgorithm}${resolutionText} clusters`
        : "";
    els.graphMeta.textContent =
        `${meta.includedNodes || state.nodes.length} nodes | ${meta.includedEdges || state.edges.length} links | ` +
        `${meta.totalNodes || 0} total pages${clusterText}${rendererText}`;
}

function renderNodeOptions() {
    els.nodeOptions.replaceChildren();
    state.nodes
        .slice()
        .sort((a, b) => a.title.localeCompare(b.title))
        .forEach((node) => {
            const option = document.createElement("option");
            option.value = node.title;
            els.nodeOptions.appendChild(option);
        });
}

function renderNodeList() {
    const query = normalizeTitle(els.nodeSearch.value);
    const rows = state.nodes
        .filter((node) => !query || normalizeTitle(node.title).includes(query))
        .sort((a, b) => (b.outDegree || 0) - (a.outDegree || 0) || a.title.localeCompare(b.title));

    els.nodeList.replaceChildren();
    rows.forEach((node) => {
        const button = document.createElement("button");
        button.type = "button";
        button.className = `node-row${state.selected?.id === node.id ? " selected" : ""}`;
        button.addEventListener("click", () => selectNode(node));

        const text = document.createElement("div");
        const title = document.createElement("div");
        title.className = "node-title";
        title.textContent = node.title;
        const subtitle = document.createElement("div");
        subtitle.className = "node-subtitle";
        subtitle.textContent = `${node.visibleOutDegree || 0} visible | ${node.externalOutDegree || 0} outside`;
        text.append(title, subtitle);

        const metric = document.createElement("div");
        metric.className = "node-metric";
        metric.style.background = clusterColor(displayCluster(node));
        metric.textContent = node.outDegree || 0;

        button.append(text, metric);
        els.nodeList.appendChild(button);
    });
}

function renderPathList() {
    els.pathList.replaceChildren();

    if (state.currentPath.length === 0) {
        els.pathSummary.textContent = "No visible path selected.";
        return;
    }

    els.pathSummary.textContent =
        `${state.currentPath.length} pages from ${state.nodeById.get(state.currentPath[0]).title} to ` +
        `${state.nodeById.get(state.currentPath[state.currentPath.length - 1]).title}`;

    state.currentPath.forEach((id, index) => {
        const node = state.nodeById.get(id);
        if (!node) return;

        const row = document.createElement("button");
        row.type = "button";
        row.className = "path-row";
        row.addEventListener("click", () => selectNode(node));
        row.addEventListener("mouseenter", () => setPathHoveredNode(node));
        row.addEventListener("mouseleave", () => setPathHoveredNode(null));
        row.addEventListener("focus", () => setPathHoveredNode(node));
        row.addEventListener("blur", () => setPathHoveredNode(null));

        const step = document.createElement("span");
        step.className = "path-step";
        step.textContent = String(index + 1);

        const text = document.createElement("div");
        const title = document.createElement("div");
        title.className = "path-title";
        title.textContent = node.title;
        const subtitle = document.createElement("div");
        subtitle.className = "path-subtitle";
        subtitle.textContent = `${node.visibleOutDegree || 0} visible outgoing`;
        text.append(title, subtitle);

        row.append(step, text);
        els.pathList.appendChild(row);
    });
}

function setPathHoveredNode(node) {
    if (state.pathHovered?.id === node?.id) return;
    state.pathHovered = node;
    recomputeVisibleGraph(true);
    updateFocusVisuals();
}

function renderDetails() {
    els.selectedDetails.replaceChildren();
    const node = state.selected;
    if (!node) {
        els.selectedDetails.className = "details-empty";
        els.selectedDetails.textContent = "Select a node.";
        return;
    }

    els.selectedDetails.className = "details-card";
    const title = document.createElement("h2");
    title.className = "details-title";
    title.textContent = node.title;

    const stats = document.createElement("div");
    stats.className = "stats-grid";
    [
        ["Out", node.outDegree || 0],
        ["Visible", node.visibleOutDegree || 0],
        ["Outside", node.externalOutDegree || 0]
    ].forEach(([label, value]) => {
        const stat = document.createElement("div");
        stat.className = "stat";
        const statValue = document.createElement("span");
        statValue.className = "stat-value";
        statValue.textContent = value;
        const statLabel = document.createElement("span");
        statLabel.className = "stat-label";
        statLabel.textContent = label;
        stat.append(statValue, statLabel);
        stats.appendChild(stat);
    });

    const sectionLabel = document.createElement("p");
    sectionLabel.className = "section-label";
    sectionLabel.textContent = "Outgoing";

    const outgoing = document.createElement("div");
    outgoing.className = "outgoing-list";
    const targets = (state.adjacency.get(node.id) || [])
        .map((id) => state.nodeById.get(id))
        .filter(Boolean)
        .sort((a, b) => a.title.localeCompare(b.title));

    if (targets.length === 0) {
        const empty = document.createElement("div");
        empty.className = "edge-subtitle";
        empty.textContent = "No visible outgoing edges.";
        outgoing.appendChild(empty);
    } else {
        targets.forEach((target) => {
            const row = document.createElement("button");
            row.type = "button";
            row.className = "edge-row";
            row.addEventListener("click", () => selectNode(target));
            const text = document.createElement("div");
            const edgeTitle = document.createElement("div");
            edgeTitle.className = "edge-title";
            edgeTitle.textContent = target.title;
            const edgeSub = document.createElement("div");
            edgeSub.className = "edge-subtitle";
            edgeSub.textContent = `${target.outDegree || 0} outgoing`;
            text.append(edgeTitle, edgeSub);
            row.appendChild(text);
            outgoing.appendChild(row);
        });
    }

    els.selectedDetails.append(title, stats, sectionLabel, outgoing);
}

function selectNode(node) {
    state.selected = node;
    renderNodeList();
    renderDetails();
    recomputeVisibleGraph(true);
    updateFocusVisuals();
}

function fitToGraph() {
    graphGroup.position.set(0, -state.maxGraphRadius * 0.08, 0);
    graphGroup.quaternion.identity();
    state.targetDistance = Math.max(state.maxGraphRadius * 2.78, 760);
}

function zoom(factor) {
    state.targetDistance = THREE.MathUtils.clamp(
        state.targetDistance * factor,
        Math.max(125, state.maxGraphRadius * 0.44),
        Math.max(920, state.maxGraphRadius * 4.2)
    );
}

function updatePointerFromEvent(event) {
    const rect = canvas.getBoundingClientRect();
    pointer.x = ((event.clientX - rect.left) / rect.width) * 2 - 1;
    pointer.y = -((event.clientY - rect.top) / rect.height) * 2 + 1;
}

function projectWorldPoint(worldPosition) {
    const depth = state.cameraDistance - worldPosition.z;
    if (!Number.isFinite(depth) || depth <= 1) {
        return { x: 0, y: 0, scale: 0, depth, visible: false };
    }

    const focalLength = (state.height * 0.5) / Math.tan(THREE.MathUtils.degToRad(camera.fov) * 0.5);
    const scale = focalLength / depth;
    const x = state.width / 2 + worldPosition.x * scale;
    const y = state.height / 2 - worldPosition.y * scale;
    const margin = 140;

    return {
        x,
        y,
        scale,
        depth,
        visible: x > -margin && x < state.width + margin && y > -margin && y < state.height + margin
    };
}

function projectLocalPoint(localPosition) {
    projectionScratch.copy(localPosition)
        .applyQuaternion(graphGroup.quaternion)
        .add(graphGroup.position);
    return projectWorldPoint(projectionScratch);
}

function canvasNodeRadius(node, projection) {
    return clamp((node.visualScale || 4) * projection.scale, 2.2, 13.5);
}

function findCanvasNodeAt(event) {
    const rect = canvas.getBoundingClientRect();
    const x = event.clientX - rect.left;
    const y = event.clientY - rect.top;
    let bestNode = null;
    let bestDepth = Number.POSITIVE_INFINITY;

    state.visibleNodes.forEach((node) => {
        const projection = projectLocalPoint(node.position);
        if (!projection.visible) return;

        const radius = Math.max(7, canvasNodeRadius(node, projection) + 4);
        const distance = Math.hypot(projection.x - x, projection.y - y);
        if (distance <= radius && projection.depth < bestDepth) {
            bestNode = node;
            bestDepth = projection.depth;
        }
    });

    return bestNode;
}

function findNodeAt(event) {
    if (isCanvasMode()) return findCanvasNodeAt(event);
    if (!state.nodeMesh) return null;
    updatePointerFromEvent(event);
    raycaster.setFromCamera(pointer, camera);
    const intersections = raycaster.intersectObject(state.nodeMesh, true);
    if (intersections.length === 0) return null;
    const hit = intersections[0];
    const nodeIndices = hit.object.userData.nodeIndices || [];
    return state.nodes[nodeIndices[hit.instanceId]] || null;
}

function rotateGraph(deltaX, deltaY) {
    const yaw = new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0, 1, 0), deltaX * 0.006);
    const pitch = new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(1, 0, 0), deltaY * 0.006);
    graphGroup.quaternion.premultiply(yaw).premultiply(pitch).normalize();
}

function panGraph(deltaX, deltaY) {
    const scale = state.cameraDistance / 1040;
    graphGroup.position.x += deltaX * scale;
    graphGroup.position.y -= deltaY * scale;
}

function setInteractionMode(mode) {
    state.interactionMode = mode === "pan" ? "pan" : "rotate";
    els.panMode.classList.toggle("active", state.interactionMode === "pan");
    canvas.style.cursor = state.interactionMode === "pan" ? "move" : "grab";
}

function findVisiblePathByTitle(startTitle, endTitle) {
    const start = state.titleToNode.get(normalizeTitle(startTitle));
    const end = state.titleToNode.get(normalizeTitle(endTitle));
    if (!start || !end) return [];
    if (start.id === end.id) return [start.id];

    const queue = [start.id];
    const visited = new Set([start.id]);
    const predecessor = new Map();

    for (let index = 0; index < queue.length; index += 1) {
        const current = queue[index];
        const neighbors = state.adjacency.get(current) || [];
        for (const next of neighbors) {
            if (visited.has(next)) continue;
            visited.add(next);
            predecessor.set(next, current);
            if (next === end.id) {
                const path = [end.id];
                let at = end.id;
                while (predecessor.has(at)) {
                    at = predecessor.get(at);
                    path.push(at);
                }
                return path.reverse();
            }
            queue.push(next);
        }
    }

    return [];
}

function startAnimation() {
    if (state.currentPath.length < 2) {
        setStatus("Choose a visible path first.");
        return;
    }
    state.animation.running = true;
    state.animation.startedAt = performance.now();
    if (state.animationMarker) state.animationMarker.visible = true;
    setStatus(`Animating ${state.currentPath.length} pages.`);
}

function updateAnimationMarker(now) {
    if (!state.animationMarker) return;
    if (!state.animation.running || state.currentPath.length < 2) {
        state.animationMarker.visible = false;
        return;
    }

    const elapsed = now - state.animation.startedAt;
    const segmentFloat = elapsed / state.animation.segmentMs;
    const segment = Math.floor(segmentFloat);
    const maxSegment = state.currentPath.length - 2;

    if (segment > maxSegment) {
        state.animation.running = false;
        state.animationMarker.visible = false;
        setStatus(`${state.nodes.length} visible nodes | ${state.edges.length} visible links | path ${state.currentPath.length} nodes`);
        return;
    }

    const from = state.nodeById.get(state.currentPath[segment]);
    const to = state.nodeById.get(state.currentPath[segment + 1]);
    if (!from || !to) return;

    const t = segmentFloat - segment;
    state.animationMarker.position.copy(from.position).lerp(to.position, t);
    state.animationMarker.visible = true;
}

function canvasProjectionMap() {
    const projections = new Map();
    state.visibleNodes.forEach((node) => {
        projections.set(node.id, projectLocalPoint(node.position));
    });
    return projections;
}

function drawCanvasGuides(context) {
    const center = projectWorldPoint(graphGroup.position);
    if (!center.visible || center.scale <= 0) return;

    context.save();
    context.setLineDash([4, 10]);
    [
        { radius: state.maxGraphRadius * 0.35, alpha: 0.12 },
        { radius: state.maxGraphRadius * 0.68, alpha: 0.09 },
        { radius: state.maxGraphRadius, alpha: 0.14 }
    ].forEach(({ radius, alpha }) => {
        const screenRadius = Math.max(2, radius * center.scale);
        context.beginPath();
        context.arc(center.x, center.y, screenRadius, 0, Math.PI * 2);
        context.strokeStyle = rgba("#8ba5a3", alpha);
        context.lineWidth = 1;
        context.stroke();
    });
    context.restore();
}

function drawableEdges(edges, projections) {
    return edges
        .map((edge) => ({
            edge,
            source: projections.get(edge.source.id),
            target: projections.get(edge.target.id)
        }))
        .filter(({ source, target }) => source?.visible && target?.visible)
        .sort((a, b) => ((b.source.depth + b.target.depth) / 2) - ((a.source.depth + a.target.depth) / 2));
}

function drawCanvasEdges(context, edges, projections, color, opacity, lineWidth) {
    const lines = drawableEdges(edges, projections);
    if (lines.length === 0) return;

    context.save();
    context.globalAlpha = opacity;
    context.strokeStyle = color;
    context.lineWidth = lineWidth;
    context.lineCap = "round";
    context.beginPath();
    lines.forEach(({ source, target }) => {
        context.moveTo(source.x, source.y);
        context.lineTo(target.x, target.y);
    });
    context.stroke();
    context.restore();
}

function pathEdges() {
    const edges = [];
    for (let i = 1; i < state.currentPath.length; i += 1) {
        const source = state.nodeById.get(state.currentPath[i - 1]);
        const target = state.nodeById.get(state.currentPath[i]);
        if (source && target) edges.push({ source, target });
    }
    return edges;
}

function selectedOutgoingEdges() {
    if (!state.selected) return [];
    return state.edges.filter((edge) =>
        edge.source.id === state.selected.id &&
        state.visibleNodeSet.has(edge.source.id) &&
        state.visibleNodeSet.has(edge.target.id)
    );
}

function drawCanvasHalo(context, node, projections, color, multiplier, lineWidth, alpha) {
    if (!node) return;
    const projection = projections.get(node.id) || projectLocalPoint(node.position);
    if (!projection.visible) return;

    const radius = canvasNodeRadius(node, projection) * multiplier + 3;
    context.save();
    context.globalAlpha = alpha;
    context.strokeStyle = color;
    context.lineWidth = lineWidth;
    context.beginPath();
    context.arc(projection.x, projection.y, radius, 0, Math.PI * 2);
    context.stroke();
    context.restore();
}

function drawCanvasNode(context, node, projection) {
    const radius = canvasNodeRadius(node, projection);
    const isPathNode = Number.isFinite(node.activePathIndex) && node.activePathIndex >= 0;

    context.save();
    context.beginPath();
    context.arc(projection.x, projection.y, radius, 0, Math.PI * 2);
    context.fillStyle = clusterColor(displayCluster(node));
    context.fill();
    context.lineWidth = isPathNode ? 2.2 : 1;
    context.strokeStyle = isPathNode ? "#d95f43" : "rgba(29, 37, 39, 0.28)";
    context.stroke();
    context.restore();
}

function canvasLabelEntries(projections) {
    const labeled = new Map();

    state.currentPath.forEach((id) => {
        const node = state.nodeById.get(id);
        if (node) labeled.set(node.id, { node, color: "#d95f43" });
    });

    if (state.selected) labeled.set(state.selected.id, { node: state.selected, color: "#1d2527" });
    if (state.hovered) labeled.set(state.hovered.id, { node: state.hovered, color: "#c8911f" });
    if (state.pathHovered) labeled.set(state.pathHovered.id, { node: state.pathHovered, color: "#d95f43" });

    return Array.from(labeled.values())
        .map((entry) => ({ ...entry, projection: projections.get(entry.node.id) || projectLocalPoint(entry.node.position) }))
        .filter(({ projection }) => projection.visible)
        .sort((a, b) => b.projection.depth - a.projection.depth);
}

function drawCanvasLabel(context, node, projection, color) {
    const textValue = node.title.length > 36 ? `${node.title.slice(0, 35)}...` : node.title;
    const radius = canvasNodeRadius(node, projection);
    context.font = "12px Inter, system-ui, sans-serif";
    const labelWidth = clamp(context.measureText(textValue).width + 18, 42, 270);
    const labelHeight = 24;
    let x = projection.x + radius + 8;
    let y = projection.y - labelHeight - 4;

    if (x + labelWidth > state.width - 8) x = projection.x - radius - labelWidth - 8;
    x = clamp(x, 8, Math.max(8, state.width - labelWidth - 8));
    y = clamp(y, 8, Math.max(8, state.height - labelHeight - 8));

    context.save();
    roundRect(context, x, y, labelWidth, labelHeight, 6);
    context.fillStyle = "rgba(255, 255, 255, 0.94)";
    context.fill();
    context.strokeStyle = color;
    context.lineWidth = 1.4;
    context.stroke();
    context.fillStyle = "#1d2527";
    context.textBaseline = "middle";
    context.fillText(textValue, x + 9, y + labelHeight / 2 + 0.5, labelWidth - 18);
    context.restore();
}

function drawCanvasAnimationMarker(context) {
    if (!state.animationMarker?.visible) return;
    const projection = projectLocalPoint(state.animationMarker.position);
    if (!projection.visible) return;

    const radius = clamp(7.6 * projection.scale, 4.5, 12);
    context.save();
    context.beginPath();
    context.arc(projection.x, projection.y, radius + 3, 0, Math.PI * 2);
    context.fillStyle = "rgba(217, 95, 67, 0.18)";
    context.fill();
    context.beginPath();
    context.arc(projection.x, projection.y, radius, 0, Math.PI * 2);
    context.fillStyle = "#ffffff";
    context.fill();
    context.strokeStyle = "#d95f43";
    context.lineWidth = 2;
    context.stroke();
    context.restore();
}

function renderCanvas2d() {
    if (!canvasContext) return;

    canvasContext.clearRect(0, 0, state.width, state.height);
    drawCanvasGuides(canvasContext);

    if (!state.data) return;

    const projections = canvasProjectionMap();
    const baseOpacity = state.visibleEdges.length > 4500 ? 0.08 : 0.14;
    drawCanvasEdges(canvasContext, state.visibleEdges, projections, "#435558", baseOpacity, 1);
    drawCanvasEdges(canvasContext, selectedOutgoingEdges(), projections, "#15847d", 0.72, 1.8);
    drawCanvasEdges(canvasContext, pathEdges(), projections, "#d95f43", 0.92, 2.2);

    drawCanvasHalo(canvasContext, state.selected, projections, "#1d2527", 2.2, 2, 0.75);
    drawCanvasHalo(canvasContext, state.hovered, projections, "#c8911f", 1.85, 2, 0.68);
    drawCanvasHalo(canvasContext, state.pathHovered, projections, "#d95f43", 2.55, 2.4, 0.84);

    state.visibleNodes
        .map((node) => ({ node, projection: projections.get(node.id) }))
        .filter(({ projection }) => projection?.visible)
        .sort((a, b) => b.projection.depth - a.projection.depth)
        .forEach(({ node, projection }) => drawCanvasNode(canvasContext, node, projection));

    drawCanvasAnimationMarker(canvasContext);
    canvasLabelEntries(projections)
        .forEach(({ node, projection, color }) => drawCanvasLabel(canvasContext, node, projection, color));
}

function frame(now) {
    state.cameraDistance += (state.targetDistance - state.cameraDistance) * 0.16;
    recomputeVisibleGraph(false);
    camera.position.set(0, 0, state.cameraDistance);
    camera.lookAt(0, 0, 0);
    camera.updateMatrixWorld();
    updateAnimationMarker(now);
    if (renderer) {
        renderer.render(scene, camera);
    } else {
        renderCanvas2d();
    }
    requestAnimationFrame(frame);
}

canvas.addEventListener("pointerdown", (event) => {
    canvas.setPointerCapture(event.pointerId);
    state.pointerDown = true;
    state.movedSinceDown = false;
    state.panMode = state.interactionMode === "pan" || event.shiftKey || event.button === 1 || event.button === 2;
    state.pointerStart = { x: event.clientX, y: event.clientY };
    state.pointerLast = { x: event.clientX, y: event.clientY };
    canvas.classList.add("dragging");
});

canvas.addEventListener("pointermove", (event) => {
    const dx = event.clientX - state.pointerLast.x;
    const dy = event.clientY - state.pointerLast.y;
    const totalDx = event.clientX - state.pointerStart.x;
    const totalDy = event.clientY - state.pointerStart.y;

    if (state.pointerDown) {
        if (Math.hypot(totalDx, totalDy) > 4) state.movedSinceDown = true;
        if (state.panMode) panGraph(dx, dy);
        else rotateGraph(dx, dy);
        state.pointerLast = { x: event.clientX, y: event.clientY };
        return;
    }

    const hovered = findNodeAt(event);
    if (hovered?.id !== state.hovered?.id) {
        state.hovered = hovered;
        updateHalo(state.hoverHalo, state.hovered, 1.75);
        rebuildLabels();
    }
});

canvas.addEventListener("pointerup", (event) => {
    const node = findNodeAt(event);
    if (!state.movedSinceDown && node) {
        if (!handlePathPickNode(node)) selectNode(node);
    }
    state.pointerDown = false;
    state.panMode = false;
    canvas.classList.remove("dragging");
});

canvas.addEventListener("pointerleave", () => {
    if (!state.pointerDown) {
        state.hovered = null;
        updateHalo(state.hoverHalo, null, 1);
        rebuildLabels();
    }
});

canvas.addEventListener("contextmenu", (event) => {
    event.preventDefault();
});

canvas.addEventListener("wheel", (event) => {
    event.preventDefault();
    if (event.shiftKey) {
        panGraph(-event.deltaX || -event.deltaY, event.deltaY);
        return;
    }
    zoom(Math.exp(event.deltaY * 0.001));
}, { passive: false });

canvas.addEventListener("dblclick", (event) => {
    const node = findNodeAt(event);
    if (node && !handlePathPickNode(node)) selectNode(node);
});

els.findPath.addEventListener("click", () => {
    state.pathPick.active = false;
    canvas.classList.remove("picking-path");
    updatePathPickUi();
    applyVisiblePathFromInputs();
});

els.animate.addEventListener("click", startAnimation);
els.pickPath.addEventListener("click", () => {
    showPanel("path");
    setPathPickMode(true);
});
els.clearPathPick.addEventListener("click", () => {
    setPathPickMode(false);
});
els.fit.addEventListener("click", fitToGraph);
els.panMode.addEventListener("click", () => {
    setInteractionMode(state.interactionMode === "pan" ? "rotate" : "pan");
});
els.zoomIn.addEventListener("click", () => zoom(0.82));
els.zoomOut.addEventListener("click", () => zoom(1.22));
els.nodeSearch.addEventListener("input", renderNodeList);
els.start.addEventListener("focus", () => setRouteTarget("start"));
els.end.addEventListener("focus", () => setRouteTarget("end"));
els.refreshSuggestions.addEventListener("click", renderRouteSuggestions);
els.generateGraph.addEventListener("click", () => {
    generateLouvainGraph().catch((err) => {
        setStatus(err.message);
        console.error(err);
    });
});
els.dataUrl.addEventListener("input", syncGraphPresetSelection);
els.graphPreset.addEventListener("change", () => {
    if (els.graphPreset.value) {
        els.dataUrl.value = els.graphPreset.value;
        loadCurrentGraph().catch((err) => {
            setStatus(err.message);
            console.error(err);
        });
    } else {
        els.dataUrl.focus();
        syncGraphPresetSelection();
    }
});
els.loadData.addEventListener("click", () => {
    loadCurrentGraph().catch((err) => {
        setStatus(err.message);
        console.error(err);
    });
});

els.tabs.forEach((tab) => {
    tab.addEventListener("click", () => {
        showPanel(tab.dataset.tab);
    });
});

window.addEventListener("resize", () => {
    resizeRenderer();
});

resizeRenderer();
setInteractionMode("rotate");
syncGraphPresetSelection();
setRouteTarget("start");
updatePathPickUi();
requestAnimationFrame(frame);
loadCurrentGraph().catch((err) => {
    setStatus(err.message);
    console.error(err);
});
