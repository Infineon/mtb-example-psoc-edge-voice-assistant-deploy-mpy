project = "DEEPCRAFT Voice Assistant Model Deploy"
author = "Infineon Technologies"
copyright = "2026 Infineon Technologies"
release = "1.0.0"

extensions = [
    "myst_parser",
    "sphinx_tabs.tabs",
    "sphinxemoji.sphinxemoji",
    "sphinxcontrib.mermaid",
]

source_suffix = [
    ".rst",
    ".md",
]

templates_path = ["_templates"]
exclude_patterns = ["_build", "_build_html", "Thumbs.db", ".DS_Store"]
suppress_warnings = ["epub.duplicated_toc_entry"]

html_theme = "sphinx_rtd_theme"
html_theme_options = {}
html_logo = "images/ifx_logo_white_green_s.png"
html_static_path = ["_static"]

# Mermaid: render diagrams client-side (no external server needed)
mermaid_output_format = "raw"
mermaid_version = "10.9.0"
