// Prepare generated inputs for the VitePress build.
//
// - Copies the Git LFS model files into docs/public/models so they are served at
//   /models/ (downloads + the hero/README image), and writes a manifest.json the
//   Models page reads.
// - Generates docs/firmware.md and docs/readme.md from the source README files so
//   the docs stay identical to the markdown sources (these two pages are
//   git-ignored and rebuilt on every dev/build).
//
// Paths are resolved relative to this file, so it works regardless of cwd.

import { cp, mkdir, readFile, readdir, rm, stat, writeFile } from "node:fs/promises";
import path from "node:path";
import { fileURLToPath } from "node:url";

const scriptDir = path.dirname(fileURLToPath(import.meta.url)); // docs/scripts
const docsDir = path.resolve(scriptDir, ".."); // docs
const root = path.resolve(docsDir, ".."); // repository root

const modelsSrc = path.join(root, "models");
const publicModels = path.join(docsDir, "public", "models");

// Drop the level-2 sections with these titles (and their subsections) from a
// markdown page: repo-only material that has its own page on the site.
function dropSections(md, titles) {
  const lines = md.split("\n");
  const out = [];
  let skipping = false;
  let fence = false;
  for (const line of lines) {
    if (line.startsWith("```")) fence = !fence;
    if (!fence && line.startsWith("## ")) skipping = titles.includes(line.slice(3).trim());
    if (!skipping) out.push(line);
  }
  return out.join("\n");
}

// Replace the first "# ..." heading (and anything before it) with a site title.
function retitle(md, title) {
  const i = md.search(/^# /m);
  const rest = i < 0 ? md : md.slice(md.indexOf("\n", i) + 1);
  return `# ${title}\n${rest}`;
}

function frontmatter(fields) {
  const body = Object.entries(fields)
    .map(([key, value]) => `${key}: ${value}`)
    .join("\n");
  return `---\n${body}\n---\n\n`;
}

async function copyModels() {
  await rm(publicModels, { recursive: true, force: true });
  await mkdir(publicModels, { recursive: true });

  const files = [];
  async function walk(relative = "") {
    for (const entry of (await readdir(path.join(modelsSrc, relative), { withFileTypes: true })).sort((a, b) => a.name.localeCompare(b.name))) {
      const name = path.posix.join(relative, entry.name);
      if (entry.isDirectory()) { await walk(name); continue; }
      if (!entry.isFile()) continue;
      const src = path.join(modelsSrc, name);
      const info = await stat(src);
      const dest = path.join(publicModels, name);
      await mkdir(path.dirname(dest), { recursive: true });
      await cp(src, dest);
      // Renders are shown on the Models page, not listed as downloads.
      if (!name.startsWith("v2/images/") && !name.startsWith("v2/web/")) files.push({ name, size: info.size });
    }
  }
  await walk();
  files.sort((a, b) => Number(b.name.startsWith("v2/")) - Number(a.name.startsWith("v2/")) || a.name.localeCompare(b.name));

  await writeFile(
    path.join(publicModels, "manifest.json"),
    JSON.stringify(files, null, 2),
  );
  return files;
}

async function generateFirmwarePage() {
  const md = await readFile(
    path.join(root, "program", "xiao_ble_keyboard", "README.md"),
    "utf8",
  );
  // The README opens with a support table and its own GitHub title; the
  // site page gets a plain title, and the battery section points at the
  // parts page, which has the full guide.
  let body = retitle(md, "Firmware reference");
  body = body.replace(
    /^## Battery\n/m,
    "## Battery\n\n::: tip Choosing, polarity, charging\nThe battery guide for builders (choosing a cell, polarity, the charger check, troubleshooting) is on the [parts page](/bom#battery). This section is the firmware side.\n:::\n",
  );
  // Long platform-specific blocks fold away.
  body = body
    .replace(/^### Manual \/ two-step upload[^\n]*\n([\s\S]*?)(?=^### )/m, (m, inner) =>
      `### Manual / two-step upload (WSL, USB-IP, or when auto-reset fails)\n\n::: details Show the manual upload steps\n${inner.trim()}\n:::\n\n`)
    .replace(/^## USB in WSL\n([\s\S]*)$/m, (m, inner) =>
      `## USB in WSL\n\n::: details Show the usbipd steps\n${inner.trim()}\n:::\n`);
  const page = frontmatter({ title: "Firmware reference", outline: "[2, 3]" }) + body;
  await writeFile(path.join(docsDir, "firmware.md"), page);
}

async function generateReadmePage() {
  let md = await readFile(path.join(root, "README.md"), "utf8");
  // Point repo-relative doc links at the rendered VitePress pages, and resolve
  // models/* images & links to the public path (served from docs/public/models).
  md = md
    .replaceAll("](models/v2/README.md)", "](/v2)")
    .replaceAll("](program/xiao_ble_keyboard/README.md)", "](/firmware)")
    .replaceAll("](docs/app/README.md)", "](/app)")
    .replaceAll("](app/README.md)", "](/app)")
    .replaceAll("](models/", "](/models/")
    .replaceAll("](docs/bom.md)", "](/bom)")
    // The V2 render at the top becomes the interactive 3D view on the site.
    .replace("![Clawd V2 render](/models/v2/images/v2-back.webp)", "<Clawd3D />");
  // The site has its own pages for these; the repo README keeps them.
  md = dropSections(md, ["Repository", "Git LFS", "Firmware", "Control App"]);
  md = retitle(md, "Overview")
    // The live-site line and the "in docs/bom.md (on the site: ...)" aside
    // only make sense on GitHub.
    .replace(/^\*\*Live site:\*\*[\s\S]*?\n\n/m, "")
    .replace("[`docs/bom.md`](/bom) (on the site: **Parts list (BOM)**)", "the [parts list](/bom)")
    .replace("review\n> [`models/v2/`](/v2) before", "review\n> [V2 hardware](/v2) before");
  md = md.replace(
    /^## Highlights/m,
    "::: tip Where to go\n**Build one:** the [build guide](/setup), step by step. **Already have one:** the [control app](/app).\n:::\n\n## Highlights",
  );
  const page = frontmatter({ title: "Overview" }) + md;
  await writeFile(path.join(docsDir, "readme.md"), page);
}

const files = await copyModels();
await generateFirmwarePage();
await generateReadmePage();
const v2 = await readFile(path.join(modelsSrc, "v2", "README.md"), "utf8");
await writeFile(path.join(docsDir, "v2.md"), frontmatter({ title: "V2 hardware & manufacturing", outline: "[2, 3]" }) + retitle(v2, "V2 hardware & manufacturing").replaceAll("](../LICENSE)", "](https://github.com/LuSaxy/claude-keyboard/blob/main/models/LICENSE)"));
console.log(`prepared: ${files.length} model file(s), firmware.md, readme.md`);
