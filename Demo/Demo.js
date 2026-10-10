const canvas = document.getElementById("robo-canvas");

let roboModule = null;
let animationHandle = 0;
let lastAnimationTime = 0;

function PageSize() {
  const view = window.visualViewport;
  const cssWidth = Math.max(1, view ? view.width : window.innerWidth);
  const cssHeight = Math.max(1, view ? view.height : window.innerHeight);
  const zoom = view && view.scale ? view.scale : 1;
  const pixelRatio = Math.min(3, Math.max(1, (window.devicePixelRatio || 1) * zoom));
  return {
    width: Math.max(1, Math.round(cssWidth * pixelRatio)),
    height: Math.max(1, Math.round(cssHeight * pixelRatio)),
    pixelRatio,
  };
}

function RenderPage() {
  if (!roboModule) {
    return;
  }
  const size = PageSize();
  roboModule.Render(size.width, size.height, size.pixelRatio);
}

function StartAnimationLoop() {
  if (!roboModule || animationHandle) {
    return;
  }
  animationHandle = requestAnimationFrame(AnimationTick);
}

function AnimationTick(now) {
  animationHandle = 0;
  if (!roboModule) {
    return;
  }
  const delta = lastAnimationTime ? Math.min(64, now - lastAnimationTime) : 16;
  lastAnimationTime = now;
  if (roboModule.Tick(delta)) {
    StartAnimationLoop();
  } else {
    lastAnimationTime = 0;
  }
}

function CanvasPoint(event) {
  const rect = canvas.getBoundingClientRect();
  if (rect.width <= 0 || rect.height <= 0) {
    return { x: 0, y: 0 };
  }
  return {
    x: (event.clientX - rect.left) * (canvas.width / rect.width),
    y: (event.clientY - rect.top) * (canvas.height / rect.height),
  };
}

function CursorForHit(kind) {
  if (kind === "button") {
    return "pointer";
  }
  if (kind === "input") {
    return "text";
  }
  return "default";
}

function IsTouch(event) {
  return event.pointerType === "touch";
}

function WheelPixels(event) {
  const factor = event.deltaMode === 1 ? 40 : event.deltaMode === 2 ? 400 : 1;
  return { x: event.deltaX * factor, y: event.deltaY * factor };
}

