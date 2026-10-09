import { EditorState } from "https://esm.sh/@codemirror/state@6.5.2";
import { EditorView, keymap, lineNumbers, highlightActiveLine, highlightActiveLineGutter, drawSelection, dropCursor } from "https://esm.sh/@codemirror/view@6.36.5";
import { defaultKeymap, history, historyKeymap, indentWithTab } from "https://esm.sh/@codemirror/commands@6.8.1";
import { xml } from "https://esm.sh/@codemirror/lang-xml@6.1.0";
import { syntaxHighlighting, HighlightStyle, foldGutter, bracketMatching } from "https://esm.sh/@codemirror/language@6.11.0";
import { tags } from "https://esm.sh/@lezer/highlight@1.2.1";

const DEBOUNCE_MS = 180;

const statusLabel = document.getElementById("statusLabel");
const errorBanner = document.getElementById("errorBanner");
const editorHost = document.getElementById("editorHost");
const editorPane = document.getElementById("editorPane");
const splitter = document.getElementById("splitter");
const previewCanvas = document.getElementById("robo-preview-canvas");
const previewSurface = document.querySelector(".PreviewSurface");

const xmlHighlight = HighlightStyle.define([
  { tag: tags.tagName, color: "#7AA2F7", fontWeight: "600" },
  { tag: tags.attributeName, color: "#E0AF68" },
  { tag: tags.attributeValue, color: "#9ECE6A" },
  { tag: tags.string, color: "#9ECE6A" },
  { tag: tags.comment, color: "#6B7C8F", fontStyle: "italic" },
  { tag: tags.angleBracket, color: "#C0CAF5" },
  { tag: tags.processingInstruction, color: "#BB9AF7" },
  { tag: tags.meta, color: "#BB9AF7" },
  { tag: tags.name, color: "#7DCFFF" },
]);

const editorTheme = EditorView.theme({
  "&": {
    height: "100%",
    backgroundColor: "#0B0F14",
    color: "#E8EEF6",
  },
  ".cm-content": {
    caretColor: "#3DDC97",
    padding: "12px 0",
  },
  ".cm-cursor, .cm-dropCursor": {
    borderLeftColor: "#3DDC97",
  },
  "&.cm-focused .cm-selectionBackground, .cm-selectionBackground, .cm-content ::selection": {
    backgroundColor: "#1F2A3A",
  },
  ".cm-activeLine": {
    backgroundColor: "#121925",
  },
  ".cm-activeLineGutter": {
    backgroundColor: "#121925",
  },
  ".cm-gutters": {
    backgroundColor: "#0B0F14",
    color: "#6B7C8F",
    border: "none",
    borderRight: "1px solid #243041",
  },
  ".cm-lineNumbers .cm-gutterElement": {
    padding: "0 12px 0 8px",
  },
  ".cm-matchingBracket": {
    backgroundColor: "#243041",
    outline: "1px solid #3DDC97",
  },
  ".cm-foldPlaceholder": {
    backgroundColor: "#1A2330",
    border: "none",
    color: "#8FA1B3",
  },
}, { dark: true });

let roboModule = null;
let renderTimer = null;
let editorView = null;

function SetStatus(message) {
  statusLabel.textContent = message;
}

function ShowError(message) {
  if (!message) {
    errorBanner.hidden = true;
    errorBanner.textContent = "";
    return;
  }
  errorBanner.hidden = false;
  errorBanner.textContent = message;
}

function PreviewSize() {
  const styles = window.getComputedStyle(previewSurface);
  const padX = (parseFloat(styles.paddingLeft) || 0) + (parseFloat(styles.paddingRight) || 0);
  const padY = (parseFloat(styles.paddingTop) || 0) + (parseFloat(styles.paddingBottom) || 0);
  const cssWidth = Math.max(1, Math.floor(previewSurface.clientWidth - padX));
  const cssHeight = Math.max(1, Math.floor(previewSurface.clientHeight - padY));
  const pixelRatio = window.devicePixelRatio || 1;
  return {
    width: Math.max(1, Math.floor(cssWidth * pixelRatio)),
    height: Math.max(1, Math.floor(cssHeight * pixelRatio)),
    cssWidth,
    cssHeight,
    pixelRatio,
  };
}

function RenderPreview(xmlSource) {
  if (!roboModule) {
    return;
  }
  try {
    const size = PreviewSize();
    const error = roboModule.RenderXml(xmlSource, size.width, size.height);
    if (error) {
      ShowError(error);
      SetStatus("XML error");
      return;
    }
    ShowError("");
    SetStatus("Live · native C++ raster");
  } catch (exception) {
    ShowError(String(exception));
    SetStatus("Runtime error");
  }
}

function ScheduleRender(xmlSource) {
  if (renderTimer) {
    clearTimeout(renderTimer);
  }
  renderTimer = setTimeout(() => {
    RenderPreview(xmlSource);
  }, DEBOUNCE_MS);
}

