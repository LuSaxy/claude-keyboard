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
  for (const name of (await readdir(modelsSrc)).sort()) {
    const src = path.join(modelsSrc, name);
    const info = await stat(src);
    if (!info.isFile()) continue;
    await cp(src, path.join(publicModels, name));
    files.push({ name, size: info.size });
  }

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
console.log(`prepared: ${files.length} model file(s), firmware.md, readme.md`);