function WireInteraction() {
  canvas.style.touchAction = "none";
  canvas.style.cursor = "default";

  let touchPage = false;
  let touchX = 0;
  let touchY = 0;

  canvas.addEventListener(
    "wheel",
    (event) => {
      if (!roboModule) {
        return;
      }
      const point = CanvasPoint(event);
      const delta = WheelPixels(event);
      event.preventDefault();
      if (!roboModule.Wheel(point.x, point.y, delta.x, delta.y)) {
        roboModule.ScrollBy(delta.y);
      }
      StartAnimationLoop();
    },
    { passive: false },
  );

  canvas.addEventListener("pointermove", (event) => {
    if (!roboModule) {
      return;
    }
    const point = CanvasPoint(event);
    if (touchPage) {
      const rect = canvas.getBoundingClientRect();
      const scale = rect.height > 0 ? canvas.height / rect.height : 1;
      roboModule.ScrollBy((touchY - event.clientY) * scale);
      touchX = event.clientX;
      touchY = event.clientY;
      return;
    }
    roboModule.PointerMove(point.x, point.y);
    const kind = roboModule.HitTestKind(point.x, point.y);
    canvas.style.cursor = kind === "scrollbar" ? "default" : CursorForHit(kind);
  });

  canvas.addEventListener("pointerdown", (event) => {
    if (!roboModule) {
      return;
    }
    canvas.focus();
    const point = CanvasPoint(event);
    const touch = IsTouch(event);
    touchPage = touch && !roboModule.OwnsScrollGesture(point.x, point.y);
    touchX = event.clientX;
    touchY = event.clientY;
    if (canvas.setPointerCapture) {
      canvas.setPointerCapture(event.pointerId);
    }
    roboModule.PointerDown(point.x, point.y, touch && !touchPage);
    canvas.style.cursor = "default";
    StartAnimationLoop();
  });

  canvas.addEventListener("pointerup", (event) => {
    if (!roboModule) {
      return;
    }
    touchPage = false;
    const point = CanvasPoint(event);
    roboModule.PointerUp(point.x, point.y);
    const kind = roboModule.HitTestKind(point.x, point.y);
    canvas.style.cursor = kind === "scrollbar" ? "default" : CursorForHit(kind);
    StartAnimationLoop();
  });

  canvas.addEventListener("pointerleave", () => {
    if (roboModule) {
      roboModule.PointerLeave();
    }
  });

  canvas.addEventListener("keydown", (event) => {
    if (!roboModule) {
      return;
    }
    if (!roboModule.HasFocusedInput()) {
      const axis = roboModule.ActiveScrollbarAxis();
      if (!axis) {
        const ratio = PageSize().pixelRatio;
        const page = PageSize().height * 0.9;
        let delta = 0;
        if (event.key === "ArrowDown") {
          delta = 64 * ratio;
        } else if (event.key === "ArrowUp") {
          delta = -64 * ratio;
        } else if (event.key === "PageDown") {
          delta = page;
        } else if (event.key === "PageUp") {
          delta = -page;
        } else if (event.key === "Home") {
          delta = -1000000;
        } else if (event.key === "End") {
          delta = 1000000;
        }
        if (delta !== 0 && roboModule.ScrollBy(delta)) {
          event.preventDefault();
        }
        return;
      }
      const vertical = axis === 1;
      let handled = false;
      if (vertical && event.key === "ArrowDown") {
        handled = roboModule.KeyScrollLine(1);
      } else if (vertical && event.key === "ArrowUp") {
        handled = roboModule.KeyScrollLine(-1);
      } else if (!vertical && event.key === "ArrowRight") {
        handled = roboModule.KeyScrollLine(1);
      } else if (!vertical && event.key === "ArrowLeft") {
        handled = roboModule.KeyScrollLine(-1);
      } else if (event.key === "PageDown") {
        handled = roboModule.KeyScrollPage(1);
      } else if (event.key === "PageUp") {
        handled = roboModule.KeyScrollPage(-1);
      } else if (event.key === "Home") {
        handled = roboModule.KeyScrollToStart();
      } else if (event.key === "End") {
        handled = roboModule.KeyScrollToEnd();
      }
      if (handled) {
        event.preventDefault();
        StartAnimationLoop();
      }
      return;
    }
    if (event.key === "Backspace") {
      event.preventDefault();
      roboModule.KeyBackspace();
      return;
    }
    if (event.key === "Delete") {
      event.preventDefault();
      roboModule.KeyDelete();
      return;
    }
    if (event.key === "ArrowLeft") {
      event.preventDefault();
      roboModule.KeyLeft();
      return;
    }
    if (event.key === "ArrowRight") {
      event.preventDefault();
      roboModule.KeyRight();
      return;
    }
    if (event.key === "Escape") {
      event.preventDefault();
      roboModule.KeyEscape();
      return;
    }
    if (event.ctrlKey || event.metaKey || event.altKey || event.key.length !== 1) {
      return;
    }
    event.preventDefault();
    roboModule.KeyInsert(event.key);
  });

  window.addEventListener("resize", RenderPage);
  if (window.visualViewport) {
    window.visualViewport.addEventListener("resize", RenderPage);
  }
}

async function Boot() {
  if (typeof CreateRoboModule !== "function") {
    document.body.textContent = "Build missing. Run RunDemo.ps1.";
    return;
  }
  roboModule = await CreateRoboModule({
    locateFile: (path) => `../Build/Web/${path}`,
  });
  WireInteraction();
  RenderPage();
  canvas.focus();
}

Boot();
