# -*- coding: utf-8 -*-

extensions = ["sphinx.ext.pngmath", "sphinx.ext.extlinks"]

exclude_trees = ['html', 'latex', '.svn']
templates_path = ['.']
source_suffix = '.rst'
source_encoding = 'utf-8'
master_doc = 'fityk-manual'
project = 'Fityk'
copyright = '2001-2009, Fityk Developers'
version = '0.8.9'
release = version
default_role = None
highlight_language = "none"

html_theme = "sphinxdoc"
html_theme_options = { "nosidebar": True }
html_sidebars = {'index': "nosidebar" }
html_short_title = 'Fityk %s manual' % version
html_title = html_short_title
html_favicon = 'fityk.ico'
html_static_path = [html_favicon, 'fityk-banner.png', 'fityk.css',
                    'unipress-button.png', 'wxwidgets_powered.png']
html_style = 'fityk.css'
html_last_updated_fmt = '%Y-%m-%d'
html_use_smartypants = True
html_use_modindex = False
html_use_index = False
html_add_permalinks = False

latex_documents = [
  ('fityk-manual', 'fityk-manual.tex', 'Fityk manual', '', 'manual', True),
]
latex_logo = 'fityk-banner.pdf'
latex_elements = { 'papersize': 'a4paper' # 'letterpaper'
                 }

download_prefix = 'http://downloads.sourceforge.net/fityk/'
msw_filename = 'fityk-%s-setup' % version
source_filename = 'fityk-%s' % version
extlinks = {
    'wiki': ('http://sourceforge.net/apps/trac/fityk/wiki/', ''),
    'sf-download-msw': (download_prefix + msw_filename, msw_filename),
    'sf-download-source': (download_prefix + source_filename, source_filename),
    }