function CanvasPoint(event) {
  const rect = previewCanvas.getBoundingClientRect();
  if (rect.width <= 0 || rect.height <= 0) {
    return { x: 0, y: 0 };
  }
  const scaleX = previewCanvas.width / rect.width;
  const scaleY = previewCanvas.height / rect.height;
  return {
    x: (event.clientX - rect.left) * scaleX,
    y: (event.clientY - rect.top) * scaleY,
  };
}

function WireCanvasInteraction() {
  previewCanvas.style.cursor = "default";

  previewCanvas.addEventListener("pointermove", (event) => {
    if (!roboModule) {
      return;
    }
    const point = CanvasPoint(event);
    roboModule.PointerMove(point.x, point.y);
    previewCanvas.style.cursor = roboModule.HitTest(point.x, point.y) ? "pointer" : "default";
  });

  previewCanvas.addEventListener("pointerdown", (event) => {
    if (!roboModule) {
      return;
    }
    previewCanvas.setPointerCapture(event.pointerId);
    const point = CanvasPoint(event);
    roboModule.PointerDown(point.x, point.y);
  });

  previewCanvas.addEventListener("pointerup", (event) => {
    if (!roboModule) {
      return;
    }
    const point = CanvasPoint(event);
    roboModule.PointerUp(point.x, point.y);
    previewCanvas.style.cursor = roboModule.HitTest(point.x, point.y) ? "pointer" : "default";
    try {
      previewCanvas.releasePointerCapture(event.pointerId);
    } catch (_) {
      // ignore
    }
  });

  previewCanvas.addEventListener("pointerleave", () => {
    if (!roboModule) {
      return;
    }
    roboModule.PointerLeave();
    previewCanvas.style.cursor = "default";
  });

  previewCanvas.addEventListener("pointercancel", () => {
    if (!roboModule) {
      return;
    }
    roboModule.PointerLeave();
    previewCanvas.style.cursor = "default";
  });
}

async function LoadDefaultXml() {
  const response = await fetch("../Examples/Home.xml");
  if (!response.ok) {
    throw new Error("Failed to load Examples/Home.xml");
  }
  return await response.text();
}

function CreateEditor(initialXml) {
  editorView = new EditorView({
    parent: editorHost,
    state: EditorState.create({
      doc: initialXml,
      extensions: [
        lineNumbers(),
        highlightActiveLine(),
        highlightActiveLineGutter(),
        drawSelection(),
        dropCursor(),
        history(),
        foldGutter(),
        bracketMatching(),
        xml(),
        syntaxHighlighting(xmlHighlight),
        editorTheme,
        keymap.of([indentWithTab, ...defaultKeymap, ...historyKeymap]),
        EditorView.updateListener.of((update) => {
          if (update.docChanged) {
            ScheduleRender(update.state.doc.toString());
          }
        }),
      ],
    }),
  });
}

function CurrentXml() {
  return editorView ? editorView.state.doc.toString() : "";
}

function WireSplitter() {
  let dragging = false;

  splitter.addEventListener("mousedown", (event) => {
    dragging = true;
    event.preventDefault();
  });

  window.addEventListener("mouseup", () => {
    dragging = false;
  });

  window.addEventListener("mousemove", (event) => {
    if (!dragging) {
      return;
    }
    const workspace = document.querySelector(".Workspace");
    const bounds = workspace.getBoundingClientRect();
    const vertical = window.matchMedia("(max-width: 860px)").matches;
    if (vertical) {
      const topHeight = Math.min(Math.max(event.clientY - bounds.top, 140), bounds.height - 140);
      editorPane.style.height = `${topHeight}px`;
      editorPane.style.width = "100%";
    } else {
      const leftWidth = Math.min(Math.max(event.clientX - bounds.left, 220), bounds.width - 220);
      editorPane.style.width = `${leftWidth}px`;
      editorPane.style.height = "";
    }
    ScheduleRender(CurrentXml());
  });

  if (typeof ResizeObserver === "function") {
    const observer = new ResizeObserver(() => {
      ScheduleRender(CurrentXml());
    });
    observer.observe(previewSurface);
  } else {
    window.addEventListener("resize", () => {
      ScheduleRender(CurrentXml());
    });
  }
}

async function Boot() {
  WireSplitter();
  SetStatus("Loading WASM…");

  if (typeof CreateRoboModule !== "function") {
    SetStatus("Build missing");
    ShowError("Robo WASM is missing. Run Build.ps1 first.");
    return;
  }

  roboModule = await CreateRoboModule({
    locateFile: (path) => `../Build/Web/${path}`,
  });

  const initialXml = await LoadDefaultXml();
  CreateEditor(initialXml);
  WireCanvasInteraction();
  RenderPreview(initialXml);
  SetStatus("Live · native C++ raster");
}

Boot().catch((error) => {
  SetStatus("Failed");
  ShowError(String(error));
});
