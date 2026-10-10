const canvas = document.getElementById("robo-canvas");

let roboModule = null;
let animationHandle = 0;
let lastAnimationTime = 0;

function PageSize() {
  const cssWidth = Math.max(1, window.innerWidth);
  const cssHeight = Math.max(1, window.innerHeight);
  // Pinch changes visualViewport.scale without resizing the layout. Fold that
  // into the bitmap so the magnified frame still has a pixel per screen pixel.
  const pinch = window.visualViewport && window.visualViewport.scale ? window.visualViewport.scale : 1;
  const pixelRatio = Math.min(8, Math.max(1, (window.devicePixelRatio || 1) * pinch));
  return {
    width: Math.max(1, Math.round(cssWidth * pixelRatio)),
    height: Math.max(1, Math.round(cssHeight * pixelRatio)),
    pixelRatio,
  };
}

let frameRender = 0;

function ScheduleFrameRender() {
  if (frameRender) {
    return;
  }
  frameRender = requestAnimationFrame(() => {
    frameRender = 0;
    RenderPage();
  });
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
  canvas.style.touchAction = "manipulation";
  canvas.style.cursor = "default";

  let touchPage = false;
  let touchX = 0;
  let touchY = 0;
  let longPressTimer = 0;
  let longSelecting = false;
  const pointers = new Map();

  function ClearLongPress() {
    if (longPressTimer) {
      clearTimeout(longPressTimer);
      longPressTimer = 0;
    }
  }

  canvas.addEventListener(
    "wheel",
    (event) => {
      if (!roboModule) {
        return;
      }
      // Ctrl/Cmd + wheel and trackpad pinch belong to the browser so its zoom
      // control stays in the address bar and the gesture stays on the GPU.
      if (event.ctrlKey || event.metaKey) {
        ScheduleFrameRender();
        return;
      }
      const point = CanvasPoint(event);
      const delta = WheelPixels(event);
      event.preventDefault();
      if (!roboModule.Wheel(point.x, point.y, delta.x, delta.y)) {
        if (Math.abs(delta.x) > Math.abs(delta.y)) {
          roboModule.ScrollByX(delta.x);
        } else {
          roboModule.ScrollBy(delta.y);
        }
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
    if (pointers.has(event.pointerId)) {
      pointers.set(event.pointerId, point);
    }
    if (pointers.size >= 2) {
      ClearLongPress();
      longSelecting = false;
      return;
    }
    if (longPressTimer && Math.hypot(event.clientX - touchX, event.clientY - touchY) > 8) {
      ClearLongPress();
    }
    if (longSelecting) {
      roboModule.PointerMove(point.x, point.y);
      return;
    }
    if (touchPage) {
      const rect = canvas.getBoundingClientRect();
      const scaleX = rect.width > 0 ? canvas.width / rect.width : 1;
      const scaleY = rect.height > 0 ? canvas.height / rect.height : 1;
      roboModule.ScrollByX((touchX - event.clientX) * scaleX);
      roboModule.ScrollBy((touchY - event.clientY) * scaleY);
      touchX = event.clientX;
      touchY = event.clientY;
      return;
    }
    roboModule.PointerMove(point.x, point.y);
    const kind = roboModule.HitTestKind(point.x, point.y);
    if (roboModule.SelectableAt(point.x, point.y)) {
      canvas.style.cursor = "text";
    } else {
      canvas.style.cursor = kind === "scrollbar" ? "default" : CursorForHit(kind);
    }
  });

  canvas.addEventListener("pointerdown", (event) => {
    if (!roboModule) {
      return;
    }
    canvas.focus();
    const point = CanvasPoint(event);
    pointers.set(event.pointerId, point);
    ClearLongPress();
    longSelecting = false;
    if (IsTouch(event) && roboModule.SelectableAt(point.x, point.y)) {
      touchPage = false;
      touchX = event.clientX;
      touchY = event.clientY;
      if (canvas.setPointerCapture) {
        canvas.setPointerCapture(event.pointerId);
      }
      const pressX = point.x;
      const pressY = point.y;
      longPressTimer = setTimeout(() => {
        longPressTimer = 0;
        longSelecting = true;
        roboModule.LongPressSelect(pressX, pressY);
      }, 480);
      return;
    }
    if (pointers.size >= 2) {
      touchPage = false;
      return;
    }
    const touch = IsTouch(event);
    touchPage = touch && !roboModule.OwnsScrollGesture(point.x, point.y);
    touchX = event.clientX;
    touchY = event.clientY;
    if (canvas.setPointerCapture) {
      canvas.setPointerCapture(event.pointerId);
    }
    roboModule.PointerDown(point.x, point.y, touch && !touchPage, event.detail || 1);
    canvas.style.cursor = "default";
    StartAnimationLoop();
  });

  canvas.addEventListener("pointerup", (event) => {
    if (!roboModule) {
      return;
    }
    pointers.delete(event.pointerId);
    ClearLongPress();
    longSelecting = false;
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

  canvas.addEventListener("pointercancel", (event) => {
    pointers.delete(event.pointerId);
    ClearLongPress();
    longSelecting = false;
    touchPage = false;
    if (roboModule) {
      roboModule.PointerUp(CanvasPoint(event).x, CanvasPoint(event).y);
    }
  });

  canvas.addEventListener("keydown", (event) => {
    if (!roboModule) {
      return;
    }
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "c") {
      if (roboModule.CopySelection()) {
        event.preventDefault();
        return;
      }
    }
    if ((event.ctrlKey || event.metaKey) && event.key.toLowerCase() === "a") {
      if (roboModule.SelectAllText()) {
        event.preventDefault();
        return;
      }
    }
    if (!roboModule.HasFocusedInput() && roboModule.HasCaret()) {
      const extend = event.shiftKey;
      const command = event.metaKey;
      const word = event.altKey || (event.ctrlKey && !event.metaKey);
      let move = -1;
      if (event.key === "ArrowLeft") {
        move = word ? 6 : command ? 4 : 0;
      } else if (event.key === "ArrowRight") {
        move = word ? 7 : command ? 5 : 1;
      } else if (event.key === "ArrowUp") {
        move = command ? 8 : 2;
      } else if (event.key === "ArrowDown") {
        move = command ? 9 : 3;
      } else if (event.key === "Home") {
        move = event.ctrlKey || event.metaKey ? 8 : 4;
      } else if (event.key === "End") {
        move = event.ctrlKey || event.metaKey ? 9 : 5;
      }
      if (move >= 0) {
        event.preventDefault();
        roboModule.MoveSelection(move, extend);
        return;
      }
    }
    if (!roboModule.HasFocusedInput()) {
      const axis = roboModule.ActiveScrollbarAxis();
      if (!axis) {
        const ratio = PageSize().pixelRatio;
        const page = PageSize().height * 0.9;
        let delta = 0;
        let deltaX = 0;
        if (event.key === "ArrowDown") {
          delta = 64 * ratio;
        } else if (event.key === "ArrowUp") {
          delta = -64 * ratio;
        } else if (event.key === "ArrowRight") {
          deltaX = 64 * ratio;
        } else if (event.key === "ArrowLeft") {
          deltaX = -64 * ratio;
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
        if (deltaX !== 0 && roboModule.ScrollByX(deltaX)) {
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

  function OnViewportChange() {
    ScheduleFrameRender();
  }
  window.addEventListener("resize", OnViewportChange);
  if (window.visualViewport) {
    window.visualViewport.addEventListener("resize", OnViewportChange);
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
