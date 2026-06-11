project = "DEEPCRAFT Voice Assistant Model Deploy"
author = "Infineon Technologies"
release = "0.2.0"

extensions = [
    "sphinxcontrib.mermaid",
]

html_theme = "sphinx_rtd_theme"

html_static_path = ["_static"]
html_css_files = ["custom.css"]

# Mermaid: render diagrams client-side (no external server needed)
mermaid_output_format = "raw"
mermaid_version = "10.9.0"
