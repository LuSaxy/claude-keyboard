# Clawd site — the VitePress project lives in docs/.

DOCS := docs

.DEFAULT_GOAL := help

## node_modules is (re)installed when the lockfile changes or it's missing.
$(DOCS)/node_modules: $(DOCS)/package.json $(DOCS)/package-lock.json
	cd $(DOCS) && npm install
	@touch $(DOCS)/node_modules

.PHONY: docs-server
docs-server: $(DOCS)/node_modules ## start the docs dev server (default)
	cd $(DOCS) && npm run dev

.PHONY: docs-build
docs-build: $(DOCS)/node_modules ## build the static site into docs/.vitepress/dist
	cd $(DOCS) && npm run build

.PHONY: docs-preview
docs-preview: docs-build ## build, then serve the production output locally
	cd $(DOCS) && npm run preview

.PHONY: docs-clean
docs-clean: ## remove build output, caches, generated pages, and deps
	cd $(DOCS) && rm -rf node_modules .vitepress/dist .vitepress/cache public/models firmware.md readme.md

.PHONY: help
help: ## list the available targets
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | \
		awk 'BEGIN {FS = ":.*?## "}; {printf "  \033[36m%-13s\033[0m %s\n", $$1, $$2}'
