<script setup lang="ts">
// Interactive 3D view of the V2 assembly (three.js). The GLB is built from the
// Ket model by docs/scripts/viewer-glb.py; node names are the Ket body names.
import { onBeforeUnmount, onMounted, ref, watch } from "vue";
import { withBase } from "vitepress";

const props = withDefaults(defineProps<{ compact?: boolean }>(), { compact: false });

const host = ref<HTMLDivElement | null>(null);
const explode = ref(0);
const spin = ref(true);
const edges = ref(true);
const loading = ref(true);
const failed = ref("");
const hovered = ref("");

// Explode offsets in Ket millimetres [x, y, z] (Z up, +Y towards the back).
// The eyes stay in the top shell and the USB bezel in the bottom shell.
const OFFSETS: Record<string, [number, number, number]> = {
  back_shell: [0, 0, -14],
  lipo: [0, 0, -2],
  battery_plate: [0, 0, 10],
  usb_bezel: [0, 0, -14],
  board: [0, 0, 22],
  front_shell: [0, 0, 48],
  eye_cap_l: [0, 0, 48],
  eye_cap_r: [0, 0, 48],
  mx1: [0, 0, 70],
  mx2: [0, 0, 70],
  cap1: [0, 0, 88],
  cap2: [0, 0, 88],
};

const LABELS: Record<string, string> = {
  back_shell: "Bottom shell",
  front_shell: "Top shell with L1/R1 paddles",
  battery_plate: "Battery plate (posts under the sockets)",
  lipo: "LiPo battery",
  usb_bezel: "Clear USB-C bezel",
  board: "PCB",
  eye_cap_l: "Eye cover (clear)",
  eye_cap_r: "Eye cover (clear)",
  mx1: "MX switch (left key)",
  mx2: "MX switch (right key)",
  cap1: "Keycap B1 (Escape)",
  cap2: "Keycap B2 (Enter)",
  part_U1: "XIAO nRF52840 Sense",
  part_K1: "Kailh hot-swap socket",
  part_K2: "Kailh hot-swap socket",
  part_SW1: "MODE switch (USB / BLE)",
  part_SW2: "POWER switch",
  part_SW3: "L1 shoulder switch",
  part_SW4: "R1 shoulder switch",
  part_D1: "Eye LED",
  part_D2: "Eye LED",
  part_R1: "LED resistor",
  part_R2: "LED resistor",
  part_J1: "Battery socket J1 (JST PH)",
};

let cleanup: (() => void) | null = null;
let setExplode: ((t: number) => void) | null = null;
let setEdges: ((on: boolean) => void) | null = null;
let controlsRef: { autoRotate: boolean } | null = null;
let anim = 0;

function toggleExplode() {
  const from = explode.value;
  const to = from > 0.5 ? 0 : 1;
  const start = performance.now();
  cancelAnimationFrame(anim);
  const step = (now: number) => {
    const k = Math.min((now - start) / 900, 1);
    const e = k < 0.5 ? 4 * k * k * k : 1 - Math.pow(-2 * k + 2, 3) / 2;
    explode.value = from + (to - from) * e;
    if (k < 1) anim = requestAnimationFrame(step);
  };
  anim = requestAnimationFrame(step);
}

watch(explode, (v) => setExplode?.(v));
watch(edges, (v) => setEdges?.(v));
watch(spin, (v) => { if (controlsRef) controlsRef.autoRotate = v; });

