# -*- coding: utf-8 -*-

# NOTE: if you have images in html/ and not in this directory
# you need to copy them here:
# cp html/_static/* html/_images/* . # this is more than enough
#
# ATM I'm using sphinx from 1.0-hg with a few hacks.
# hopefully in the future these hacks won't be necessary
#
# sphinx-build -d ./doctrees/ -b html . html


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
#latex_appendices = ['appendix']

download_prefix = 'http://downloads.sourceforge.net/fityk/'
msw_filename = 'fityk-%s-setup' % version
source_filename = 'fityk-%s' % version
extlinks = {
    'wiki': ('http://sourceforge.net/apps/trac/fityk/wiki/', ''),
    'sf-download-msw': (download_prefix + msw_filename, msw_filename),
    'sf-download-source': (download_prefix + source_filename, source_filename),
    }

