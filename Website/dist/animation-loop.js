// Suspend callbacks entirely while paused/hidden; resume without a time jump.
export function animationLoop(draw, isReduced) {
  let frame = 0, last = 0, disposed = false;
  const active = () => !disposed && !document.hidden && !isReduced();
  function tick(now) {
    frame = 0;
    if (!active()) return;
    if (!last) last = now;
    if (now - last >= 33) {
      const dt = Math.min((now - last) / 1000, .05);
      last = now;
      draw(dt);
    }
    frame = requestAnimationFrame(tick);
  }
  function sync() {
    cancelAnimationFrame(frame);
    frame = 0;
    last = 0;
    if (active()) frame = requestAnimationFrame(tick);
  }
  document.addEventListener('visibilitychange', sync);
  document.addEventListener('ora-motion-change', sync);
  sync();
  return () => {
    disposed = true;
    cancelAnimationFrame(frame);
    document.removeEventListener('visibilitychange', sync);
    document.removeEventListener('ora-motion-change', sync);
  };
}