onMounted(async () => {
  const el = host.value;
  if (!el) return;
  try {
    const THREE = await import("three");
    const { OrbitControls } = await import("three/examples/jsm/controls/OrbitControls.js");
    const { GLTFLoader } = await import("three/examples/jsm/loaders/GLTFLoader.js");
    const { MeshoptDecoder } = await import("three/examples/jsm/libs/meshopt_decoder.module.js");
    const { mergeVertices } = await import("three/examples/jsm/utils/BufferGeometryUtils.js");

    const renderer = new THREE.WebGLRenderer({ antialias: true, alpha: true });
    renderer.setPixelRatio(Math.min(window.devicePixelRatio, 2));
    renderer.outputColorSpace = THREE.SRGBColorSpace;
    el.appendChild(renderer.domElement);

    const scene = new THREE.Scene();
    // Plain CAD-style shading (like Ket's "normal" renders): matte, no reflections.
    const sun = new THREE.DirectionalLight(0xffffff, 1.6);
    sun.position.set(60, 120, 80);
    const fill = new THREE.DirectionalLight(0xffffff, 0.5);
    fill.position.set(-80, 40, -60);
    scene.add(sun, fill, new THREE.HemisphereLight(0xffffff, 0x8a8a8a, 1.3));

    const camera = new THREE.PerspectiveCamera(32, 1, 1, 2000);
    const controls = new OrbitControls(camera, renderer.domElement);
    controls.enableDamping = true;
    controls.autoRotate = spin.value;
    controls.autoRotateSpeed = 1.2;
    controls.enableZoom = !props.compact;
    controls.minDistance = 60;
    controls.maxDistance = 400;
    controlsRef = controls;

    const gltf = await new GLTFLoader().setMeshoptDecoder(MeshoptDecoder).loadAsync(withBase("/models/v2/web/clawd-v2.glb"));
    const root = gltf.scene;
    scene.add(root);

    // Parts move along the GLB axes: Ket (x, y, z) is (x, z, -y) here.
    const parts: { obj: THREE.Object3D; base: THREE.Vector3; off: THREE.Vector3 }[] = [];
    const lines: THREE.LineSegments[] = [];
    const pickable: THREE.Mesh[] = [];
    const lineMat = new THREE.LineBasicMaterial({ color: 0x111111, transparent: true, opacity: 0.85 });
    root.traverse((o) => {
      const mesh = o as THREE.Mesh;
      if (mesh.isMesh) {
        const src = mesh.material as THREE.MeshStandardMaterial;
        mesh.material = new THREE.MeshLambertMaterial({
          color: src.color,
          transparent: src.transparent,
          opacity: src.opacity,
          depthWrite: !src.transparent,
          side: THREE.DoubleSide,
        });
        src.dispose();
        // Weld the quantized vertices first, or every seam shows up as an edge.
        const welded = mergeVertices(mesh.geometry.clone().deleteAttribute("normal"), 1e-3);
        mesh.geometry = welded.toNonIndexed();
        mesh.geometry.computeVertexNormals();
        const line = new THREE.LineSegments(new THREE.EdgesGeometry(welded, 30), lineMat);
        line.raycast = () => {};
        mesh.add(line);
        lines.push(line);
        pickable.push(mesh);
      }
    });
    for (const child of [...root.children]) {
      const name = child.name;
      const k = name.startsWith("part_") ? "board" : name;
      const o = OFFSETS[k] ?? [0, 0, 0];
      parts.push({ obj: child, base: child.position.clone(), off: new THREE.Vector3(o[0], o[2], -o[1]) });
    }
    // Compact (home hero): straight from above when assembled (front of the
    // keyboard at the bottom), tilting to a side view and pulling back as it
    // comes apart, so the stacked parts separate on screen.
    const frame = (t: number) => {
      const y = props.compact ? 6 + 32 * t : 22;
      const dist = props.compact ? 150 + 155 * t : 230;
      let dir = camera.position.clone().sub(controls.target);
      if (dir.lengthSq() < 1e-6) dir.set(0.55, 0.45, 0.7);
      if (props.compact) {
        const polar = 0.001 + 1.05 * t;
        const azimuth = Math.atan2(dir.x, dir.z);
        dir = new THREE.Vector3().setFromSphericalCoords(1, polar, azimuth);
      }
      controls.target.set(center.x, baseY + y, center.z);
      camera.position.copy(controls.target).add(dir.normalize().multiplyScalar(dist));
    };
    setExplode = (t) => {
      for (const p of parts) p.obj.position.copy(p.base).addScaledVector(p.off, t);
      frame(t);
    };
    setEdges = (on) => { for (const l of lines) l.visible = on; };

    const box = new THREE.Box3().setFromObject(root);
    const center = box.getCenter(new THREE.Vector3());
    const baseY = center.y;
    camera.position.set(props.compact ? 0 : 0.55, props.compact ? 1 : 0.45, props.compact ? 0.001 : 0.7);
    setExplode(explode.value);
    setEdges(edges.value);

    const resize = () => {
      const w = el.clientWidth, h = el.clientHeight;
      renderer.setSize(w, h, false);
      camera.aspect = w / h;
      camera.updateProjectionMatrix();
    };
    const ro = new ResizeObserver(resize);
    ro.observe(el);
    resize();

    const ray = new THREE.Raycaster();
    const ptr = new THREE.Vector2();
    const onMove = (ev: PointerEvent) => {
      const r = renderer.domElement.getBoundingClientRect();
      ptr.set(((ev.clientX - r.left) / r.width) * 2 - 1, -((ev.clientY - r.top) / r.height) * 2 + 1);
      ray.setFromCamera(ptr, camera);
      const hit = ray.intersectObjects(pickable, false)[0];
      let o: THREE.Object3D | null = hit?.object ?? null;
      while (o && o.parent && o.parent !== root) o = o.parent;
      hovered.value = o ? LABELS[o.name] ?? "" : "";
    };
    const onLeave = () => { hovered.value = ""; };
    renderer.domElement.addEventListener("pointermove", onMove);
    renderer.domElement.addEventListener("pointerleave", onLeave);

    let raf = 0;
    const loop = () => {
      raf = requestAnimationFrame(loop);
      controls.update();
      renderer.render(scene, camera);
    };
    loop();
    loading.value = false;

    cleanup = () => {
      cancelAnimationFrame(raf);
      ro.disconnect();
      renderer.domElement.removeEventListener("pointermove", onMove);
      renderer.domElement.removeEventListener("pointerleave", onLeave);
      controls.dispose();
      renderer.dispose();
      renderer.domElement.remove();
    };
  } catch (e) {
    failed.value = String(e);
    loading.value = false;
  }
});

