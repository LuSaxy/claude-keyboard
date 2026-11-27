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
      if (!name.startsWith("v2/images/")) files.push({ name, size: info.size });
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
  const page = frontmatter({ title: "Firmware docs", outline: "deep" }) + md;
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
    .replaceAll("](models/", "](/models/");
  const page = frontmatter({ title: "Project README" }) + md;
  await writeFile(path.join(docsDir, "readme.md"), page);
}

const files = await copyModels();
await generateFirmwarePage();
await generateReadmePage();
const v2 = await readFile(path.join(modelsSrc, "v2", "README.md"), "utf8");
await writeFile(path.join(docsDir, "v2.md"), frontmatter({ title: "V2 prototype", outline: "deep" }) + v2.replaceAll("](../LICENSE)", "](https://github.com/LuSaxy/claude-keyboard/blob/main/models/LICENSE)"));
console.log(`prepared: ${files.length} model file(s), firmware.md, readme.md`);
