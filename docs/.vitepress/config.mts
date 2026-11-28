import { defineConfig } from "vitepress";

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "Clawd",
  description:
    "A tiny two-button BLE + USB keyboard built on the Seeed Studio XIAO nRF52840 — print it, flash it, and remap it from the browser.",
  lang: "en-US",
  head: [["link", { rel: "icon", type: "image/svg+xml", href: "/claude-keyboard/logo.svg" }]],

  // Project page served from https://lusaxy.github.io/claude-keyboard/
  base: "/claude-keyboard/",

  cleanUrls: true,
  lastUpdated: true,
  // The generated README pages carry repo-relative links; don't fail the build.
  ignoreDeadLinks: true,

  themeConfig: {
    logo: "/logo.svg",

    // The nav holds the two ways in (build one, or set one up); the sidebar
    // holds the order to read them in. Labels match between the two.
    nav: [
      { text: "Build guide", link: "/setup", activeMatch: "^/(setup|bom|models|v2|firmware|readme)" },
      { text: "Control app", link: "/app" },
    ],

    sidebar: [
      {
        text: "Build",
        items: [
          { text: "Overview", link: "/readme" },
          { text: "Build guide", link: "/setup" },
          { text: "Parts & battery", link: "/bom" },
          { text: "Print files & models", link: "/models" },
        ],
      },
      {
        text: "Use",
        items: [{ text: "Control app", link: "/app" }],
      },
      {
        text: "Reference",
        items: [
          { text: "V2 hardware & manufacturing", link: "/v2" },
          { text: "Firmware", link: "/firmware" },
        ],
      },
    ],

    outline: { label: "On this page" },
    docFooter: { prev: "Previous", next: "Next" },
    editLink: {
      pattern: "https://github.com/LuSaxy/claude-keyboard/edit/main/docs/:path",
      text: "Edit this page on GitHub",
    },

    socialLinks: [
      { icon: "github", link: "https://github.com/LuSaxy/claude-keyboard" },
    ],

    search: { provider: "local" },

    footer: {
      message:
        'Firmware and site: MIT. Models: <a href="https://creativecommons.org/licenses/by-nc-sa/4.0/">CC BY-NC-SA 4.0</a>, based on the <a href="https://www.myminifactory.com/object/3d-print-claude-code-mascot-fidget-toy-791307">Claude Code Mascot Fidget Toy</a> by 白林.',
      copyright: "Clawd Keyboard",
    },
  },
});