onBeforeUnmount(() => {
  cancelAnimationFrame(anim);
  cleanup?.();
});
</script>

<template>
  <figure class="clawd3d" :class="{ compact: props.compact }">
    <div
      ref="host"
      class="stage"
      role="img"
      aria-label="3D model of Clawd V2: an orange case with two black keycaps, two clear eyes and side arms. Drag to turn it."
    />
    <img v-if="failed" class="fallback" :src="withBase('/models/v2/images/v2-back.webp')" alt="Clawd V2 render" />
    <div v-if="loading" class="status">Loading 3D model…</div>
    <div v-if="hovered" class="label">{{ hovered }}</div>
    <div class="bar">
      <button v-if="!props.compact" type="button" class="btn primary" @click="toggleExplode">
        {{ explode > 0.5 ? "Assemble" : "Take apart" }}
      </button>
      <input
        v-model.number="explode"
        :style="{ '--p': explode }"
        class="slider"
        type="range"
        min="0"
        max="1"
        step="0.01"
        aria-label="Take apart: slide to separate the parts"
        :aria-valuetext="explode > 0.5 ? 'apart' : 'assembled'"
        title="Slide to take Clawd apart"
      />
      <template v-if="!props.compact">
        <label class="chk"><input v-model="spin" type="checkbox" /> Spin</label>
        <label class="chk"><input v-model="edges" type="checkbox" /> Edges</label>
      </template>
    </div>
  </figure>
</template>

<style scoped>
.clawd3d {
  position: relative;
  box-sizing: border-box;
  padding: 0;
  width: 100%;
  height: 480px;
  border-radius: 14px;
  border: 1px solid var(--vp-c-divider);
  background: radial-gradient(circle at 50% 40%, var(--vp-c-bg-soft), var(--vp-c-bg));
  overflow: hidden;
  margin: 16px 0;
}
.clawd3d.compact {
  height: 520px;
  border: none;
  background: transparent;
  margin: 0;
}
.stage {
  position: absolute;
  inset: 0;
  cursor: grab;
  touch-action: none;
}
.stage:active { cursor: grabbing; }
.stage :deep(canvas) { width: 100%; height: 100%; display: block; }
.fallback { position: absolute; inset: 0; width: 100%; height: 100%; object-fit: contain; }
.status, .label {
  position: absolute;
  left: 50%;
  transform: translateX(-50%);
  padding: 4px 10px;
  border-radius: 8px;
  font-size: 13px;
  background: var(--vp-c-bg-elv);
  border: 1px solid var(--vp-c-divider);
  pointer-events: none;
}
.status { top: 45%; }
.label { top: 10px; white-space: nowrap; }
.bar {
  position: absolute;
  left: 12px;
  right: 12px;
  bottom: 12px;
  display: flex;
  align-items: center;
  gap: 12px;
  font-size: 13px;
}
.btn {
  padding: 4px 14px;
  border-radius: 20px;
  border: 1px solid var(--vp-c-divider);
  background: var(--vp-c-bg-elv);
  font-weight: 600;
  white-space: nowrap;
}
.btn.primary { background: var(--vp-c-brand-3); border-color: var(--vp-c-brand-3); color: var(--vp-c-white); }
.btn.primary:hover { background: var(--vp-c-brand-2); }
.compact .bar { left: 20%; right: 20%; transition: opacity 0.2s ease; }
/* Home hero: the slider shows while the pointer is on the model (always on
   touch screens, which cannot hover). */
@media (hover: hover) {
  .compact .bar { opacity: 0; }
  .compact:hover .bar,
  .compact .bar:focus-within { opacity: 1; }
}
@media (max-width: 959px) {
  .clawd3d.compact { height: 360px; }
}
/* Thin track, a slim upright handle instead of the round knob. */
.slider {
  flex: 1;
  min-width: 60px;
  height: 20px;
  margin: 0;
  background: transparent;
  appearance: none;
  -webkit-appearance: none;
  cursor: pointer;
}
.slider::-webkit-slider-runnable-track {
  height: 2px;
  border-radius: 1px;
  background: linear-gradient(var(--vp-c-brand-1), var(--vp-c-brand-1)) 0 / calc(var(--p, 0) * 100%) 100% no-repeat,
    var(--vp-c-divider);
}
.slider::-moz-range-track {
  height: 2px;
  border-radius: 1px;
  background: var(--vp-c-divider);
}
.slider::-moz-range-progress {
  height: 2px;
  background: var(--vp-c-brand-1);
}
.slider::-webkit-slider-thumb {
  -webkit-appearance: none;
  width: 4px;
  height: 16px;
  margin-top: -7px;
  border-radius: 2px;
  border: 0;
  background: var(--vp-c-brand-1);
}
.slider::-moz-range-thumb {
  width: 4px;
  height: 16px;
  border-radius: 2px;
  border: 0;
  background: var(--vp-c-brand-1);
}
.slider:focus-visible {
  outline: 2px solid var(--vp-c-brand-1);
  outline-offset: 2px;
  border-radius: 4px;
}
.chk { display: flex; align-items: center; gap: 4px; white-space: nowrap; }
</style>
